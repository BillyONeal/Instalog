// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include <boost/config.hpp>
#include <windows.h>
#include "Registry.hpp"
#include "Win32Exception.hpp"
#include "Library.hpp"
#include "Path.hpp"
#include "File.hpp"
#include "StringUtilities.hpp"
#include "StockOutputFormats.hpp"

using Instalog::SystemFacades::RegistryKey;
using Instalog::SystemFacades::Win32Exception;
using Instalog::SystemFacades::ErrorFileNotFoundException;
using Instalog::SystemFacades::ErrorPathNotFoundException;
using Instalog::SystemFacades::File;

namespace Instalog
{

static void FileTimeToSystemTimeImpl(std::uint64_t time, SYSTEMTIME& st)
{
    if (FileTimeToSystemTime(reinterpret_cast<FILETIME const*>(&time), &st) == 0)
    {
        Win32Exception::ThrowFromLastError();
    }
}

void WriteDefaultDateFormat(log_sink& str, std::uint64_t time)
{
    SYSTEMTIME st;
    FileTimeToSystemTimeImpl(time, st);
    // YYYY-MM-DD HH:MM:SS
    write(str, pad(4, '0', st.wYear), '-', pad(2, '0', st.wMonth), '-', pad(2, '0', st.wDay), ' ', pad(2, '0', st.wHour), ':', pad(2, '0', st.wMinute), ':', pad(2, '0', st.wSecond));
}
void WriteMillisecondDateFormat(log_sink& str, std::uint64_t time)
{
    SYSTEMTIME st;
    FileTimeToSystemTimeImpl(time, st);
    // YYYY-MM-DD HH:MM:SS.mmmm
    // YYYY-MM-DD HH:MM:SS
    write(str, pad(4, '0', st.wYear), '-', pad(2, '0', st.wMonth), '-', pad(2, '0', st.wDay), ' ', pad(2, '0', st.wHour), ':', pad(2, '0', st.wMinute), ':', pad(2, '0', st.wSecond), '.', pad(4, '0', st.wMilliseconds));
}
void WriteFileAttributes(log_sink& str, std::uint32_t attributes)
{
    char const result[] = {
        (attributes & FILE_ATTRIBUTE_DIRECTORY     ? 'd' : '-'),
        (attributes & FILE_ATTRIBUTE_COMPRESSED    ? 'c' : '-'),
        (attributes & FILE_ATTRIBUTE_SYSTEM        ? 's' : '-'),
        (attributes & FILE_ATTRIBUTE_HIDDEN        ? 'h' : '-'),
        (attributes & FILE_ATTRIBUTE_ARCHIVE       ? 'a' : '-'),
        (attributes & FILE_ATTRIBUTE_TEMPORARY     ? 't' : '-'),
        (attributes & FILE_ATTRIBUTE_READONLY      ? 'r' : 'w'),
        (attributes & FILE_ATTRIBUTE_REPARSE_POINT ? 'r' : '-')
    };

    str.append(result, sizeof(result));
}
void WriteDefaultFileOutput(log_sink& str, std::string targetFile)
{
    if (Path::ResolveFromCommandLine(targetFile) == false)
    {
        write(str, targetFile, " [x]");
        return;
    }
    std::string companyInfo(1, ' ');
    try
    {
        companyInfo.append(File::GetCompany(targetFile));
    }
    catch (Win32Exception const&)
    {
        companyInfo.clear();
    }
    write(str, targetFile);
    try
    {
        WIN32_FILE_ATTRIBUTE_DATA fad = File::GetExtendedAttributes(targetFile);
        std::uint64_t size =
            static_cast<std::uint64_t>(fad.nFileSizeHigh) << 32 | fad.nFileSizeLow;
        write(str, " [", size, ' ');
        std::uint64_t ctime =
            static_cast<std::uint64_t>(fad.ftCreationTime.dwHighDateTime) << 32 |
            fad.ftCreationTime.dwLowDateTime;
        WriteDefaultDateFormat(str, ctime);
        write(str, companyInfo, "]");
    }
    catch (ErrorFileNotFoundException const&)
    {
        write(str, " [?]");
    }
    catch (ErrorPathNotFoundException const&)
    {
        write(str, " [?]");
    }
}

void WriteFileListingFile(log_sink& str, std::string const& targetFile)
{
    WIN32_FILE_ATTRIBUTE_DATA fad = File::GetExtendedAttributes(targetFile);
    std::uint64_t size =
        static_cast<std::uint64_t>(fad.nFileSizeHigh) << 32 | fad.nFileSizeLow;
    std::uint64_t ctime =
        static_cast<std::uint64_t>(fad.ftCreationTime.dwHighDateTime) << 32 |
        fad.ftCreationTime.dwLowDateTime;
    std::uint64_t mtime =
        static_cast<std::uint64_t>(fad.ftLastWriteTime.dwHighDateTime) << 32 |
        fad.ftLastWriteTime.dwLowDateTime;
    WriteDefaultDateFormat(str, ctime);
    str.append(" . ", 3);
    WriteDefaultDateFormat(str, mtime);
    write(str, ' ', pad(10, ' ', size), ' ');
    WriteFileAttributes(str, fad.dwFileAttributes);
    write(str, ' ', targetFile);
}

void WriteFileListingFromFindData(log_sink& str,
                                  SystemFacades::FindFilesRecord const& fad)
{
    WriteDefaultDateFormat(str, fad.GetCreationTime());
    str.append(" . ", 3);
    WriteDefaultDateFormat(str, fad.GetLastWriteTime());
    write(str, ' ', pad(10, ' ', fad.GetSize()), ' ');
    WriteFileAttributes(str, fad.GetAttributes());
    str.append(" ", 1);
    std::string escapedFileName;
    write(escapedFileName, fad.GetFileName());
    GeneralEscape(escapedFileName);
    write(str, escapedFileName);
}

void WriteMemoryInformation(log_sink& log)
{
    std::uint64_t availableRam = 0;
    std::uint64_t totalRam = 0;
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memStatus);

