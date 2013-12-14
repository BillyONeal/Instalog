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
template <typename Container>
static bool list_contains(Container const& c, std::string const& input)
{
    for (auto const& entry : c)
    {
        if (boost::algorithm::iequals(entry, input))
        {
            return true;
        }
    }

    return false;
}

void RunningProcesses::Execute(log_sink& logOutput,
                               ScriptSection const&,
                               std::vector<std::string> const&) const
{
    using Instalog::SystemFacades::ProcessEnumerator;
    using Instalog::SystemFacades::ErrorAccessDeniedException;
    using Instalog::SystemFacades::ScopedPrivilege;

    std::string winDir = Path::GetWindowsPath();
    std::vector<std::string> fullPrintList;
    char const* const fullPrintSources[] = {
        "System32\\Svchost.exe",
        "System32\\Svchost",
        "System32\\Rundll32.exe",
        "Syswow64\\Rundll32.exe"
    };

    for (auto path : fullPrintSources)
    {
        fullPrintList.emplace_back(Path::Append(winDir, path));
    }

    std::vector<std::string> noPrintList;
    char const* const noPrintSources[] = {
        "ntoskrnl.exe",
        "csrss.exe",
        "wininit.exe",
        "services.exe",
        "lsass.exe"
    };

    std::string system32(Path::Append(winDir, "system32"));
    for (auto path : noPrintSources)
    {
        noPrintList.emplace_back(Path::Append(system32, path));
    }

    noPrintList.emplace_back("System Idle Process");
    noPrintList.emplace_back("\\Systemroot\\System32\\smss.exe");

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
            if (boost::starts_with(executable, "\\??\\"))
            {
                executable.erase(executable.begin(), executable.begin() + 4);
            }
            if (list_contains(noPrintList, executable))
            {
                continue;
            }
            Path::Prettify(executable.begin(), executable.end());
            std::string pathElement;
            auto equTest = [&](std::string const & str)->bool
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
            writeln(logOutput, pathElement);
        }
        else
        {
            writeln(logOutput, "Could not open process PID=", it->GetProcessId());
        }
    }
}

void
ServicesDrivers::Execute(log_sink& logOutput,
                         ScriptSection const& /*sectionData*/,
                         std::vector<std::string> const& /*options*/) const
{
    using Instalog::SystemFacades::ServiceControlManager;
    using Instalog::SystemFacades::Service;

    ServiceControlManager scm;
    std::vector<Service> services = scm.GetServices();
    std::vector<std::string> serviceStrings;

    for (auto service = services.begin(); service != services.end(); ++service)
    {
        std::string currentServiceString;
        write(currentServiceString, service->GetState(), service->GetStart());
        if (service->IsDamagedSvchost())
        {
            currentServiceString.push_back('D');
        }

        write(currentServiceString, ' ', service->GetServiceName(), ';', service->GetDisplayName(), ';');
        auto const& svcHostDll = service->GetSvchostDll();
        string_sink tempSink;
        if (service->IsSvchostService() && svcHostDll.is_valid())
        {
            write(currentServiceString, service->GetSvchostGroup(), "->");
            WriteDefaultFileOutput(tempSink, svcHostDll.get());
            currentServiceString.append(tempSink.get());
        }
        else
        {
            WriteDefaultFileOutput(tempSink, service->GetFilepath());
        }
        currentServiceString.append(tempSink.get());
        serviceStrings.emplace_back(std::move(currentServiceString));
    }

    std::sort(serviceStrings.begin(),
              serviceStrings.end(),
              [](std::string const & a, std::string const & b) {
        return boost::ilexicographical_compare(a, b);
    });

    for (auto const& serviceString : serviceStrings)
    {
        writeln(logOutput, serviceString);
    }
}

void EventViewer::Execute(log_sink& logOutput,
                          ScriptSection const& /*sectionData*/,
                          std::vector<std::string> const& /*options*/) const
{
    using Instalog::SystemFacades::OldEventLog;
    using Instalog::SystemFacades::XmlEventLog;
    using Instalog::SystemFacades::EventLogEntry;

    // Query the entries
    std::vector<std::unique_ptr<EventLogEntry>> eventLogEntries;
    try
    {
        XmlEventLog xmlEventLog("System", "Event/System[Level=1 or Level=2]");
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
            write(logOutput, ", Critical: ");
            break;
        case EventLogEntry::EvtLevelError:
            write(logOutput, ", Error: ");
            break;
        }

        // Print the Source
        auto const& source = (*eventLogEntry)->GetSource();
        write(logOutput, source, " [");

        // Print the EventID
        write(logOutput, eventId, "] ");

        // Print the description
        std::string description = (*eventLogEntry)->GetDescription();
        GeneralEscape(description);
        if (boost::algorithm::ends_with(description, "#r#n"))
        {
            description.erase(description.end() - 4, description.end());
        }
        writeln(logOutput, description);
    }
}

