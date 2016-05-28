// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include <type_traits>
#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/trim.hpp>
#include "Win32Exception.hpp"
#include "ServiceControlManager.hpp"
#include "Path.hpp"
#include "Registry.hpp"
#include "File.hpp"
#include "Utf8.hpp"

namespace Instalog
{
namespace SystemFacades
{

Service::Service( std::string const& serviceName, std::string const& displayName, SERVICE_STATUS const& status, SC_HANDLE scmHandle ) 
        : serviceName(serviceName)
        , displayName(displayName)
        , svchostDamaged(false)
{
    // Set the state
    switch (status.dwCurrentState)
    {
    case SERVICE_STOPPED:
        this->state = "S";
        break;
    case SERVICE_START_PENDING:
        this->state = "R?";
        break;
    case SERVICE_STOP_PENDING:
        this->state = "S?";
        break;
    case SERVICE_RUNNING:
        this->state = "R";
        break;
    case SERVICE_CONTINUE_PENDING:
        this->state = "C?";
        break;
    case SERVICE_PAUSE_PENDING:
        this->state = "P?";
        break;
    case SERVICE_PAUSED:
        this->state = "P";
        break;
    default:
        this->state = "?";
        break;
    }

    std::wstring wideServiceName(utf8::ToUtf16(this->serviceName));
    serviceHandle = OpenServiceW(
        scmHandle, wideServiceName.c_str(), SERVICE_QUERY_CONFIG);
    if (serviceHandle == NULL)
    {
        Win32Exception::ThrowFromLastError();
    }
    std::aligned_storage<8192 /* (8K bytes maximum size) */,
                         std::alignment_of<QUERY_SERVICE_CONFIGW>::value>::type
    queryServiceConfigBuffer;
    DWORD bytesNeeded = 0; // not needed
    if (QueryServiceConfig(serviceHandle,
                           reinterpret_cast<LPQUERY_SERVICE_CONFIGW>(
                               &queryServiceConfigBuffer),
                           8192,
                           &bytesNeeded) == false)
    {
        Win32Exception::ThrowFromLastError();
    }
    QUERY_SERVICE_CONFIGW* queryServiceConfig =
        reinterpret_cast<QUERY_SERVICE_CONFIGW*>(&queryServiceConfigBuffer);

    // Set the start type
    this->start = queryServiceConfig->dwStartType;

    // Set the file
    this->filepath = utf8::ToUtf8(queryServiceConfig->lpBinaryPathName);
    if (filepath.empty())
    {
        if (queryServiceConfig->dwServiceType == SERVICE_KERNEL_DRIVER ||
            queryServiceConfig->dwServiceType == SERVICE_FILE_SYSTEM_DRIVER)
        {
            filepath =
                Path::Append(Path::GetWindowsPath(),
                             "System32\\Drivers\\" + serviceName + ".sys");
        }
        else
        {
            filepath = Path::Append(Path::GetWindowsPath(),
                                    "System32\\" + serviceName + ".exe");
        }
    }
    Path::ResolveFromCommandLine(this->filepath);

    static std::string svchostPath(
        Path::Append(Path::GetWindowsPath(), "System32\\Svchost.exe"));
    // Set the svchost group, dll path, and damaged status if applicable
    if (!boost::iequals(filepath, svchostPath))
    {
        return;
    }

    // Get the svchost group
    this->svchostGroup = utf8::ToUtf8(queryServiceConfig->lpBinaryPathName);
    this->svchostGroup.erase(
        this->svchostGroup.begin(),
        boost::ifind_first(this->svchostGroup, "-k").end());
    boost::trim(this->svchostGroup);

    // Check to see if it's damaged
    RegistryKey svchostGroupKey = RegistryKey::Open(
        "\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Svchost",
        KEY_QUERY_VALUE);

    try
    {
        RegistryValue svchostGroupRegistration{svchostGroupKey.GetValue(this->svchostGroup)};
        std::vector<std::string> svchostGroupRegistrationStrings =
            svchostGroupRegistration.GetMultiStringArray();
        std::locale loc;
        auto groupRef = std::find_if(svchostGroupRegistrationStrings.begin(),
                                     svchostGroupRegistrationStrings.end(),
                                         [&](std::string const & a)->bool {
            return boost::iequals(a, serviceName, loc);
        });
        svchostDamaged = groupRef == svchostGroupRegistrationStrings.end();
    }
    catch (ErrorFileNotFoundException const&)
    {
        // value not found, that's OK
    }

    // Get the dll path
    std::string serviceKeyName =
        "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" +
        serviceName;
    RegistryKey serviceParameters =
        RegistryKey::Open(serviceKeyName + "\\Parameters", KEY_QUERY_VALUE);
    if (serviceParameters.Invalid())
    {
        serviceParameters = RegistryKey::Open(serviceKeyName, KEY_QUERY_VALUE);
    }
    if (serviceParameters.Invalid())
    {
        this->svchostDll = expected<std::string>::from_exception(
            Win32Exception::FromNtError(::GetLastError()));
        return;
    }

    try
    {
        expected<RegistryValue> serviceDllValue =
            serviceParameters.GetValue("ServiceDll");
        if (serviceDllValue.is_valid())
        {
            std::string rawValue = serviceDllValue.get().GetStringStrict();
            Path::ResolveFromCommandLine(rawValue);
            this->svchostDll = std::move(rawValue);
        }
    }
    catch (std::exception&)
    {
        this->svchostDll = expected<std::string>::from_exception();
    }
}

Service::Service( Service && s ) 
        : serviceHandle(s.serviceHandle)
        , serviceName(std::move(s.serviceName))
        , displayName(std::move(s.displayName))
        , state(std::move(s.state))
        , start(s.start)
        , filepath(std::move(s.filepath))
        , svchostGroup(std::move(s.svchostGroup))
        , svchostDll(std::move(s.svchostDll))
        , svchostDamaged(s.svchostDamaged)
{
    s.serviceHandle = NULL;
}

Service::~Service()
{
    CloseServiceHandle(serviceHandle);
}

Service& Service::operator=(Service s)
{
    serviceName = std::move(s.serviceName);
    displayName = std::move(s.displayName);
    state = std::move(s.state);
    start = s.start;
    filepath = std::move(s.filepath);
    svchostGroup = std::move(s.svchostGroup);
    svchostDll = std::move(s.svchostDll);
    svchostDamaged = s.svchostDamaged;
    return *this;
}

ServiceControlManager::ServiceControlManager(
    DWORD
        desiredAccess /* = SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE */)
{
    scmHandle = ::OpenSCManagerW(NULL, NULL, desiredAccess);

    if (scmHandle == NULL)
    {
        Win32Exception::ThrowFromLastError();
    }
}

ServiceControlManager::~ServiceControlManager()
{
    ::CloseServiceHandle(scmHandle);
}

std::vector<Service> ServiceControlManager::GetServices() const
{
    DWORD const bufferSize = 65536;
    std::vector<Service> services;

    // 64k bytes stack buffer size aligned for ENUM_SERVICE_STATUSW structures.
    typedef std::aligned_storage<
        bufferSize,
        std::alignment_of<ENUM_SERVICE_STATUSW>::value>::type buffer_type;
    buffer_type servicesBufferBacking;
    auto servicesBuffer =
        reinterpret_cast<ENUM_SERVICE_STATUSW*>(&servicesBufferBacking);
    DWORD bytesNeeded = 0; // not needed
    DWORD servicesReturned = 0;
    DWORD resumeHandle = 0;
    BOOL status = false;
    DWORD error = ERROR_MORE_DATA;

    do
    {
        status = ::EnumServicesStatusW(scmHandle,
                                       SERVICE_DRIVER | SERVICE_WIN32,
                                       SERVICE_STATE_ALL,
                                       servicesBuffer,
                                       bufferSize,
                                       &bytesNeeded,
                                       &servicesReturned,
                                       &resumeHandle);
        error = ::GetLastError();
        if (status == false && error != ERROR_MORE_DATA)
        {
            Win32Exception::Throw(error);
        }

        for (auto enumServiceStatus = servicesBuffer; servicesReturned > 0;
             --servicesReturned, ++enumServiceStatus)
        {
            services.emplace_back(utf8::ToUtf8(enumServiceStatus->lpServiceName),
                                  utf8::ToUtf8(enumServiceStatus->lpDisplayName),
                                  enumServiceStatus->ServiceStatus,
                                  scmHandle);
        }
    } while (status == false && error == ERROR_MORE_DATA);

    return services;
}
}
}