    availableRam = memStatus.ullAvailPhys;

    Instalog::SystemFacades::RuntimeDynamicLinker kernel32("kernel32.dll");
    typedef BOOL(WINAPI *
                 GetPhysicallyInstalledFunc)(PULONGLONG TotalMemoryInKilobytes);
    try
    {
        auto gpFunc = kernel32.GetProcAddress<GetPhysicallyInstalledFunc>(
            "GetPhysicallyInstalledSystemMemory");
        gpFunc(&totalRam);
        totalRam *= 1024;
    }
    catch (SystemFacades::ErrorProcedureNotFoundException&)
    {
        totalRam = memStatus.ullTotalPhys;
    }
    totalRam /= 1024 * 1024;
    availableRam /= 1024 * 1024;
    write(log, availableRam, '/', totalRam, " MB Free");
}

void WriteOsVersion(log_sink& log)
{
    typedef BOOL(WINAPI * GetProductInfoFunc)(
        DWORD, DWORD, DWORD, DWORD, PDWORD);
    OSVERSIONINFOEX versionInfo;
    ZeroMemory(&versionInfo, sizeof(OSVERSIONINFOEX));
    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((LPOSVERSIONINFO) & versionInfo);

    DWORD productType = 0;
    Instalog::SystemFacades::RuntimeDynamicLinker kernel32("kernel32.dll");

    try
    {
        auto getProductInfo =
            kernel32.GetProcAddress<GetProductInfoFunc>("GetProductInfo");
        getProductInfo(6, 3, 0, 0, &productType);
    }
    catch (SystemFacades::ErrorProcedureNotFoundException const&)
    {
    } // Expected on earlier OSes

    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);

    write(log, "Windows ");
    char const* professionalName = "Professional Edition";

    switch (versionInfo.dwMajorVersion)
    {
    case 5:
        switch (versionInfo.dwMinorVersion)
        {
        case 0:
            write(log, "2000 ");
            if (versionInfo.wProductType == VER_NT_WORKSTATION)
                write(log, "Professional");
            else
            {
                if (versionInfo.wSuiteMask & VER_SUITE_DATACENTER)
                    write(log, "Datacenter Server");
                else if (versionInfo.wSuiteMask & VER_SUITE_ENTERPRISE)
                    write(log, "Enterprise Server");
                else
                    write(log, "Server");
            }
            break;
        case 1:
            write(log, "XP ");
            if (GetSystemMetrics(SM_MEDIACENTER))
                write(log, "Media Center Edition");
            else if (GetSystemMetrics(SM_STARTER))
                write(log, "Starter Edition");
            else if (GetSystemMetrics(SM_TABLETPC))
                write(log, "Tablet PC Edition");
            else if (versionInfo.wSuiteMask & VER_SUITE_PERSONAL)
                write(log, "Home Edition");
            else
                write(log, "Professional Edition");
            break;
        case 2:
            if (GetSystemMetrics(SM_SERVERR2))
                write(log, "Server 2003 R2 ");
            else if (versionInfo.wSuiteMask == VER_SUITE_STORAGE_SERVER)
                write(log, "Storage Server 2003");
            else if (versionInfo.wSuiteMask ==
                     /*VER_SUITE_WH_SERVER*/ 0x00008000)
                write(log, "Home Server");
            else if (versionInfo.wProductType == VER_NT_WORKSTATION &&
                     systemInfo.wProcessorArchitecture ==
                         PROCESSOR_ARCHITECTURE_AMD64)
                write(log, "XP Professional x64 Edition");
            else
                write(log, "Server 2003 ");

            if (versionInfo.wProductType != VER_NT_WORKSTATION)
            {
                if (systemInfo.wProcessorArchitecture ==
                    PROCESSOR_ARCHITECTURE_AMD64)
                {
                    if (versionInfo.wSuiteMask & VER_SUITE_DATACENTER)
                        write(log, "Datacenter x64 Edition");
                    else if (versionInfo.wSuiteMask & VER_SUITE_ENTERPRISE)
                        write(log, "Enterprise x64 Edition");
                    else
                        write(log, "Standard x64 Edition");
                }
                else
                {
                    if (versionInfo.wSuiteMask & VER_SUITE_COMPUTE_SERVER)
                        write(log, "Compute Cluster Edition");
                    else if (versionInfo.wSuiteMask & VER_SUITE_DATACENTER)
                        write(log, "Datacenter Edition");
                    else if (versionInfo.wSuiteMask & VER_SUITE_ENTERPRISE)
                        write(log, "Enterprise Edition");
                    else if (versionInfo.wSuiteMask & VER_SUITE_BLADE)
                        write(log, "Web Edition");
                    else
                        write(log, "Standard Edition");
                }
            }
        }
        break;
    case 6:
        switch (versionInfo.dwMinorVersion)
        {
        case 0:
            if (versionInfo.wProductType == VER_NT_WORKSTATION)
                write(log, "Vista ");
            else
                write(log, "Server 2008 ");

            break;
        case 1:
            if (versionInfo.wProductType == VER_NT_WORKSTATION)
                write(log, "7 ");
            else
                write(log, "Server 2008 R2 ");
            break;
        case 2:
            professionalName = "Pro";
            if (versionInfo.wProductType == VER_NT_WORKSTATION)
                write(log, "8 ");
            else
                write(log, "Server 2012 ");
            break;
        case 3:
            professionalName = "Pro";
            if (versionInfo.wProductType == VER_NT_WORKSTATION)
                write(log, "8.1 ");
            else
                write(log, "Server 2012 R2 ");
            break;
        }
        switch (productType)
        {
        case PRODUCT_BUSINESS:
            write(log, "Business Edition");
            break;
        case PRODUCT_BUSINESS_N:
            write(log, "Business Edition N");
            break;
        case PRODUCT_CLUSTER_SERVER:
            write(log, "HPC Edition");
            break;
        case PRODUCT_DATACENTER_SERVER:
            write(log, "Server Datacenter Edition");
            break;
        case PRODUCT_DATACENTER_SERVER_CORE:
            write(log, "Server Datacenter Edition (Server Core)");
            break;
        case /*PRODUCT_DATACENTER_SERVER_CORE_V*/ 0x00000027:
            write(log, "Server Datacenter Edition without Hyper-V (Server Core)");
            break;
        case /*PRODUCT_DATACENTER_SERVER_V*/ 0x00000025:
            write(log, "Server Datacenter Edition without Hyper-V");
            break;
        case PRODUCT_ENTERPRISE:
            write(log, "Enterprise Edition");
            break;
        case /*PRODUCT_ENTERPRISE_E*/ 0x00000046:
            write(log, "Enterprise Edition E");
            break;
        case /*PRODUCT_ENTERPRISE_N*/ 0x0000001B:
            write(log, "Enterprise Edition N");
            break;
        case PRODUCT_ENTERPRISE_SERVER:
            write(log, "Server Enterprise Edition");
            break;
        case PRODUCT_ENTERPRISE_SERVER_CORE:
            write(log, "Server Enterprise Edition (Server Core)");
            break;
        case /*PRODUCT_ENTERPRISE_SERVER_CORE_V*/ 0x00000029:
            write(log, "Server Enterprise Edition without Hyper-V (Server Core)");
            break;
        case PRODUCT_ENTERPRISE_SERVER_IA64:
            write(log, "Server Enterprise Edition for Itanium-based Systems");
            break;
        case /*PRODUCT_ENTERPRISE_SERVER_V*/ 0x00000026:
            write(log, "Server Enterprise Edition without Hyper-V");
            break;
        case PRODUCT_HOME_BASIC:
            write(log, "Home Basic Edition");
            break;
        case /*PRODUCT_HOME_BASIC_E*/ 0x00000043:
            write(log, "Home Basic Edition E");
            break;
        case /*PRODUCT_HOME_BASIC_N*/ 0x00000005:
            write(log, "Home Basic Edition N");
            break;
        case PRODUCT_HOME_PREMIUM:
            write(log, "Home Premium Edition");
            break;
        case /*PRODUCT_HOME_PREMIUM_E*/ 0x00000044:
            write(log, "Home Premium Edition E");
            break;
        case /*PRODUCT_HOME_PREMIUM_N*/ 0x0000001A:
            write(log, "Home Premium Edition N");
            break;
        case /*PRODUCT_HYPERV*/ 0x0000002A:
            write(log, "Hyper-V Server Edition");
            break;
        case /*PRODUCT_MEDIUMBUSINESS_SERVER_MANAGEMENT*/ 0x0000001E:
            write(log, "Essential Business Server Management Server");
            break;
        case /*PRODUCT_MEDIUMBUSINESS_SERVER_MESSAGING*/ 0x00000020:
            write(log, "Essential Business Server Messaging Server");
            break;
        case /*PRODUCT_MEDIUMBUSINESS_SERVER_SECURITY*/ 0x0000001F:
            write(log, "Essential Business Server Security Server");
            break;
        case /*PRODUCT_PROFESSIONAL*/ 0x00000030:
            write(log, professionalName);
            break;
        case /*PRODUCT_PROFESSIONAL_E*/ 0x00000045:
            write(log, professionalName, " E");
            break;
        case /*PRODUCT_PROFESSIONAL_N*/ 0x00000031:
            write(log, professionalName, " N");
            break;
        case PRODUCT_SERVER_FOR_SMALLBUSINESS:
            write(log, "for Windows Essential Server Solutions");
            break;
        case /*PRODUCT_SERVER_FOR_SMALLBUSINESS_V*/ 0x00000023:
            write(log, "without Hyper-V for Windows Essential Server Solutions");
            break;
        case /*PRODUCT_SERVER_FOUNDATION*/ 0x00000021:
            write(log, "Server Foundation Edition");
            break;
        case PRODUCT_SMALLBUSINESS_SERVER:
            write(log, "Small Business Server");
            break;
        case PRODUCT_STANDARD_SERVER:
            write(log, "Server Standard Edition");
            break;
        case PRODUCT_STANDARD_SERVER_CORE:
            write(log, "Server Standard Edition (Server Core)");
            break;
        case /*PRODUCT_STANDARD_SERVER_CORE_V*/ 0x00000028:
            write(log, "Server Standard Edition without Hyper-V (Server Core)");
            break;
        case /*PRODUCT_STANDARD_SERVER_V*/ 0x00000024:
            write(log, "Server Standard Edition without Hyper-V");
            break;
        case PRODUCT_STARTER:
            write(log, "Starter Edition");
            break;
        case /*PRODUCT_STARTER_E*/ 0x00000042:
            write(log, "Starter Edition E");
            break;
        case /*PRODUCT_STARTER_N*/ 0x0000002F:
            write(log, "Starter Edition N");
            break;
        case PRODUCT_STORAGE_ENTERPRISE_SERVER:
            write(log, "Storage Server Enterprise Edition");
            break;
        case PRODUCT_STORAGE_EXPRESS_SERVER:
            write(log, "Storage Server Express Edition");
            break;
        case PRODUCT_STORAGE_STANDARD_SERVER:
            write(log, "Storage Server Standard Edition");
            break;
        case PRODUCT_STORAGE_WORKGROUP_SERVER:
            write(log, "Storage Server Workgroup Edition");
            break;
        case PRODUCT_ULTIMATE:
            write(log, "Ultimate Edition");
            break;
        case /*PRODUCT_ULTIMATE_E*/ 0x00000047:
            write(log, "Ultimate Edition E");
            break;
        case /*PRODUCT_ULTIMATE_N*/ 0x0000001C:
            write(log, "Ultimate Edition N");
            break;
        case PRODUCT_WEB_SERVER:
            write(log, "Web Edition");
            break;
        case /*PRODUCT_WEB_SERVER_CORE*/ 0x0000001D:
            write(log, "Web Edition (Server Core)");
            break;
        case /*PRODUCT_CONSUMER_PREVIEW ?*/ 0x0000004A:
            write(log, "Consumer Preview");
            break;
        default:
        {
            write(log, "UNKNOWN EDITION (GetProductInfo -> 0x", hex(productType), ')');
        }
        }
        break;
    }
    if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
    {
        write(log, " x86 ");
    }
    else if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
    {
        write(log, " x64 ");
    }
    write(log, versionInfo.dwMajorVersion, '.', versionInfo.dwMinorVersion, '.',
          versionInfo.dwBuildNumber, '.', versionInfo.wServicePackMajor);
}