void MachineSpecifications::OperatingSystem(log_sink& logOutput) const
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
    writeln(logOutput, "Boot Device: ", variant.AsString());

    ThrowIfFailed(response->Get(
        L"InstallDate", 0, variant.PassAsOutParameter(), NULL, NULL));
    write(logOutput, "Install Date: ");
    WriteMillisecondDateFormat(
        logOutput,
        FiletimeToInteger(WmiDateStringToFiletime(variant.AsString())));
    writeln(logOutput);
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
    log_sink& logOutput) const
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
    write(logOutput, "Booted at: ");
    WriteDefaultDateFormat(
        logOutput, timeData.bootTime - (GetTimeZoneBias() * ticksPerMinute));
    write(logOutput, " (Up ");
    if (uptime > ticksPerDay)
    {
        write(logOutput, uptime / ticksPerDay, " Days ");
        uptime = uptime % ticksPerDay;
    }
    if (uptime > ticksPerHour)
    {
        write(logOutput, uptime / ticksPerHour, " Hours ");
        uptime = uptime % ticksPerHour;
    }
    writeln(logOutput, uptime / ticksPerMinute, " Minutes)");
}

void MachineSpecifications::Execute(
    log_sink& logOutput,
    ScriptSection const& /*sectionData*/,
    std::vector<std::string> const& /*options*/) const
{
    OperatingSystem(logOutput);
    PerfFormattedData_PerfOS_System(logOutput);
    BaseBoard(logOutput);
    Processor(logOutput);
    LogicalDisk(logOutput);
}

void MachineSpecifications::BaseBoard(log_sink& logOutput) const
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
    write(logOutput, "Motherboard: ", variant.AsString());
    ThrowIfFailed(
        response->Get(L"Product", 0, variant.PassAsOutParameter(), NULL, NULL));
    writeln(logOutput, ' ', variant.AsString());
}

void MachineSpecifications::Processor(log_sink& logOutput) const
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
    writeln(logOutput, "Processor: ", variant.AsString());
}

void MachineSpecifications::LogicalDisk(log_sink& logOutput) const
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
        write(logOutput, variant.AsString(), " is ");

        ThrowIfFailed(response->Get(
            L"DriveType", 0, variant.PassAsOutParameter(), NULL, NULL));
        switch (variant.AsUlong())
        {
        case 0:
            write(logOutput, "UNKNOWN");
            break;
        case 1:
            write(logOutput, "NOROOT");
            break;
        case 2:
            write(logOutput, "REMOVABLE");
            break;
        case 3:
            write(logOutput, "LOCAL");
            break;
        case 4:
            write(logOutput, "NETWORK");
            break;
        case 5:
            write(logOutput, "CDROM");
            break;
        case 6:
            write(logOutput, "RAM");
            break;
        default:
            assert(false);
        }

        ThrowIfFailed(response->Get(
            L"Size", 0, variant.PassAsOutParameter(), NULL, NULL));

        if (variant.IsNull())
        {
            writeln(logOutput);
        }
        else
        {
            ULONGLONG totalSize = variant.AsUlonglong();
            ThrowIfFailed(response->Get(
                L"FreeSpace", 0, variant.PassAsOutParameter(), NULL, NULL));
            ULONGLONG freeSpace = variant.AsUlonglong();

            totalSize /= 1073741824;
            freeSpace /= 1073741824;

            writeln(logOutput, " - ", totalSize, " GiB total, ", freeSpace, " GiB free");
        }
    }
}

void RestorePoints::Execute(log_sink& logOutput,
                            ScriptSection const& /*sectionData*/,
                            std::vector<std::string> const& /*options*/) const
{
    try
    {
        std::vector<SystemFacades::RestorePoint> restorePoints =
            SystemFacades::EnumerateRestorePoints();

        for (auto const& restorePoint : restorePoints)
        {
            write(logOutput, restorePoint.SequenceNumber, ' ');
            WriteMillisecondDateFormat(
                logOutput,
                FiletimeToInteger(SystemFacades::WmiDateStringToFiletime(
                    restorePoint.CreationTime)));
            writeln(logOutput, ' ', restorePoint.Description);
        }
    }
    catch (SystemFacades::HresultException const& ex)
    {
        writeln(logOutput, "(Failed to enumerate restore points; HRESULT=", hex(ex.GetErrorCode()), ')');
    }
}

