// Copyright © 2012 Jacob Snyder, Billy O'Neal III
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
    void RunningProcesses::Execute( std::wostream& logOutput, ScriptSection const&, std::vector<std::wstring> const& ) const
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
        replacements.emplace_back(std::pair<std::wstring, std::wstring>(L"c:\\windows\\", winDir));
        Whitelist w(IDR_RUNNINGPROCESSESWHITELIST, replacements);

        ScopedPrivilege privilegeHolder(SE_DEBUG_NAME);
        ProcessEnumerator enumerator;
        for (ProcessEnumerator::iterator it = enumerator.begin(); it != enumerator.end(); ++it)
        {
            std::wstring executable = it->GetExecutablePath();
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
            auto equTest = [&] (std::wstring const& str) -> bool
            {
                return boost::iequals(executable, str, std::locale());
            };
            if (std::find_if(fullPrintList.begin(), fullPrintList.end(), equTest) != fullPrintList.end())
            {
                pathElement = it->GetCmdLine();
            }
            else
            {
                pathElement = std::move(executable);
            }
            GeneralEscape(pathElement);
            logOutput << std::move(pathElement) << L"\n";
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
        whitelistcheck.reserve(3 + svc.GetSvchostGroup().size() +
            svc.GetFilepath().size() +
            svc.GetServiceName().size() +
            svc.GetDisplayName().size());
        whitelistcheck.append(svc.GetSvchostGroup());
        whitelistcheck.push_back(L';');
        whitelistcheck.append(svc.GetFilepath());
        whitelistcheck.push_back(L';');
        whitelistcheck.append(svc.GetServiceName());
        whitelistcheck.push_back(L';');
        whitelistcheck.append(svc.GetDisplayName());
        return wht.IsOnWhitelist(whitelistcheck);
    }

    void ServicesDrivers::Execute( std::wostream& logOutput, ScriptSection const& /*sectionData*/, std::vector<std::wstring> const& /*options*/ ) const
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
            currentSvcStream << L' ' << service->GetServiceName() << L';' << service->GetDisplayName() << L';';
            if (service->IsSvchostService())
            {
                currentSvcStream << service->GetSvchostGroup() << L"->";
                WriteDefaultFileOutput(currentSvcStream, service->GetSvchostDll());
            }
            else
            {
                WriteDefaultFileOutput(currentSvcStream, service->GetFilepath());
            }
            serviceStrings.emplace_back(currentSvcStream.str());
        }

        std::sort(serviceStrings.begin(), serviceStrings.end(), [] (std::wstring const& a, std::wstring const& b) { return boost::ilexicographical_compare(a, b); });
        std::copy(serviceStrings.cbegin(), serviceStrings.cend(), std::ostream_iterator<std::wstring, wchar_t>(logOutput, L"\n"));
    }

    void EventViewer::Execute( std::wostream& logOutput, ScriptSection const& /*sectionData*/, std::vector<std::wstring> const& /*options*/ ) const
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
        for (auto eventLogEntry = eventLogEntries.begin(); eventLogEntry != eventLogEntries.end(); ++eventLogEntry)
        {
            auto const level = (*eventLogEntry)->level;
            // Whitelist everything but "Critical" and "Error" messages
            if (level != EventLogEntry::EvtLevelCritical && level != EventLogEntry::EvtLevelError) continue;

            // Whitelist all events that are older than this week
            ULARGE_INTEGER date;
            date.LowPart = (*eventLogEntry)->date.dwLowDateTime;
            date.HighPart = (*eventLogEntry)->date.dwHighDateTime;
            if (date.QuadPart < oneWeekAgo.QuadPart) continue;

            // Whitelist EventIDs 1000, 8023, 10010
            auto eventId = (*eventLogEntry)->eventId;
            if (eventId == 1000 || eventId == 8023 || eventId == 10010) continue;

            // Print the Date
            WriteDefaultDateFormat(logOutput, FiletimeToInteger((*eventLogEntry)->date));

            // Print the Type
            switch ((*eventLogEntry)->level)
            {
            case EventLogEntry::EvtLevelCritical: logOutput << L", Critical: "; break;
            case EventLogEntry::EvtLevelError: logOutput << L", Error: "; break;
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
    
    void MachineSpecifications::OperatingSystem( std::wostream &logOutput ) const
    {
        using namespace SystemFacades;

        UniqueComPtr<IWbemServices> wbemServices(GetWbemServices());

        UniqueComPtr<IWbemServices> namespaceCimv2;
        ThrowIfFailed(wbemServices->OpenNamespace(
            BSTR(L"cimv2"), 0, NULL, namespaceCimv2.PassAsOutParameter(), NULL));

        UniqueComPtr<IEnumWbemClassObject> enumWin32_OperatingSystem;
        ThrowIfFailed(namespaceCimv2->CreateInstanceEnum(
            BSTR(L"Win32_OperatingSystem"), WBEM_FLAG_FORWARD_ONLY, NULL, enumWin32_OperatingSystem.PassAsOutParameter()));

        HRESULT hr;
        ULONG returnCount = 0;
        UniqueComPtr<IWbemClassObject> response;
        UniqueVariant variant;

        hr = enumWin32_OperatingSystem->Next(WBEM_INFINITE, 1, response.PassAsOutParameter(), &returnCount);
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

        ThrowIfFailed(response->Get(L"SystemDrive", 0, variant.PassAsOutParameter(), NULL, NULL));
        logOutput << L"Boot Device: " << variant.AsString() << L'\n';

        ThrowIfFailed(response->Get(L"InstallDate", 0, variant.PassAsOutParameter(), NULL, NULL));
        logOutput << L"Install Date: ";
        WriteMillisecondDateFormat(logOutput, FiletimeToInteger(WmiDateStringToFiletime(variant.AsString())));
        logOutput << L'\n';
    }

    struct SystemTimeInformation
    { // MSDN says "reserved [48]" http://msdn.microsoft.com/en-us/library/windows/desktop/ms724509.aspx
      // This came from ReactOS: http://doxygen.reactos.org/da/d46/structSYSTEM__TIMEOFDAY__INFORMATION.html
        __int64 bootTime; // 8
        __int64 currentTime; // 16
        __int64 timeZoneBias; // 24
        unsigned __int32 timeZoneId; // 28
        unsigned __int32 reserved; // 32
        unsigned __int64 bootTimeBias; // 40
        unsigned __int64 sleepTimeBias; // 48
    };

    void MachineSpecifications::PerfFormattedData_PerfOS_System( std::wostream &logOutput ) const
    {
        NtQuerySystemInformationFunc ntQuerySysInfo = 
            SystemFacades::GetNtDll().GetProcAddress<NtQuerySystemInformationFunc>("NtQuerySystemInformation");

        const unsigned __int64 ticksPerDay = 10000000ull * 60ull * 60ull * 24ull;
        const unsigned __int64 ticksPerHour = 10000000ull * 60ull * 60ull;
        const unsigned __int64 ticksPerMinute = 10000000ull * 60ull;
        SystemTimeInformation timeData;
        NTSTATUS errorCheck = ntQuerySysInfo(SystemTimeOfDayInformation, reinterpret_cast<void*>(&timeData), sizeof(timeData), 0);
        if (errorCheck != 0)
        {
            SystemFacades::Win32Exception::ThrowFromNtError(errorCheck);
        }
        uint64_t uptime = timeData.currentTime - timeData.bootTime;
        logOutput << L"Booted at: ";
        WriteDefaultDateFormat(logOutput, timeData.bootTime - (GetTimeZoneBias() * ticksPerMinute));
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

    void MachineSpecifications::Execute( std::wostream& logOutput, ScriptSection const& /*sectionData*/, std::vector<std::wstring> const& /*options*/ ) const
    {
        OperatingSystem(logOutput);
        PerfFormattedData_PerfOS_System(logOutput);
        BaseBoard(logOutput);
        Processor(logOutput);
        LogicalDisk(logOutput);
    }

    void MachineSpecifications::BaseBoard( std::wostream &logOutput ) const
    {
        using namespace SystemFacades;

        UniqueComPtr<IWbemServices> wbemServices(GetWbemServices());

        UniqueComPtr<IWbemServices> namespaceCimv2;
        ThrowIfFailed(wbemServices->OpenNamespace(
            BSTR(L"cimv2"), 0, NULL, namespaceCimv2.PassAsOutParameter(), NULL));

        UniqueComPtr<IEnumWbemClassObject> enumWin32_BaseBoard;
        ThrowIfFailed(namespaceCimv2->CreateInstanceEnum(
            BSTR(L"Win32_BaseBoard"), WBEM_FLAG_FORWARD_ONLY, NULL, enumWin32_BaseBoard.PassAsOutParameter()));

        HRESULT hr;
        ULONG returnCount = 0;
        UniqueComPtr<IWbemClassObject> response;
        UniqueVariant variant;

        hr = enumWin32_BaseBoard->Next(WBEM_INFINITE, 1, response.PassAsOutParameter(), &returnCount);
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

        ThrowIfFailed(response->Get(L"Manufacturer", 0, variant.PassAsOutParameter(), NULL, NULL));
        logOutput << L"Motherboard: " << variant.AsString();
        ThrowIfFailed(response->Get(L"Product", 0, variant.PassAsOutParameter(), NULL, NULL));
        logOutput << L" " << variant.AsString() << L'\n';
    }

    void MachineSpecifications::Processor( std::wostream &logOutput ) const
    {
        using namespace SystemFacades;

        UniqueComPtr<IWbemServices> wbemServices(GetWbemServices());

        UniqueComPtr<IWbemServices> namespaceCimv2;
        ThrowIfFailed(wbemServices->OpenNamespace(
            BSTR(L"cimv2"), 0, NULL, namespaceCimv2.PassAsOutParameter(), NULL));

        UniqueComPtr<IEnumWbemClassObject> enumWin32_Processor;
        ThrowIfFailed(namespaceCimv2->CreateInstanceEnum(
            BSTR(L"Win32_Processor"), WBEM_FLAG_FORWARD_ONLY, NULL, enumWin32_Processor.PassAsOutParameter()));

        HRESULT hr;
        ULONG returnCount = 0;
        UniqueComPtr<IWbemClassObject> response;
        UniqueVariant variant;

        hr = enumWin32_Processor->Next(WBEM_INFINITE, 1, response.PassAsOutParameter(), &returnCount);
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

        ThrowIfFailed(response->Get(L"Name", 0, variant.PassAsOutParameter(), NULL, NULL));
        logOutput << L"Processor: " << variant.AsString() << L'\n';
    }

    void MachineSpecifications::LogicalDisk( std::wostream &logOutput ) const
    {
        using namespace SystemFacades;

        UniqueComPtr<IWbemServices> wbemServices(GetWbemServices());

        UniqueComPtr<IWbemServices> namespaceCimv2;
        ThrowIfFailed(wbemServices->OpenNamespace(
            BSTR(L"cimv2"), 0, NULL, namespaceCimv2.PassAsOutParameter(), NULL));

        UniqueComPtr<IEnumWbemClassObject> enumWin32_LogicalDisk;
        ThrowIfFailed(namespaceCimv2->CreateInstanceEnum(
            BSTR(L"Win32_LogicalDisk"), WBEM_FLAG_FORWARD_ONLY, NULL, enumWin32_LogicalDisk.PassAsOutParameter()));

        for (;;)
        {
            HRESULT hr;
            ULONG returnCount = 0;
            UniqueComPtr<IWbemClassObject> response;
            UniqueVariant variant;

            hr = enumWin32_LogicalDisk->Next(WBEM_INFINITE, 1, response.PassAsOutParameter(), &returnCount);
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

            ThrowIfFailed(response->Get(L"DeviceID", 0, variant.PassAsOutParameter(), NULL, NULL));
            logOutput << variant.AsString() << L" is ";
            
            ThrowIfFailed(response->Get(L"DriveType", 0, variant.PassAsOutParameter(), NULL, NULL));
            switch (variant.AsUlong())
            {
            case 0: logOutput << L"UNKNOWN"; break;
            case 1: logOutput << L"NOROOT"; break;
            case 2: logOutput << L"REMOVABLE"; break;
            case 3: logOutput << L"LOCAL"; break;
            case 4: logOutput << L"NETWORK"; break;
            case 5: logOutput << L"CDROM"; break;
            case 6: logOutput << L"RAM"; break;
            default: assert(false);
            }

            ThrowIfFailed(response->Get(L"Size", 0, variant.PassAsOutParameter(), NULL, NULL));

            if (variant.IsNull())
            {
                logOutput << L'\n';
            }
            else
            {
                ULONGLONG totalSize = variant.AsUlonglong();
                ThrowIfFailed(response->Get(L"FreeSpace", 0, variant.PassAsOutParameter(), NULL, NULL));
                ULONGLONG freeSpace = variant.AsUlonglong();

                totalSize /= 1073741824;
                freeSpace /= 1073741824;

                logOutput << L" - " << totalSize << L" GiB total, " << freeSpace << L" GiB free\n";
            }
        }
    }

    void RestorePoints::Execute( std::wostream& logOutput, ScriptSection const& /*sectionData*/, std::vector<std::wstring> const& /*options*/ ) const
    {
        std::vector<SystemFacades::RestorePoint> restorePoints = SystemFacades::EnumerateRestorePoints();

        for (auto restorePoint = restorePoints.begin(); restorePoint != restorePoints.end(); ++restorePoint)
        {
            logOutput << restorePoint->SequenceNumber << L" " ;
            WriteMillisecondDateFormat(logOutput, FiletimeToInteger(SystemFacades::WmiDateStringToFiletime(restorePoint->CreationTime)));
            logOutput << L" " << restorePoint->Description << L'\n';
        }
    }
    
    void InstalledPrograms::Execute( std::wostream& logOutput, ScriptSection const& /*sectionData*/, std::vector<std::wstring> const& /*options*/ ) const
    {
        Enumerate(logOutput, L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
#ifdef _M_X64
        Enumerate(logOutput, L"\\Registry\\Machine\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
#endif
    }

    void InstalledPrograms::Enumerate( std::wostream& logOutput, std::wstring const& rootKeyPath ) const
    {
        using namespace SystemFacades;

        RegistryKey rootKey(RegistryKey::Open(rootKeyPath));
        if (rootKey.Invalid())
        {
            Win32Exception::ThrowFromNtError(::GetLastError());
        }

        std::vector<RegistryKey> uninstallKeys(rootKey.EnumerateSubKeys());
        std::vector<std::wstring> entries;

        for (auto uninstallKey = uninstallKeys.begin(); uninstallKey != uninstallKeys.end(); ++uninstallKey)
        {
            std::wstring currentEntry;
            try
            {
                uninstallKey->GetValue(L"ParentKeyName");
                continue;
            }
            catch (ErrorFileNotFoundException const&)
            {
                //Expected behavior
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
                //Expected behavior
            }
            catch (InvalidRegistryDataTypeException const&)
            {
                //Expected behavior
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
                    currentEntry += boost::lexical_cast<std::wstring>(versionMajor.GetDWord());
                }
                else
                {
                    currentEntry += versionMajor.GetString();
                }
                currentEntry.push_back(L'.');
                if (versionMinor.GetType() == REG_DWORD)
                {
                    currentEntry += boost::lexical_cast<std::wstring>(versionMinor.GetDWord());
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
            entries.emplace_back(std::move(currentEntry));
        }
        using namespace std::placeholders;
        std::sort(entries.begin(), entries.end(), std::bind(boost::ilexicographical_compare<std::wstring, std::wstring>, _1, _2, std::locale()));
        std::copy(entries.cbegin(), entries.cend(), std::ostream_iterator<std::wstring, wchar_t>(logOutput, L""));
    }
    
    /// @brief    Sort WIN32_FIND_DATAW structs.
    ///
    /// @param    d1    The first const WIN32_FIND_DATAW;
    /// @param    d2    The second const WIN32_FIND_DATAW;
    ///
    /// @return    true if it succeeds, false if it fails.
    /// 
    /// @detail The output will then be sorted by creation date, then by modification date, then by 
    ///         size, then by attribute string, and finally by file path.
    bool SortWin32FindDataW(const WIN32_FIND_DATAW& d1, const WIN32_FIND_DATAW& d2)
    {
        using SystemFacades::File;

        ULARGE_INTEGER largeInt1;
        largeInt1.LowPart  = d1.ftCreationTime.dwLowDateTime;
        largeInt1.HighPart = d1.ftCreationTime.dwHighDateTime;
        ULARGE_INTEGER largeInt2;
        largeInt2.LowPart  = d2.ftCreationTime.dwLowDateTime;
        largeInt2.HighPart = d2.ftCreationTime.dwHighDateTime;
        if (largeInt1.QuadPart != largeInt2.QuadPart)
        {
            return largeInt1.QuadPart > largeInt2.QuadPart;
        }

        largeInt1.LowPart  = d1.ftLastWriteTime.dwLowDateTime;
        largeInt1.HighPart = d1.ftLastWriteTime.dwHighDateTime;
        largeInt2.LowPart  = d2.ftLastWriteTime.dwLowDateTime;
        largeInt2.HighPart = d2.ftLastWriteTime.dwHighDateTime;
        if (largeInt1.QuadPart != largeInt2.QuadPart)
        {
            return largeInt1.QuadPart > largeInt2.QuadPart;
        }

        largeInt1.LowPart  = d1.nFileSizeLow;
        largeInt1.HighPart = d1.nFileSizeHigh;
        largeInt2.LowPart  = d2.nFileSizeLow;
        largeInt2.HighPart = d2.nFileSizeHigh;
        if (largeInt1.QuadPart != largeInt2.QuadPart)
        {
            return largeInt1.QuadPart > largeInt2.QuadPart;
        }

        std::wstringstream attributes1ss, attributes2ss;
        WriteFileAttributes(attributes1ss, File::GetAttributes(d1.cFileName));
        WriteFileAttributes(attributes2ss, File::GetAttributes(d2.cFileName));
        std::wstring attributes1(attributes1ss.str()),
                     attributes2(attributes2ss.str());
        if (attributes1 != attributes2)
        {
            return attributes1 > attributes2;
        }

        return std::wstring(d1.cFileName) > std::wstring(d2.cFileName);
    }

    /// @brief    Gets the number of 100-nanosecond intervals since 1600-whatever that were x months ago
    ///
    /// @param    months    The number of months to go back
    ///
    /// @return    The int representation of a filetime that was that many months ago
    unsigned __int64 MonthsAgo(int months)
    {
        FILETIME fileTime;
        GetSystemTimeAsFileTime(&fileTime);
        unsigned __int64 monthsAgo = FiletimeToInteger(fileTime);
        monthsAgo -= months * 25920000000000; /* 30 days of 100-nanosecond intervals */
        return monthsAgo;
    }

    /// @brief    Adds a base path to cFileName data in the WIN32_FIND_DATAW struct
    ///
    /// @param [in,out]    fileData    WIN32_FIND_DATAW struct of interest
    /// @param    basePath            Base path to prepend to the file path
    ///
    /// @return    The modified struct (it is the same as the argument passed in)
    WIN32_FIND_DATAW AddBasePathToFileData(WIN32_FIND_DATAW &fileData, std::wstring const& basePath)
    {
        wcscpy_s(fileData.cFileName, MAX_PATH, std::wstring(basePath).append(fileData.cFileName).c_str());
        return fileData;
    }

    /// @brief    Removes long strings of 12 or more closely created files from a list
    ///
    /// @param [in,out]    fileData    File data of interest
    void RemoveWindowsUpdateRuns( std::vector<WIN32_FIND_DATAW> &fileData ) 
    {
        // Remove "runs" of files
        if (fileData.size() >= 12)
        {
            auto rangeStart = fileData.begin();
            auto rangeEnd = rangeStart + 1;
            for (; rangeEnd != fileData.end(); ++rangeEnd)
            {
                if (FiletimeToInteger((rangeEnd - 1)->ftCreationTime) - FiletimeToInteger(rangeEnd->ftCreationTime) <= 10000000)
                {
                    continue;
                }
                else
                {
                    if (std::distance(rangeStart, rangeEnd) >= 12)
                    {
                        rangeStart = fileData.erase(rangeStart, rangeEnd);
                    }
                    else
                    {
                        rangeStart = rangeEnd;
                    }

                    if (std::distance(rangeStart, fileData.end()) >= 12)
                    {
                        rangeEnd = rangeStart + 1;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }

    /// @brief    Gets the CreatedLast30 file data
    ///
    /// @return    The CreatedLast30 file data.
    std::vector<WIN32_FIND_DATAW> GetCreatedLast30FileData()
    {
        using SystemFacades::FindFiles;

        static const wchar_t *directories[] = {
            L"%SystemRoot%\\System32\\drivers\\",
            L"%SystemRoot%\\System32\\wbem\\",
            L"%SystemRoot%\\System32\\",
            L"%SystemRoot%\\system\\",
            L"%SystemRoot%\\",
            L"%Systemdrive%\\",
            L"%Systemdrive%\\temp\\",
            L"%userprofile%\\",
            L"%commonprogramfiles%\\",
            L"%programfiles%\\",
            L"%AppData%\\",
            L"%AllUsersprofile%\\",
#ifdef _M_X64
            L"%SystemRoot%\\SysWow64\\",
            L"%ProgramFiles(x86)%\\",
            L"%CommonProgramFiles(x86)%\\"
#endif
        };

        unsigned __int64 oneMonthAgo = MonthsAgo(1);

        std::vector<WIN32_FIND_DATAW> fileData;

        for (size_t i = 0; i < sizeof(directories) / sizeof(const wchar_t *); ++i)
        {
            std::wstring fullDirectory(directories[i]);
            fullDirectory = Path::ExpandEnvStrings(fullDirectory);

            for (FindFiles files(std::wstring(fullDirectory).append(L"*")); files.IsValid(); files.Next())
            {
                unsigned __int64 createdTime = FiletimeToInteger(files.data.ftCreationTime);
                if (createdTime >= oneMonthAgo)
                {
                    fileData.emplace_back(std::move(AddBasePathToFileData(files.data, fullDirectory)));
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
    bool ExtensionCheck(const wchar_t *path, const wchar_t **extensions, size_t numextensions)
    {
        for (size_t i = 0; i < numextensions; ++i)
        {
            if (boost::iends_with(path, extensions[i]))
            {
                return true;
            }
        }

        return false;
    }

    /// @brief    Gets a Find3M file data.
    ///
    /// @param    createdLast30FileData    The createdLast30 data.  If this is empty, it will be 
    ///                                 assumed that CreatedLast30 wasn't run and therefore the 
    ///                                 results should not be stripped from the log.
    ///
    /// @return    The Find3M file data.
    std::vector<WIN32_FIND_DATAW> GetFind3MFileData(std::vector<WIN32_FIND_DATAW> &createdLast30FileData)
    {
        using SystemFacades::FindFiles;
        using SystemFacades::File;

        std::vector<WIN32_FIND_DATAW> fileData;

        unsigned __int64 threeMonthsAgo = MonthsAgo(3);

        // The first part of list1 is generated the same as list5
        static const wchar_t *extensions_list15[] = { 
            L"bat", L"reg", L"vbs", L"wsf", L"vbe", L"msi", L"msp", L"com", L"pif", L"ren", L"vir", 
            L"tmp", L"dll", L"scr", L"sys", L"exe", L"bin", L"drv"
        };
        static const wchar_t *directories_list1a[] = {
            L"%PROGRAMFILES%\\",
            L"%COMMONPROGRAMFILES%\\",
#ifdef _M_X64
            L"%PROGRAMFILES(x86)%\\",
            L"%COMMONPROGRAMFILES(x86)%\\",
#endif
        };
        for (size_t i = 0; i < sizeof(directories_list1a) / sizeof(const wchar_t *); ++i) 
        {
            std::wstring fullDirectory(directories_list1a[i]);
            fullDirectory = Path::ExpandEnvStrings(fullDirectory);

            for (FindFiles files(std::wstring(fullDirectory).append(L"*")); files.IsValid(); files.Next())
            {
                // Discard entries that do not have the proper extension
                if (ExtensionCheck(files.data.cFileName, extensions_list15, sizeof(extensions_list15) / sizeof(const wchar_t *)) == false)
                {
                    continue;
                }

                WIN32_FIND_DATAW data = AddBasePathToFileData(files.data, fullDirectory);

                // Discard entries that are not executables
                if (File::IsExecutable(data.cFileName) == false)
                {    
                    continue;
                }

                fileData.emplace_back(std::move(data));    
            }
        }

        // The second part of list1 also has 3M filtering
        static const wchar_t *directories_list1b[] = {
            L"%APPDATA%\\",
            L"%SYSTEMDRIVE%\\",
            L"%SYSTEMROOT%\\",
            L"%SYSTEMROOT%\\system32\\",
            L"%USERPROFILE%\\",
            L"%ALLUSERSPROFILE%\\",
#ifdef _M_X64
            L"%SYSTEMROOT%\\Syswow64\\",
#endif
        };
        for (size_t i = 0; i < sizeof(directories_list1b) / sizeof(const wchar_t *); ++i) 
        {
            std::wstring fullDirectory(directories_list1b[i]);
            fullDirectory = Path::ExpandEnvStrings(fullDirectory);

            for (FindFiles files(std::wstring(fullDirectory).append(L"*")); files.IsValid(); files.Next())
            {
                unsigned __int64 createdTime = FiletimeToInteger(files.data.ftCreationTime);
                if (createdTime >= threeMonthsAgo)
                {
                    // Discard entries that do not have the proper extension
                    if (ExtensionCheck(files.data.cFileName, extensions_list15, sizeof(extensions_list15) / sizeof(const wchar_t *)) == false)
                    {
                        continue;    
                    }

                    WIN32_FIND_DATAW data = AddBasePathToFileData(files.data, fullDirectory);

                    // Discard entries that are not executable
                    if (File::IsExecutable(data.cFileName) == false)
                    {            
                        continue;
                    }            

                    fileData.emplace_back(std::move(data));    
                }
            }
        }

        // list5 is basically list1b (3M filtering) but also has recursive            
        static const wchar_t *directories_list5[] = {
            L"%SYSTEMROOT%\\java\\",
            L"%SYSTEMROOT%\\msapps\\",
            L"%SYSTEMROOT%\\pif\\",
            L"%SYSTEMROOT%\\Registration\\",
            L"%SYSTEMROOT%\\help\\",
            L"%SYSTEMROOT%\\web\\",
            L"%SYSTEMROOT%\\pchealth\\",
            L"%SYSTEMROOT%\\srchasst\\",
            L"%SYSTEMROOT%\\tasks\\",
            L"%SYSTEMROOT%\\apppatch\\",
            L"%SYSTEMROOT%\\Internet Logs\\",
            L"%SYSTEMROOT%\\Media\\",
            L"%SYSTEMROOT%\\prefetch\\",
            L"%SYSTEMROOT%\\cursors\\",
            L"%SYSTEMROOT%\\inf\\",
        };
        for (size_t i = 0; i < sizeof(directories_list5) / sizeof(const wchar_t *); ++i) 
        {
            std::wstring fullDirectory(directories_list5[i]);
            fullDirectory = Path::ExpandEnvStrings(fullDirectory);

            // Recursive
            for (FindFiles files(std::wstring(fullDirectory).append(L"*"), true); files.IsValid(); files.Next())
            {
                unsigned __int64 createdTime = FiletimeToInteger(files.data.ftCreationTime);
                if (createdTime >= threeMonthsAgo)
                {
                    // Discard entries that do not have the proper extension
                    if (ExtensionCheck(files.data.cFileName, extensions_list15, sizeof(extensions_list15) / sizeof(const wchar_t *)) == false)
                    {
                        continue;    
                    }

                    WIN32_FIND_DATAW data = AddBasePathToFileData(files.data, fullDirectory);

                    // Discard entries that are not executable
                    if (File::IsExecutable(data.cFileName) == false)
                    {            
                        continue;
                    }            

                    fileData.emplace_back(std::move(data));    
                }
            }
        }

        static const wchar_t *directories_list2[] = {
            L"%SYSTEMROOT%\\System\\",
            L"%SYSTEMROOT%\\System32\\Wbem\\",
            L"%SYSTEMROOT%\\System32\\GroupPolicy\\Machine\\Scripts\\Shutdown\\",
            L"%SYSTEMROOT%\\System32\\GroupPolicy\\User\\Scripts\\Logoff\\",
#ifdef _M_X64
            L"%SYSTEMROOT%\\Syswow64\\Drivers\\",
            L"%SYSTEMROOT%\\Syswow64\\Wbem\\",
#endif
        };
        static const wchar_t *extensions_list2_notExecutable[] = { 
            L"com", L"pif", L"ren", L"vir", L"tmp", L"dll", L"scr", L"sys", L"exe", L"bin", L"dat", L"drv"
        };
        static const wchar_t *extensions_list2_notDirectory[] = { 
            L"bat", L"cmd", L"reg", L"vbs", L"wsf", L"vbe", L"msi", L"msp"
        };
        for (size_t i = 0; i < sizeof(directories_list2) / sizeof(const wchar_t *); ++i) 
        {
            std::wstring fullDirectory(directories_list2[i]);
            fullDirectory = Path::ExpandEnvStrings(fullDirectory);

            // List2 is recursive
            for (FindFiles files(std::wstring(fullDirectory).append(L"*"), true); files.IsValid(); files.Next())
            {
                // Discard entries that are more than three months old
                unsigned __int64 createdTime = FiletimeToInteger(files.data.ftCreationTime);
                if (createdTime < threeMonthsAgo)
                {
                    continue;
                }

                WIN32_FIND_DATAW data = AddBasePathToFileData(files.data, fullDirectory);

                // Discard entries that have list2_nonExecutable extensions and are not executable
                if (ExtensionCheck(data.cFileName, extensions_list2_notExecutable, sizeof(extensions_list2_notExecutable) / sizeof(const wchar_t *)))
                {
                    if (File::IsExecutable(data.cFileName) == false)
                    {
                        continue;
                    }
                }

                // Discard entries that have list2_notDirectory extensions and are not directories
                if (ExtensionCheck(data.cFileName, extensions_list2_notDirectory, sizeof(extensions_list2_notDirectory) / sizeof(const wchar_t *)))
                {
                    if (File::IsDirectory(data.cFileName) == false)
                    {
                        continue;
                    }
                }

                fileData.emplace_back(std::move(data));
            }
        }

        std::wstring directory_list3 = L"%SYSTEMROOT%\\System32\\Spool\\prtprocs\\w32x86\\";
        directory_list3 = Path::ExpandEnvStrings(directory_list3);
        for (FindFiles files(std::wstring(directory_list3).append(L"*"), true); files.IsValid(); files.Next())
        {
            WIN32_FIND_DATAW data = AddBasePathToFileData(files.data, directory_list3);

            // Discard non-executables
            if (File::IsExecutable(data.cFileName) == false)
            {
                continue;
            }

            fileData.emplace_back(std::move(data));
        }

        std::wstring directory_list6 = L"%SYSTEMROOT%\\Fonts\\";
        directory_list6 = Path::ExpandEnvStrings(directory_list6);
        static const wchar_t *extensions_list6[] = { 
            L"com", L"pif", L"ren", L"vir", L"tmp", L"dll", L"scr", L"sys", L"exe", L"bin", L"dat", L"drv"
        };
        // List6 is recursive
        for (FindFiles files(std::wstring(directory_list6).append(L"*"), true); files.IsValid(); files.Next())
        {
            WIN32_FIND_DATAW data = AddBasePathToFileData(files.data, directory_list6);

            unsigned __int64 fileSize = File::GetSize(data.cFileName);

            // Keep only those with size between 1500 and 2000 bytes 
            // or
            // greater than 1500 bytes and executable and with list6 extensions
            if ((fileSize >= 1500 && fileSize <= 2000) ||
                (ExtensionCheck(data.cFileName, extensions_list6, sizeof(extensions_list6) / sizeof(const wchar_t *)) &&
                fileSize >= 1500 && File::IsExecutable(data.cFileName)))
            {
                fileData.emplace_back(std::move(data));
            }
        }

        // Sort entries
        std::sort(fileData.begin(), fileData.end(), SortWin32FindDataW);

        // Remove "runs" of files
        RemoveWindowsUpdateRuns(fileData);

        // Remove things from CreatedLast30
        fileData.erase(
            std::remove_if(fileData.begin(), fileData.end(), [&](WIN32_FIND_DATAW const& val)->bool {
                return std::binary_search(createdLast30FileData.begin(), createdLast30FileData.end(), val, SortWin32FindDataW);
            }),
            fileData.end());

        return fileData;
    }

    /// @brief    Logs given FileData
    ///
    /// @param [in,out]    logOutput    The log output stream.
    /// @param    fileData             The fileData to output
    void PrintFileData( std::wostream& logOutput, std::vector<WIN32_FIND_DATAW> const& fileData)
    {
        for (size_t i = 0; i < 100 && i < fileData.size(); ++i)
        {
            WriteFileListingFromFindData(logOutput, fileData[i]);
            logOutput << L'\n';
        }

        if (fileData.size() > 100)
        {
            logOutput << L"\nToo many files to show.  Most recent 100 files shown above.\n";
        }
    }
    
    void FindStarM::Execute( std::wostream& logOutput, ScriptSection const& /*sectionData*/, std::vector<std::wstring> const& /*options*/ ) const
    {
        std::vector<WIN32_FIND_DATAW> createdLast30FileData(GetCreatedLast30FileData());

        PrintFileData(logOutput, createdLast30FileData);

        std::wstring head(L"Find3M");
        Header(head);
        logOutput << L'\n' << head << L'\n' << L'\n';

        std::vector<WIN32_FIND_DATAW> find3MFileData(GetFind3MFileData(createdLast30FileData));

        PrintFileData(logOutput, find3MFileData);
    }

}