static std::string GetAdobeReaderVersion()
{
    using SystemFacades::RegistryKey;
#ifdef _M_X64
    RegistryKey adobeKey = RegistryKey::Open(
        "\\Registry\\Machine\\Software\\Wow6432Node\\Adobe\\Installer",
        KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
#else
    RegistryKey adobeKey =
        RegistryKey::Open("\\Registry\\Machine\\Software\\Adobe\\Installer",
                          KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
#endif
    if (adobeKey.Invalid())
    {
        throw std::exception("Broken Adobe Reader Install");
    }

    auto subkeys = adobeKey.EnumerateSubKeys(KEY_QUERY_VALUE);
    if (subkeys.empty())
    {
        throw std::exception("Broken Adobe Reader Install");
    }
    return subkeys[0]["ProductVersion"].GetStringStrict();
}

LONG GetTimeZoneBias()
{
    TIME_ZONE_INFORMATION tzInfo;
    auto tzSetting = GetTimeZoneInformation(&tzInfo);
    switch (tzSetting)
    {
    case TIME_ZONE_ID_STANDARD:
        tzInfo.Bias += tzInfo.StandardBias;
    case TIME_ZONE_ID_DAYLIGHT:
        tzInfo.Bias += tzInfo.DaylightBias;
    }
    ;

    return tzInfo.Bias;
}

std::uint64_t GetLocalTime()
{
    SYSTEMTIME st;
    FILETIME ft;
    GetLocalTime(&st);
    SystemTimeToFileTime(&st, &ft);
    return FiletimeToInteger(ft);
}

static void WriteFlashData(log_sink& log)
{
#ifdef _M_X64
    RegistryKey flashPluginKey = RegistryKey::Open(
        "\\Registry\\Machine\\Software\\Wow6432Node\\Macromedia\\FlashPlayerPlugin",
        KEY_QUERY_VALUE);
#else
    RegistryKey flashPluginKey = RegistryKey::Open(
        "\\Registry\\Machine\\Software\\Macromedia\\FlashPlayerPlugin",
        KEY_QUERY_VALUE);
#endif

#ifdef _M_X64
    RegistryKey flashKey = RegistryKey::Open(
        "\\Registry\\Machine\\Software\\Wow6432Node\\Macromedia\\FlashPlayer",
        KEY_QUERY_VALUE);
#else
    RegistryKey flashKey = RegistryKey::Open(
        "\\Registry\\Machine\\Software\\Macromedia\\FlashPlayer",
        KEY_QUERY_VALUE);
#endif

    write(log, " Flash: ");
    if (flashPluginKey.Valid())
    {
        try
        {
            std::string flashVer(flashPluginKey["Version"].GetStringStrict());
            write(log, flashVer);
            return;
        }
        catch (SystemFacades::ErrorFileNotFoundException const&)
        {
            // No output if this value isn't found.
        }
    }

    if (flashKey.Valid())
    {
        try
        {
            std::string flashVer(flashKey["CurrentVersion"].GetStringStrict());
            std::transform(flashVer.begin(),
                           flashVer.end(),
                           flashVer.begin(),
                           [](char x) { return x == ',' ? '.' : x; });
            write(log, flashVer);
            return;
        }
        catch (SystemFacades::ErrorFileNotFoundException const&)
        {
            // No output if this value wasn't found either.
        }
    }

    // Base case, does not appear to be installed (at least not correctly).
    write(log, "Not Installed");
}

using SystemFacades::RegistryValueAndData;

static std::vector<RegistryValueAndData>::const_iterator find_registry_value(std::vector<RegistryValueAndData> const& values, char const* const valueName)
{
    return std::find_if(values.cbegin(), values.cend(), [valueName](RegistryValueAndData const& data) {
        return data.GetName() == valueName;
    });
}

static void WriteIeData(log_sink& log)
{
    RegistryKey ieKey = RegistryKey::Open(
        "\\Registry\\Machine\\Software\\Microsoft\\Internet Explorer",
        KEY_QUERY_VALUE);
    if (!ieKey.Valid())
    {
        write(log, "IE ERROR!");
        return;
    }

    std::string actualVersion;
    try
    {
        actualVersion = ieKey["svcVersion"].GetStringStrict();
    }
    catch (SystemFacades::ErrorFileNotFoundException const&)
    {
        actualVersion = ieKey["Version"].GetStringStrict();
    }
    
    write(log, "IE: ", actualVersion);
 }

static void WriteJavaData(log_sink& log)
{
    RegistryKey javaKey = RegistryKey::Open(
        "\\Registry\\Machine\\Software\\JavaSoft\\Java Runtime Environment",
        KEY_QUERY_VALUE);
    if (javaKey.Valid())
    {
        auto const valueNames = javaKey.EnumerateValues();
        auto const java7FamilyVersion = find_registry_value(valueNames, "Java7FamilyVersion");
        if (java7FamilyVersion != valueNames.cend())
        {
            write(log, " Java: ", java7FamilyVersion->GetStringStrict().substr(2));
            return;
        }

        auto const browserJavaVersion = find_registry_value(valueNames, "BrowserJavaVersion");
        if (browserJavaVersion != valueNames.cend())
        {
            write(log, " Java: ", browserJavaVersion->GetStringStrict());
            return;
        }
    }

    write(log, " Java: Not Installed");
}

void WriteScriptHeader(log_sink& log, std::uint64_t startTime)
{
    writeln(log, "Instalog ", BOOST_STRINGIZE(INSTALOG_VERSION));
    switch (GetSystemMetrics(SM_CLEANBOOT))
    {
    case 1:
        writeln(log, " MINIMAL");
        break;
    case 2:
        writeln(log, " NETWORK");
        break;
    }
    write(log, "Run By ");
    wchar_t userName[257]; // UNLEN + 1
    DWORD userNameLength = 257;
    GetUserName(userName, &userNameLength);
    write(log, std::wstring(userName, userNameLength - 1), " on ");

    auto timeZoneBias = (-GetTimeZoneBias() - 60);
    long displayBiasHour = timeZoneBias / 60;
    long displayBiasMinutes = timeZoneBias % 60;

    WriteMillisecondDateFormat(log, startTime);
    writeln (log, " [GMT ", displayBiasHour >= 0 ? "+" : "", displayBiasHour, ':', pad(2, '0', displayBiasMinutes), ']');

    WriteIeData(log);
    WriteJavaData(log);
    WriteFlashData(log);

    try
    {
        std::string adobeVersion(GetAdobeReaderVersion());
        write(log, " Adobe: ", adobeVersion);
    }
    catch (std::exception const&)
    {
    } // No log output on failure.

    writeln(log);
    WriteOsVersion(log);
    write(log, ' ');
    WriteMemoryInformation(log);
    writeln(log);
}

void WriteScriptFooter(log_sink& log, std::uint64_t startTime)
{
    auto endTime = Instalog::GetLocalTime();
    auto duration = endTime - startTime;
    auto seconds = duration / 10000000ull;
    auto milliseconds = (duration / 10000ull) - (seconds * 1000);
    write(log, "Instalog ", BOOST_STRINGIZE(INSTALOG_VERSION), " finished at ");
    WriteMillisecondDateFormat(log, endTime);
    writeln(log, " (Generation took ", seconds, '.', pad(4, '0', milliseconds), " seconds)");
}
}
