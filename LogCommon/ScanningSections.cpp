// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <vector>
#include <boost/algorithm/string/case_conv.hpp>
#include "resource.h"
#include "Process.hpp"
#include "ServiceControlManager.hpp"
#include "Path.hpp"
#include "Whitelist.hpp"
#include "ScopedPrivilege.hpp"
#include "Win32Exception.hpp"
#include "StockOutputFormats.hpp"
#include "EventLog.hpp"
#include "Win32Glue.hpp"
#include "RestorePoints.hpp"
#include "Wmi.hpp"
#include "Registry.hpp"
#include "File.hpp"
#include "Path.hpp"
#include "ScanningSections.hpp"

namespace Instalog
{
void RunningProcesses::Execute(std::wostream& logOutput,
                               ScriptSection const&,
                               std::vector<std::wstring> const&) const
{
    using Instalog::SystemFacades::ProcessEnumerator;
    using Instalog::SystemFacades::ErrorAccessDeniedException;
    using Instalog::SystemFacades::ScopedPrivilege;

    std::vector<std::wstring> fullPrintList;
    std::wstring winDir = Path::GetWindowsPath();
    fullPrintList.push_back(Path::Append(winDir, L"System32\\Svchost.exe"));
    fullPrintList.push_back(Path::Append(winDir, L"System32\\Svchost"));
    fullPrintList.push_back(Path::Append(winDir, L"System32\\Rundll32.exe"));
    fullPrintList.push_back(Path::Append(winDir, L"Syswow64\\Rundll32.exe"));
    boost::algorithm::to_lower(winDir);
    std::vector<std::pair<std::wstring, std::wstring>> replacements;
    replacements.emplace_back(
        std::pair<std::wstring, std::wstring>(L"c:\\windows\\", winDir));
    Whitelist w(IDR_RUNNINGPROCESSESWHITELIST, replacements);

    ScopedPrivilege privilegeHolder(SE_DEBUG_NAME);
    ProcessEnumerator enumerator;
    for (ProcessEnumerator::iterator it = enumerator.begin();
         it != enumerator.end();
         ++it)
    {
        auto executableTry = it->GetExecutablePath();
        if (executableTry.is_valid())
        {
            auto executable = executableTry.get();
            if (boost::starts_with(executable, L"\\??\\"))
            {
                executable.erase(executable.begin(), executable.begin() + 4);
            }
            if (w.IsOnWhitelist(executable))
            {
                continue;
            }
            Path::Prettify(executable.begin(), executable.end());
            std::wstring pathElement;
            auto equTest = [&](std::wstring const & str)->bool
            {
                return boost::iequals(executable, str, std::locale());
            }
            ;
            if (std::find_if(fullPrintList.begin(),
                             fullPrintList.end(),
                             equTest) == fullPrintList.end())
            {
                pathElement = std::move(executable);
            }
            else
            {
                auto commandTry = it->GetCmdLine();
                if (commandTry.is_valid())
                {
                    pathElement = std::move(commandTry.get());
                }
                else
                {
                    pathElement = std::move(executable);
                }
            }
            GeneralEscape(pathElement);
            logOutput << std::move(pathElement) << L"\n";
        }
        else
        {
            logOutput << L"Could not open process PID=" << it->GetProcessId()
                      << L'\n';
        }
    }
}

static bool IsOnServicesWhitelist(Instalog::SystemFacades::Service const& svc)
{
    static Whitelist wht(IDR_SERVICESDRIVERSWHITELIST);
    if (svc.IsDamagedSvchost())
    {
        return false;
    }
    std::wstring whitelistcheck;
    whitelistcheck.reserve(
        3 + svc.GetSvchostGroup().size() + svc.GetFilepath().size() +
        svc.GetServiceName().size() + svc.GetDisplayName().size());
    whitelistcheck.append(svc.GetSvchostGroup());
    whitelistcheck.push_back(L';');
    whitelistcheck.append(svc.GetFilepath());
    whitelistcheck.push_back(L';');
    whitelistcheck.append(svc.GetServiceName());
    whitelistcheck.push_back(L';');
    whitelistcheck.append(svc.GetDisplayName());
    return wht.IsOnWhitelist(whitelistcheck);
}

void
ServicesDrivers::Execute(std::wostream& logOutput,
                         ScriptSection const& /*sectionData*/,
                         std::vector<std::wstring> const& /*options*/) const
{
    using Instalog::SystemFacades::ServiceControlManager;
    using Instalog::SystemFacades::Service;

    ServiceControlManager scm;
    std::vector<Service> services = scm.GetServices();
    std::vector<std::wstring> serviceStrings;

    for (auto service = services.begin(); service != services.end(); ++service)
    {
        if (IsOnServicesWhitelist(*service))
        {
            continue;
        }

        std::wostringstream currentSvcStream;
        currentSvcStream << service->GetState() << service->GetStart();
        if (service->IsDamagedSvchost())
        {
            currentSvcStream << L'D';
        }
        currentSvcStream << L' ' << service->GetServiceName() << L';'
                         << service->GetDisplayName() << L';';
        auto const& svcHostDll = service->GetSvchostDll();
        if (service->IsSvchostService() && svcHostDll.is_valid())
        {
            currentSvcStream << service->GetSvchostGroup() << L"->";
            WriteDefaultFileOutput(currentSvcStream, svcHostDll.get());
        }
        else
        {
            WriteDefaultFileOutput(currentSvcStream, service->GetFilepath());
        }
        serviceStrings.emplace_back(currentSvcStream.str());
    }

    std::sort(serviceStrings.begin(),
              serviceStrings.end(),
              [](std::wstring const & a, std::wstring const & b) {
        return boost::ilexicographical_compare(a, b);
    });
    std::copy(serviceStrings.cbegin(),
              serviceStrings.cend(),
              std::ostream_iterator<std::wstring, wchar_t>(logOutput, L"\n"));
}

void EventViewer::Execute(std::wostream& logOutput,
                          ScriptSection const& /*sectionData*/,
                          std::vector<std::wstring> const& /*options*/) const
{
    using Instalog::SystemFacades::OldEventLog;
    using Instalog::SystemFacades::XmlEventLog;
    using Instalog::SystemFacades::EventLogEntry;

    // Query the entries
    std::vector<std::unique_ptr<EventLogEntry>> eventLogEntries;
    try
    {
        XmlEventLog xmlEventLog(L"System", L"Event/System[Level=1 or Level=2]");
        eventLogEntries = xmlEventLog.ReadEvents();
    }
    catch (Instalog::SystemFacades::Win32Exception const&)
    {
        OldEventLog eventLog;
        eventLogEntries = eventLog.ReadEvents();
    }

    // Calculate the time a week ago
    SYSTEMTIME currentSystemTime;
    GetSystemTime(&currentSystemTime);
    FILETIME currentFileTime;
    if (SystemTimeToFileTime(&currentSystemTime, &currentFileTime) == false)
    {
        SystemFacades::Win32Exception::ThrowFromLastError();
    }
    ULARGE_INTEGER currentTime;
    currentTime.LowPart = currentFileTime.dwLowDateTime;
    currentTime.HighPart = currentFileTime.dwHighDateTime;
    ULARGE_INTEGER oneWeekAgo;
    oneWeekAgo.QuadPart = currentTime.QuadPart - 6048000000000ll;

    // Log applicable events
    for (auto eventLogEntry = eventLogEntries.begin();
         eventLogEntry != eventLogEntries.end();
         ++eventLogEntry)
    {
        auto const level = (*eventLogEntry)->level;
        // Whitelist everything but "Critical" and "Error" messages
        if (level != EventLogEntry::EvtLevelCritical &&
            level != EventLogEntry::EvtLevelError)
            continue;

        // Whitelist all events that are older than this week
        ULARGE_INTEGER date;
        date.LowPart = (*eventLogEntry)->date.dwLowDateTime;
        date.HighPart = (*eventLogEntry)->date.dwHighDateTime;
        if (date.QuadPart < oneWeekAgo.QuadPart)
            continue;

        // Whitelist EventIDs 1000, 8023, 10010
        auto eventId = (*eventLogEntry)->eventId;
        if (eventId == 1000 || eventId == 8023 || eventId == 10010)
            continue;

        // Print the Date
        WriteDefaultDateFormat(logOutput,
                               FiletimeToInteger((*eventLogEntry)->date));

        // Print the Type
        switch ((*eventLogEntry)->level)
        {
        case EventLogEntry::EvtLevelCritical:
            logOutput << L", Critical: ";
            break;
        case EventLogEntry::EvtLevelError:
            logOutput << L", Error: ";
            break;
        }

        // Print the Source
        auto const& source = (*eventLogEntry)->GetSource();
        logOutput << source << L" [";

        // Print the EventID
        logOutput << eventId << L"] ";

        // Print the description
        std::wstring description = (*eventLogEntry)->GetDescription();
        GeneralEscape(description);
        if (boost::algorithm::ends_with(description, "#r#n"))
        {
            description.erase(description.end() - 4, description.end());
        }
        logOutput << description << L'\n';
    }
}

void MachineSpecifications::OperatingSystem(std::wostream& logOutput) const
{
    using namespace SystemFacades;

    UniqueComPtr<IWbemServices> wbemServices(GetWbemServices());

    UniqueComPtr<IWbemServices> namespaceCimv2;
    ThrowIfFailed(wbemServices->OpenNamespace(
        BSTR(L"cimv2"), 0, NULL, namespaceCimv2.PassAsOutParameter(), NULL));

    UniqueComPtr<IEnumWbemClassObject> enumWin32_OperatingSystem;
    ThrowIfFailed(namespaceCimv2->CreateInstanceEnum(
        BSTR(L"Win32_OperatingSystem"),
        WBEM_FLAG_FORWARD_ONLY,
        NULL,
        enumWin32_OperatingSystem.PassAsOutParameter()));

    HRESULT hr;
    ULONG returnCount = 0;
    UniqueComPtr<IWbemClassObject> response;
    UniqueVariant variant;

    hr = enumWin32_OperatingSystem->Next(
        WBEM_INFINITE, 1, response.PassAsOutParameter(), &returnCount);
    if (hr == WBEM_S_FALSE)
    {
        throw std::runtime_error("Unexpected number of returned classes.");
    }
    else if (FAILED(hr))
    {
        ThrowFromHResult(hr);
    }
    else if (returnCount == 0)
    {
        throw std::runtime_error("Unexpected number of returned classes.");
    }

    ThrowIfFailed(response->Get(
        L"SystemDrive", 0, variant.PassAsOutParameter(), NULL, NULL));
    logOutput << L"Boot Device: " << variant.AsString() << L'\n';

    ThrowIfFailed(response->Get(
        L"InstallDate", 0, variant.PassAsOutParameter(), NULL, NULL));
    logOutput << L"Install Date: ";
    WriteMillisecondDateFormat(
        logOutput,
        FiletimeToInteger(WmiDateStringToFiletime(variant.AsString())));
    logOutput << L'\n';
}

struct SystemTimeInformation
{   // MSDN says "reserved [48]"
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ms724509.aspx
    // This came from ReactOS:
    // http://doxygen.reactos.org/da/d46/structSYSTEM__TIMEOFDAY__INFORMATION.html
    std::int64_t bootTime;       // 8
    std::int64_t currentTime;    // 16
    std::int64_t timeZoneBias;   // 24
    std::int32_t timeZoneId;     // 28
    std::int32_t reserved;       // 32
    std::uint64_t bootTimeBias;  // 40
    std::uint64_t sleepTimeBias; // 48
};

void MachineSpecifications::PerfFormattedData_PerfOS_System(
    std::wostream& logOutput) const
{
    NtQuerySystemInformationFunc ntQuerySysInfo =
        SystemFacades::GetNtDll().GetProcAddress<NtQuerySystemInformationFunc>(
            "NtQuerySystemInformation");

    const std::uint64_t ticksPerDay = 10000000ull * 60ull * 60ull * 24ull;
    const std::uint64_t ticksPerHour = 10000000ull * 60ull * 60ull;
    const std::uint64_t ticksPerMinute = 10000000ull * 60ull;
    SystemTimeInformation timeData;
    NTSTATUS errorCheck = ntQuerySysInfo(SystemTimeOfDayInformation,
                                         reinterpret_cast<void*>(&timeData),
                                         sizeof(timeData),
                                         0);
    if (errorCheck != 0)
    {
        SystemFacades::Win32Exception::ThrowFromNtError(errorCheck);
    }
    uint64_t uptime = timeData.currentTime - timeData.bootTime;
    logOutput << L"Booted at: ";
    WriteDefaultDateFormat(
        logOutput, timeData.bootTime - (GetTimeZoneBias() * ticksPerMinute));
    logOutput << L" (Up ";
    if (uptime > ticksPerDay)
    {
        logOutput << uptime / ticksPerDay << L" Days ";
        uptime = uptime % ticksPerDay;
    }
    if (uptime > ticksPerHour)
    {
        logOutput << uptime / ticksPerHour << L" Hours ";
        uptime = uptime % ticksPerHour;
    }
    logOutput << uptime / ticksPerMinute << L" Minutes)\n";
}

void MachineSpecifications::Execute(
    std::wostream& logOutput,
    ScriptSection const& /*sectionData*/,
    std::vector<std::wstring> const& /*options*/) const
{
    OperatingSystem(logOutput);
    PerfFormattedData_PerfOS_System(logOutput);
    BaseBoard(logOutput);
    Processor(logOutput);
    LogicalDisk(logOutput);
}

void MachineSpecifications::BaseBoard(std::wostream& logOutput) const
{
    using namespace SystemFacades;

    UniqueComPtr<IWbemServices> wbemServices(GetWbemServices());

    UniqueComPtr<IWbemServices> namespaceCimv2;
    ThrowIfFailed(wbemServices->OpenNamespace(
        BSTR(L"cimv2"), 0, NULL, namespaceCimv2.PassAsOutParameter(), NULL));

    UniqueComPtr<IEnumWbemClassObject> enumWin32_BaseBoard;
    ThrowIfFailed(namespaceCimv2->CreateInstanceEnum(
        BSTR(L"Win32_BaseBoard"),
        WBEM_FLAG_FORWARD_ONLY,
        NULL,
        enumWin32_BaseBoard.PassAsOutParameter()));

    HRESULT hr;
    ULONG returnCount = 0;
    UniqueComPtr<IWbemClassObject> response;
    UniqueVariant variant;

    hr = enumWin32_BaseBoard->Next(
        WBEM_INFINITE, 1, response.PassAsOutParameter(), &returnCount);
    if (hr == WBEM_S_FALSE)
    {
        throw std::runtime_error("Unexpected number of returned classes.");
    }
    else if (FAILED(hr))
    {
        ThrowFromHResult(hr);
    }
    else if (returnCount == 0)
    {
        throw std::runtime_error("Unexpected number of returned classes.");
    }

    ThrowIfFailed(response->Get(
        L"Manufacturer", 0, variant.PassAsOutParameter(), NULL, NULL));
    logOutput << L"Motherboard: " << variant.AsString();
    ThrowIfFailed(
        response->Get(L"Product", 0, variant.PassAsOutParameter(), NULL, NULL));
    logOutput << L" " << variant.AsString() << L'\n';
}

void MachineSpecifications::Processor(std::wostream& logOutput) const
{
    using namespace SystemFacades;

    UniqueComPtr<IWbemServices> wbemServices(GetWbemServices());

    UniqueComPtr<IWbemServices> namespaceCimv2;
    ThrowIfFailed(wbemServices->OpenNamespace(
        BSTR(L"cimv2"), 0, NULL, namespaceCimv2.PassAsOutParameter(), NULL));

    UniqueComPtr<IEnumWbemClassObject> enumWin32_Processor;
    ThrowIfFailed(namespaceCimv2->CreateInstanceEnum(
        BSTR(L"Win32_Processor"),
        WBEM_FLAG_FORWARD_ONLY,
        NULL,
        enumWin32_Processor.PassAsOutParameter()));

    HRESULT hr;
    ULONG returnCount = 0;
    UniqueComPtr<IWbemClassObject> response;
    UniqueVariant variant;

    hr = enumWin32_Processor->Next(
        WBEM_INFINITE, 1, response.PassAsOutParameter(), &returnCount);
    if (hr == WBEM_S_FALSE)
    {
        throw std::runtime_error("Unexpected number of returned classes.");
    }
    else if (FAILED(hr))
    {
        ThrowFromHResult(hr);
    }
    else if (returnCount == 0)
    {
        throw std::runtime_error("Unexpected number of returned classes.");
    }

    ThrowIfFailed(
        response->Get(L"Name", 0, variant.PassAsOutParameter(), NULL, NULL));
    logOutput << L"Processor: " << variant.AsString() << L'\n';
}

void MachineSpecifications::LogicalDisk(std::wostream& logOutput) const
{
    using namespace SystemFacades;

    UniqueComPtr<IWbemServices> wbemServices(GetWbemServices());

    UniqueComPtr<IWbemServices> namespaceCimv2;
    ThrowIfFailed(wbemServices->OpenNamespace(
        BSTR(L"cimv2"), 0, NULL, namespaceCimv2.PassAsOutParameter(), NULL));

    UniqueComPtr<IEnumWbemClassObject> enumWin32_LogicalDisk;
    ThrowIfFailed(namespaceCimv2->CreateInstanceEnum(
        BSTR(L"Win32_LogicalDisk"),
        WBEM_FLAG_FORWARD_ONLY,
        NULL,
        enumWin32_LogicalDisk.PassAsOutParameter()));

    for (;;)
    {
        HRESULT hr;
        ULONG returnCount = 0;
        UniqueComPtr<IWbemClassObject> response;
        UniqueVariant variant;

        hr = enumWin32_LogicalDisk->Next(
            WBEM_INFINITE, 1, response.PassAsOutParameter(), &returnCount);
        if (hr == WBEM_S_FALSE)
        {
            break;
        }
        else if (FAILED(hr))
        {
            ThrowFromHResult(hr);
        }
        else if (returnCount == 0)
        {
            throw std::runtime_error("Unexpected number of returned classes.");
        }

        ThrowIfFailed(response->Get(
            L"DeviceID", 0, variant.PassAsOutParameter(), NULL, NULL));
        logOutput << variant.AsString() << L" is ";

        ThrowIfFailed(response->Get(
            L"DriveType", 0, variant.PassAsOutParameter(), NULL, NULL));
        switch (variant.AsUlong())
        {
        case 0:
            logOutput << L"UNKNOWN";
            break;
        case 1:
            logOutput << L"NOROOT";
            break;
        case 2:
            logOutput << L"REMOVABLE";
            break;
        case 3:
            logOutput << L"LOCAL";
            break;
        case 4:
            logOutput << L"NETWORK";
            break;
        case 5:
            logOutput << L"CDROM";
            break;
        case 6:
            logOutput << L"RAM";
            break;
        default:
            assert(false);
        }

        ThrowIfFailed(response->Get(
            L"Size", 0, variant.PassAsOutParameter(), NULL, NULL));

        if (variant.IsNull())
        {
            logOutput << L'\n';
        }
        else
        {
            ULONGLONG totalSize = variant.AsUlonglong();
            ThrowIfFailed(response->Get(
                L"FreeSpace", 0, variant.PassAsOutParameter(), NULL, NULL));
            ULONGLONG freeSpace = variant.AsUlonglong();

            totalSize /= 1073741824;
            freeSpace /= 1073741824;

            logOutput << L" - " << totalSize << L" GiB total, " << freeSpace
                      << L" GiB free\n";
        }
    }
}

void RestorePoints::Execute(std::wostream& logOutput,
                            ScriptSection const& /*sectionData*/,
                            std::vector<std::wstring> const& /*options*/) const
{
    try
    {
        std::vector<SystemFacades::RestorePoint> restorePoints =
            SystemFacades::EnumerateRestorePoints();

        for (auto const& restorePoint : restorePoints)
        {
            logOutput << restorePoint.SequenceNumber << L" ";
            WriteMillisecondDateFormat(
                logOutput,
                FiletimeToInteger(SystemFacades::WmiDateStringToFiletime(
                    restorePoint.CreationTime)));
            logOutput << L" " << restorePoint.Description << L'\n';
        }
    }
    catch (SystemFacades::HresultException const& ex)
    {
        logOutput << L"(Failed to enumerate restore points; HRESULT="
                  << std::hex << ex.GetErrorCode() << L")\n";
    }
}

void
InstalledPrograms::Execute(std::wostream& logOutput,
                           ScriptSection const& /*sectionData*/,
                           std::vector<std::wstring> const& /*options*/) const
{
    Enumerate(
        logOutput,
        L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
#ifdef _M_X64
    Enumerate(
        logOutput,
        L"\\Registry\\Machine\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
#endif
}

void InstalledPrograms::Enumerate(std::wostream& logOutput,
                                  std::wstring const& rootKeyPath) const
{
    using namespace SystemFacades;

    RegistryKey rootKey(RegistryKey::Open(rootKeyPath));
    if (rootKey.Invalid())
    {
        Win32Exception::ThrowFromNtError(::GetLastError());
    }

    std::vector<RegistryKey> uninstallKeys(rootKey.EnumerateSubKeys());
    std::vector<std::wstring> entries;

    for (auto uninstallKey = uninstallKeys.begin();
         uninstallKey != uninstallKeys.end();
         ++uninstallKey)
    {
        std::wstring currentEntry;
        try
        {
            uninstallKey->GetValue(L"ParentKeyName");
            continue;
        }
        catch (ErrorFileNotFoundException const&)
        {
            // Expected behavior
        }
        try
        {
            if (uninstallKey->GetValue(L"SystemComponent").GetDWord() == 1)
            {
                continue;
            }
        }
        catch (ErrorFileNotFoundException const&)
        {
            // Expected behavior
        }
        catch (InvalidRegistryDataTypeException const&)
        {
            // Expected behavior
        }
        try
        {
            RegistryValue displayName(uninstallKey->GetValue(L"DisplayName"));
            currentEntry = displayName.GetString();
            GeneralEscape(currentEntry);
        }
        catch (ErrorFileNotFoundException const&)
        {
            continue;
        }

        try
        {
            RegistryValue versionMajor(uninstallKey->GetValue(L"VersionMajor"));
            RegistryValue versionMinor(uninstallKey->GetValue(L"VersionMinor"));

            currentEntry += L" (version ";
            if (versionMajor.GetType() == REG_DWORD)
            {
                currentEntry +=
                    boost::lexical_cast<std::wstring>(versionMajor.GetDWord());
            }
            else
            {
                currentEntry += versionMajor.GetString();
            }
            currentEntry.push_back(L'.');
            if (versionMinor.GetType() == REG_DWORD)
            {
                currentEntry +=
                    boost::lexical_cast<std::wstring>(versionMinor.GetDWord());
            }
            else
            {
                currentEntry += versionMinor.GetString();
            }
            currentEntry += L")\n";
        }
        catch (ErrorFileNotFoundException const&)
        {
            currentEntry.push_back(L'\n');
        }
        entries.push_back(std::move(currentEntry));
    }
    std::sort(entries.begin(),
              entries.end(),
              [](std::wstring const & a, std::wstring const & b) {
        return boost::ilexicographical_compare(a, b);
    });
    std::copy(entries.cbegin(),
              entries.cend(),
              std::ostream_iterator<std::wstring, wchar_t>(logOutput, L""));
}

/// @brief    Sort SystemFacades::FindFilesRecord structs.
///
/// @param    d1    The first const SystemFacades::FindFilesRecord;
/// @param    d2    The second const SystemFacades::FindFilesRecord;
///
/// @return    true if it succeeds, false if it fails.
///
/// @detail The output will then be sorted by creation date, then by
/// modification date, then by
///         size, then by attribute string, and finally by file path.
bool SortWin32FindDataW(const SystemFacades::FindFilesRecord& d1,
                        const SystemFacades::FindFilesRecord& d2)
{
    using SystemFacades::File;

    if (d1.GetCreationTime() != d2.GetCreationTime())
    {
        return d1.GetCreationTime() > d2.GetCreationTime();
    }

    if (d1.GetLastWriteTime() != d2.GetLastWriteTime())
    {
        return d1.GetLastWriteTime() > d2.GetLastWriteTime();
    }

    if (d1.GetSize() != d2.GetSize())
    {
        return d1.GetSize() > d2.GetSize();
    }

    std::wstringstream attributes1ss, attributes2ss;
    WriteFileAttributes(attributes1ss, d1.GetAttributes());
    WriteFileAttributes(attributes2ss, d2.GetAttributes());
    std::wstring attributes1(attributes1ss.str());
    std::wstring attributes2(attributes2ss.str());
    if (attributes1 != attributes2)
    {
        return attributes1 > attributes2;
    }

    return d1.GetFileName() > d2.GetFileName();
}

/// @brief    Gets the number of 100-nanosecond intervals since 1600-whatever
/// that were x months ago
///
/// @param    months    The number of months to go back
///
/// @return    The int representation of a filetime that was that many months
/// ago
std::uint64_t MonthsAgo(int months)
{
    FILETIME fileTime;
    GetSystemTimeAsFileTime(&fileTime);
    std::uint64_t monthsAgo = FiletimeToInteger(fileTime);
    monthsAgo -=
        months * 25920000000000; /* 30 days of 100-nanosecond intervals */
    return monthsAgo;
}

/// @brief    Removes long strings of 12 or more closely created files from a
/// list
///
/// @param [in,out]    fileData    File data of interest
void
RemoveWindowsUpdateRuns(std::vector<SystemFacades::FindFilesRecord>& fileData)
{
    using SystemFacades::FindFilesRecord;
    // Remove "runs" of files
    if (fileData.size() < 12)
    {
        return;
    }

    auto first = fileData.cbegin();
    while (first != fileData.cend())
    {
        // Find the end of a run -- which is when the difference between an item
        // and the next is
        // greater than one second.
        auto runEnd = std::adjacent_find(
            first,
            fileData.cend(),
            [](FindFilesRecord const & lhs, FindFilesRecord const & rhs) {
                return lhs.GetCreationTime() - rhs.GetCreationTime() > 10000000;
            });

        // Get the iterator to the second adjacent element when deciding to
        // remove or not.
        if (runEnd != fileData.end())
        {
            ++runEnd;
        }

        // If more than 12 files in this run, remove them from the results.
        if (std::distance(first, runEnd) >= 12)
        {
            runEnd = fileData.erase(first, runEnd);
        }

        if (runEnd == fileData.cend())
        {
            first = runEnd;
        }
        else
        {
            first = ++runEnd;
        }
    }
}

/// @brief    Gets the CreatedLast30 file data
///
/// @return    The CreatedLast30 file data.
std::vector<SystemFacades::FindFilesRecord> GetCreatedLast30FileData()
{
    using SystemFacades::FindFiles;

    static const wchar_t* directories[] = {
        L"%SystemRoot%\\System32\\drivers\\", L"%SystemRoot%\\System32\\wbem\\",
        L"%SystemRoot%\\System32\\",          L"%SystemRoot%\\system\\",
        L"%SystemRoot%\\",                    L"%Systemdrive%\\",
        L"%Systemdrive%\\temp\\",             L"%userprofile%\\",
        L"%commonprogramfiles%\\",            L"%programfiles%\\",
        L"%AppData%\\",                       L"%AllUsersprofile%\\",
#ifdef _M_X64
        L"%SystemRoot%\\SysWow64\\",          L"%ProgramFiles(x86)%\\",
        L"%CommonProgramFiles(x86)%\\"
#endif
    };

    std::uint64_t oneMonthAgo = MonthsAgo(1);

    std::vector<SystemFacades::FindFilesRecord> fileData;

    for (size_t i = 0; i < sizeof(directories) / sizeof(const wchar_t*); ++i)
    {
        std::wstring fullDirectory(directories[i]);
        fullDirectory = Path::ExpandEnvStrings(fullDirectory);

        for (FindFiles files(fullDirectory); files.NextSuccess();)
        {
            auto const& data = files.GetRecord();
            std::uint64_t createdTime = data.GetCreationTime();
            if (createdTime >= oneMonthAgo)
            {
                fileData.emplace_back(data);
            }
        }
    }

    std::sort(fileData.begin(), fileData.end(), SortWin32FindDataW);

    RemoveWindowsUpdateRuns(fileData);

    return fileData;
}

/// @brief    Checks if the path has one of the given extensions
///
/// @param    path             Full pathname of the file.
/// @param    extensions       The extensions to check for
/// @param    numextensions    The number of extensions in the extensions array
///
/// @return    true if a matching extension is found, false otherwise
template <typename StringType>
bool ExtensionCheck(StringType&& path,
                    const wchar_t** extensions,
                    size_t numextensions)
{
    std::locale loc;
    for (size_t i = 0; i < numextensions; ++i)
    {
        if (boost::iends_with(
                std::forward<StringType>(path), extensions[i], loc))
        {
            return true;
        }
    }

    return false;
}

/// @brief    Gets a Find3M file data.
///
/// @param    createdLast30FileData    The createdLast30 data.  If this is
/// empty, it will be
///                                 assumed that CreatedLast30 wasn't run and
/// therefore the
///                                 results should not be stripped from the log.
///
/// @return    The Find3M file data.
std::vector<SystemFacades::FindFilesRecord> GetFind3MFileData(
    std::vector<SystemFacades::FindFilesRecord>& createdLast30FileData)
{
    using SystemFacades::FindFiles;
    using SystemFacades::FindFilesOptions;
    using SystemFacades::File;

    std::vector<SystemFacades::FindFilesRecord> fileData;

    std::uint64_t threeMonthsAgo = MonthsAgo(3);

    // The first part of list1 is generated the same as list5
    static const wchar_t* extensions_list15[] = {
        L"bat", L"reg", L"vbs", L"wsf", L"vbe", L"msi", L"msp", L"com", L"pif",
        L"ren", L"vir", L"tmp", L"dll", L"scr", L"sys", L"exe", L"bin", L"drv"};
    static const wchar_t* directories_list1a[] = {
        L"%PROGRAMFILES%\\",      L"%COMMONPROGRAMFILES%\\",
#ifdef _M_X64
        L"%PROGRAMFILES(x86)%\\", L"%COMMONPROGRAMFILES(x86)%\\",
#endif
    };
    for (size_t i = 0; i < sizeof(directories_list1a) / sizeof(const wchar_t*);
         ++i)
    {
        std::wstring fullDirectory(directories_list1a[i]);
        fullDirectory = Path::ExpandEnvStrings(std::move(fullDirectory));

        for (FindFiles files(fullDirectory); files.NextSuccess();)
        {
            auto const& curRecord = files.GetRecord();
            // Discard entries that do not have the proper extension
            if (ExtensionCheck(curRecord.GetFileName(),
                               extensions_list15,
                               sizeof(extensions_list15) /
                                   sizeof(const wchar_t*)) == false)
            {
                continue;
            }

            // Discard entries that are not executables
            if (File::IsExecutable(curRecord.GetFileName()) == false)
            {
                continue;
            }

            fileData.emplace_back(curRecord);
        }
    }

    // The second part of list1 also has 3M filtering
    static const wchar_t* directories_list1b[] = {
        L"%APPDATA%\\",              L"%SYSTEMDRIVE%\\", L"%SYSTEMROOT%\\",
        L"%SYSTEMROOT%\\system32\\", L"%USERPROFILE%\\", L"%ALLUSERSPROFILE%\\",
#ifdef _M_X64
        L"%SYSTEMROOT%\\Syswow64\\",
#endif
    };
    for (size_t i = 0; i < sizeof(directories_list1b) / sizeof(const wchar_t*);
         ++i)
    {
        std::wstring fullDirectory(directories_list1b[i]);
        fullDirectory = Path::ExpandEnvStrings(std::move(fullDirectory));

        for (FindFiles files(fullDirectory); files.NextSuccess();)
        {
            auto const& curRecord = files.GetRecord();
            if (curRecord.GetCreationTime() >= threeMonthsAgo)
            {
                // Discard entries that do not have the proper extension
                if (ExtensionCheck(curRecord.GetFileName(),
                                   extensions_list15,
                                   sizeof(extensions_list15) /
                                       sizeof(const wchar_t*)) == false)
                {
                    continue;
                }

                // Discard entries that are not executable
                if (File::IsExecutable(curRecord.GetFileName()) == false)
                {
                    continue;
                }

                fileData.emplace_back(curRecord);
            }
        }
    }

    // list5 is basically list1b (3M filtering) but also has recursive
    static const wchar_t* directories_list5[] = {
        L"%SYSTEMROOT%\\java\\",          L"%SYSTEMROOT%\\msapps\\",
        L"%SYSTEMROOT%\\pif\\",           L"%SYSTEMROOT%\\Registration\\",
        L"%SYSTEMROOT%\\help\\",          L"%SYSTEMROOT%\\web\\",
        L"%SYSTEMROOT%\\pchealth\\",      L"%SYSTEMROOT%\\srchasst\\",
        L"%SYSTEMROOT%\\tasks\\",         L"%SYSTEMROOT%\\apppatch\\",
        L"%SYSTEMROOT%\\Internet Logs\\", L"%SYSTEMROOT%\\Media\\",
        L"%SYSTEMROOT%\\prefetch\\",      L"%SYSTEMROOT%\\cursors\\",
        L"%SYSTEMROOT%\\inf\\", };
    for (size_t i = 0; i < sizeof(directories_list5) / sizeof(const wchar_t*);
         ++i)
    {
        std::wstring fullDirectory(directories_list5[i]);
        fullDirectory = Path::ExpandEnvStrings(fullDirectory);

        // Recursive
        for (FindFiles files(fullDirectory, FindFilesOptions::RecursiveSearch);
             files.NextSuccess();)
        {
            auto const& curRecord = files.GetRecord();
            if (curRecord.GetCreationTime() >= threeMonthsAgo)
            {
                // Discard entries that do not have the proper extension
                if (ExtensionCheck(curRecord.GetFileName(),
                                   extensions_list15,
                                   sizeof(extensions_list15) /
                                       sizeof(const wchar_t*)) == false)
                {
                    continue;
                }

                // Discard entries that are not executable
                if (File::IsExecutable(curRecord.GetFileName()) == false)
                {
                    continue;
                }

                fileData.emplace_back(curRecord);
            }
        }
    }

    static const wchar_t* directories_list2[] = {
        L"%SYSTEMROOT%\\System\\",
        L"%SYSTEMROOT%\\System32\\Wbem\\",
        L"%SYSTEMROOT%\\System32\\GroupPolicy\\Machine\\Scripts\\Shutdown\\",
        L"%SYSTEMROOT%\\System32\\GroupPolicy\\User\\Scripts\\Logoff\\",
#ifdef _M_X64
        L"%SYSTEMROOT%\\Syswow64\\Drivers\\",
        L"%SYSTEMROOT%\\Syswow64\\Wbem\\",
#endif
    };
    static const wchar_t* extensions_list2_notExecutable[] = {
        L"com", L"pif", L"ren", L"vir", L"tmp", L"dll",
        L"scr", L"sys", L"exe", L"bin", L"dat", L"drv"};
    static const wchar_t* extensions_list2_notDirectory[] = {
        L"bat", L"cmd", L"reg", L"vbs", L"wsf", L"vbe", L"msi", L"msp"};
    for (size_t i = 0; i < sizeof(directories_list2) / sizeof(const wchar_t*);
         ++i)
    {
        std::wstring fullDirectory(directories_list2[i]);
        fullDirectory = Path::ExpandEnvStrings(std::move(fullDirectory));

        // List2 is recursive
        for (FindFiles files(fullDirectory, FindFilesOptions::RecursiveSearch);
             files.NextSuccess();)
        {
            auto const& curRecord = files.GetRecord();
            // Discard entries that are more than three months old
            if (curRecord.GetCreationTime() < threeMonthsAgo)
            {
                continue;
            }

            // Discard entries that have list2_nonExecutable extensions and are
            // not executable
            if (ExtensionCheck(curRecord.GetFileName(),
                               extensions_list2_notExecutable,
                               sizeof(extensions_list2_notExecutable) /
                                   sizeof(const wchar_t*)))
            {
                if (File::IsExecutable(curRecord.GetFileName()) == false)
                {
                    continue;
                }
            }

            // Discard entries that have list2_notDirectory extensions and are
            // not directories
            if (ExtensionCheck(curRecord.GetFileName(),
                               extensions_list2_notDirectory,
                               sizeof(extensions_list2_notDirectory) /
                                   sizeof(const wchar_t*)))
            {
                if ((curRecord.GetAttributes() & FILE_ATTRIBUTE_DIRECTORY) ==
                    false)
                {
                    continue;
                }
            }

            fileData.emplace_back(curRecord);
        }
    }

    std::wstring directory_list3 = Path::ExpandEnvStrings(
        L"%SYSTEMROOT%\\System32\\Spool\\prtprocs\\w32x86\\");
    for (FindFiles files(directory_list3, FindFilesOptions::RecursiveSearch);
         files.NextSuccess();)
    {
        auto const& curRecord = files.GetRecord();
        // Discard non-executables
        if (File::IsExecutable(curRecord.GetFileName()) == false)
        {
            continue;
        }

        fileData.emplace_back(curRecord);
    }

    std::wstring directory_list6 =
        Path::ExpandEnvStrings(L"%SYSTEMROOT%\\Fonts\\");
    static const wchar_t* extensions_list6[] = {L"com", L"pif", L"ren", L"vir",
                                                L"tmp", L"dll", L"scr", L"sys",
                                                L"exe", L"bin", L"dat", L"drv"};
    // List6 is recursive
    for (FindFiles files(directory_list6, FindFilesOptions::RecursiveSearch);
         files.NextSuccess();)
    {
        auto const& curRecord = files.GetRecord();
        // Keep only those with size between 1500 and 2000 bytes
        // or
        // greater than 1500 bytes and executable and with list6 extensions
        if ((curRecord.GetSize() >= 1500 && curRecord.GetSize() <= 2000) ||
            (ExtensionCheck(
                 curRecord.GetFileName(),
                 extensions_list6,
                 sizeof(extensions_list6) / sizeof(const wchar_t*)) &&
             curRecord.GetSize() >= 1500 &&
             File::IsExecutable(curRecord.GetFileName())))
        {
            fileData.emplace_back(curRecord);
        }
    }

    // Sort entries
    std::sort(fileData.begin(), fileData.end(), SortWin32FindDataW);

    // Remove "runs" of files
    RemoveWindowsUpdateRuns(fileData);

    // Remove things from CreatedLast30
    fileData.erase(
        std::remove_if(
            fileData.begin(),
            fileData.end(),
                [&](SystemFacades::FindFilesRecord const & val)->bool {
                return std::binary_search(createdLast30FileData.begin(),
                                          createdLast30FileData.end(),
                                          val,
                                          SortWin32FindDataW);
            }),
        fileData.end());

    return fileData;
}

/// @brief    Logs given FileData
///
/// @param [in,out]    logOutput    The log output stream.
/// @param    fileData             The fileData to output
void PrintFileData(std::wostream& logOutput,
                   std::vector<SystemFacades::FindFilesRecord> const& fileData)
{
    for (size_t i = 0; i < 100 && i < fileData.size(); ++i)
    {
        WriteFileListingFromFindData(logOutput, fileData[i]);
        logOutput << L'\n';
    }

    if (fileData.size() > 100)
    {
        logOutput
            << L"\nToo many files to show.  Most recent 100 files shown above.\n";
    }
}

void FindStarM::Execute(std::wostream& logOutput,
                        ScriptSection const& /*sectionData*/,
                        std::vector<std::wstring> const& /*options*/) const
{
    std::vector<SystemFacades::FindFilesRecord> createdLast30FileData(
        GetCreatedLast30FileData());

    PrintFileData(logOutput, createdLast30FileData);

    std::wstring head(L"Find3M");
    Header(head);
    logOutput << L'\n' << head << L'\n' << L'\n';

    std::vector<SystemFacades::FindFilesRecord> find3MFileData(
        GetFind3MFileData(createdLast30FileData));

    PrintFileData(logOutput, find3MFileData);
}
}