void
InstalledPrograms::Execute(log_sink& logOutput,
                           ScriptSection const& /*sectionData*/,
                           std::vector<std::string> const& /*options*/) const
{
    Enumerate(
        logOutput,
        "\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
#ifdef _M_X64
    Enumerate(
        logOutput,
        "\\Registry\\Machine\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
#endif
}

void InstalledPrograms::Enumerate(log_sink& logOutput,
                                  std::string const& rootKeyPath) const
{
    using namespace SystemFacades;

    RegistryKey rootKey(RegistryKey::Open(rootKeyPath));
    if (rootKey.Invalid())
    {
        Win32Exception::ThrowFromNtError(::GetLastError());
    }

    std::vector<RegistryKey> uninstallKeys(rootKey.EnumerateSubKeys());
    std::vector<std::string> entries;

    for (auto uninstallKey = uninstallKeys.begin();
         uninstallKey != uninstallKeys.end();
         ++uninstallKey)
    {
        std::string currentEntry;
        try
        {
            uninstallKey->GetValue("ParentKeyName");
            continue;
        }
        catch (ErrorFileNotFoundException const&)
        {
            // Expected behavior
        }
        try
        {
            if (uninstallKey->GetValue("SystemComponent").GetDWord() == 1)
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
            RegistryValue displayName(uninstallKey->GetValue("DisplayName"));
            currentEntry = displayName.GetString();
            // A common bug in programs is that they set their display name to end with a null in the registry.
            if (!currentEntry.empty() && currentEntry.back() == '\0')
            {
                currentEntry.pop_back();
            }

            GeneralEscape(currentEntry);
        }
        catch (ErrorFileNotFoundException const&)
        {
            continue;
        }

        try
        {
            RegistryValue versionMajor(uninstallKey->GetValue("VersionMajor"));
            RegistryValue versionMinor(uninstallKey->GetValue("VersionMinor"));

            currentEntry += " (version ";
            if (versionMajor.GetType() == REG_DWORD)
            {
                write(currentEntry, versionMajor.GetDWord());
            }
            else
            {
                currentEntry += versionMajor.GetString();
            }
            currentEntry.push_back('.');
            if (versionMinor.GetType() == REG_DWORD)
            {
                write(currentEntry, versionMinor.GetDWord());
            }
            else
            {
                currentEntry += versionMinor.GetString();
            }
            currentEntry.push_back(')');
        }
        catch (ErrorFileNotFoundException const&)
        {
        }

        entries.emplace_back(std::move(currentEntry));
    }
    std::sort(entries.begin(),
              entries.end(),
              [](std::string const & a, std::string const & b) {
        return boost::ilexicographical_compare(a, b);
    });

    for (auto const& str : entries)
    {
        writeln(logOutput, str);
    }
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

    string_sink attributes1, attributes2;
    WriteFileAttributes(attributes1, d1.GetAttributes());
    WriteFileAttributes(attributes2, d2.GetAttributes());
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

    static const char* directories[] = {
        "%SystemRoot%\\System32\\drivers\\", "%SystemRoot%\\System32\\wbem\\",
        "%SystemRoot%\\System32\\",          "%SystemRoot%\\system\\",
        "%SystemRoot%\\",                    "%Systemdrive%\\",
        "%Systemdrive%\\temp\\",             "%userprofile%\\",
        "%commonprogramfiles%\\",            "%programfiles%\\",
        "%AppData%\\",                       "%AllUsersprofile%\\",
#ifdef _M_X64
        "%SystemRoot%\\SysWow64\\",          "%ProgramFiles(x86)%\\",
        "%CommonProgramFiles(x86)%\\"
#endif
    };

    std::uint64_t oneMonthAgo = MonthsAgo(1);

    std::vector<SystemFacades::FindFilesRecord> fileData;

    for (char const* directory : directories)
    {
        for (FindFiles files(Path::ExpandEnvStrings(directory)); files.NextSuccess();)
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
                    const char** extensions,
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
    static const char* extensions_list15[] = {
        "bat", "reg", "vbs", "wsf", "vbe", "msi", "msp", "com", "pif",
        "ren", "vir", "tmp", "dll", "scr", "sys", "exe", "bin", "drv"};
    static const char* directories_list1a[] = {
        "%PROGRAMFILES%\\",      "%COMMONPROGRAMFILES%\\",
#ifdef _M_X64
        "%PROGRAMFILES(x86)%\\", "%COMMONPROGRAMFILES(x86)%\\",
#endif
    };
    for (char const* directory : directories_list1a)
    {
        for (FindFiles files(Path::ExpandEnvStrings(directory)); files.NextSuccess();)
        {
            auto const& curRecord = files.GetRecord();
            // Discard entries that do not have the proper extension
            if (ExtensionCheck(curRecord.GetFileName(),
                               extensions_list15,
                               _countof(extensions_list15)) == false)
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
    static const char* directories_list1b[] = {
        "%APPDATA%\\",              "%SYSTEMDRIVE%\\", "%SYSTEMROOT%\\",
        "%SYSTEMROOT%\\system32\\", "%USERPROFILE%\\", "%ALLUSERSPROFILE%\\",
#ifdef _M_X64
        "%SYSTEMROOT%\\Syswow64\\",
#endif
    };
    for (char const* directory : directories_list1b)
    {
        for (FindFiles files(Path::ExpandEnvStrings(directory)); files.NextSuccess();)
        {
            auto const& curRecord = files.GetRecord();
            if (curRecord.GetCreationTime() >= threeMonthsAgo)
            {
                // Discard entries that do not have the proper extension
                if (ExtensionCheck(curRecord.GetFileName(),
                                   extensions_list15,
                                   _countof(extensions_list15)) == false)
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
    static const char* directories_list5[] = {
        "%SYSTEMROOT%\\java\\",          "%SYSTEMROOT%\\msapps\\",
        "%SYSTEMROOT%\\pif\\",           "%SYSTEMROOT%\\Registration\\",
        "%SYSTEMROOT%\\help\\",          "%SYSTEMROOT%\\web\\",
        "%SYSTEMROOT%\\pchealth\\",      "%SYSTEMROOT%\\srchasst\\",
        "%SYSTEMROOT%\\tasks\\",         "%SYSTEMROOT%\\apppatch\\",
        "%SYSTEMROOT%\\Internet Logs\\", "%SYSTEMROOT%\\Media\\",
        "%SYSTEMROOT%\\prefetch\\",      "%SYSTEMROOT%\\cursors\\",
        "%SYSTEMROOT%\\inf\\", };
    for (char const* directory : directories_list5)
    {
        // Recursive
        for (FindFiles files(Path::ExpandEnvStrings(directory), FindFilesOptions::RecursiveSearch);
             files.NextSuccess();)
        {
            auto const& curRecord = files.GetRecord();
            if (curRecord.GetCreationTime() >= threeMonthsAgo)
            {
                // Discard entries that do not have the proper extension
                if (ExtensionCheck(curRecord.GetFileName(),
                                   extensions_list15,
                                   _countof(extensions_list15)) == false)
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

    static const char* directories_list2[] = {
        "%SYSTEMROOT%\\System\\",
        "%SYSTEMROOT%\\System32\\Wbem\\",
        "%SYSTEMROOT%\\System32\\GroupPolicy\\Machine\\Scripts\\Shutdown\\",
        "%SYSTEMROOT%\\System32\\GroupPolicy\\User\\Scripts\\Logoff\\",
#ifdef _M_X64
        "%SYSTEMROOT%\\Syswow64\\Drivers\\",
        "%SYSTEMROOT%\\Syswow64\\Wbem\\",
#endif
    };
    static const char* extensions_list2_notExecutable[] = {
        "com", "pif", "ren", "vir", "tmp", "dll",
        "scr", "sys", "exe", "bin", "dat", "drv"};
    static const char* extensions_list2_notDirectory[] = {
        "bat", "cmd", "reg", "vbs", "wsf", "vbe", "msi", "msp"};
    for (char const* directory : directories_list2)
    {
        // List2 is recursive
        for (FindFiles files(Path::ExpandEnvStrings(directory), FindFilesOptions::RecursiveSearch);
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
                               _countof(extensions_list2_notExecutable)))
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
                               _countof(extensions_list2_notDirectory)))
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

    std::string directory_list3 = Path::ExpandEnvStrings(
        "%SYSTEMROOT%\\System32\\Spool\\prtprocs\\w32x86\\");
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

    std::string directory_list6 =
        Path::ExpandEnvStrings("%SYSTEMROOT%\\Fonts\\");
    static const char* extensions_list6[] = {"com", "pif", "ren", "vir",
                                             "tmp", "dll", "scr", "sys",
                                             "exe", "bin", "dat", "drv"};
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
                 _countof(extensions_list6)) &&
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
void PrintFileData(log_sink& logOutput,
                   std::vector<SystemFacades::FindFilesRecord> const& fileData)
{
    for (size_t i = 0; i < 100 && i < fileData.size(); ++i)
    {
        WriteFileListingFromFindData(logOutput, fileData[i]);
        writeln(logOutput);
    }

    if (fileData.size() > 100)
    {
        writeln(logOutput);
        writeln(logOutput, "Too many files to show.  Most recent 100 files shown above.");
    }
}

void FindStarM::Execute(log_sink& logOutput,
                        ScriptSection const& /*sectionData*/,
                        std::vector<std::string> const& /*options*/) const
{
    std::vector<SystemFacades::FindFilesRecord> createdLast30FileData(
        GetCreatedLast30FileData());

    PrintFileData(logOutput, createdLast30FileData);

    std::string head("Find3M");
    Header(head);
    writeln(logOutput);
    writeln(logOutput, head);
    writeln(logOutput);

    std::vector<SystemFacades::FindFilesRecord> find3MFileData(
        GetFind3MFileData(createdLast30FileData));

    PrintFileData(logOutput, find3MFileData);
}
}
