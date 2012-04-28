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
            if (std::find(fullPrintList.begin(), fullPrintList.end(), executable) != fullPrintList.end())
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

        using namespace std::placeholders;
        std::sort(serviceStrings.begin(), serviceStrings.end(), std::bind(boost::ilexicographical_compare<std::wstring, std::wstring>, _1, _2, std::locale()));
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
		oneWeekAgo.QuadPart = currentTime.QuadPart - 6048000000000;

		// Log applicable events
		for (auto eventLogEntry = eventLogEntries.begin(); eventLogEntry != eventLogEntries.end(); ++eventLogEntry)
		{
			// Whitelist everything but "Critical" and "Error" messages
			if ((*eventLogEntry)->level != EventLogEntry::EvtLevelCritical && (*eventLogEntry)->level != EventLogEntry::EvtLevelError) continue;

			// Whitelist all events that are older than this week
			ULARGE_INTEGER date;
			date.LowPart = (*eventLogEntry)->date.dwLowDateTime;
			date.HighPart = (*eventLogEntry)->date.dwHighDateTime;
			if (date.QuadPart < oneWeekAgo.QuadPart) continue;

			// Whitelist EventIDs 1000, 8023, 10010
			if ((*eventLogEntry)->eventId == 1000 || (*eventLogEntry)->eventId == 8023 || (*eventLogEntry)->eventId == 10010) continue;

			// Print the Date
			WriteDefaultDateFormat(logOutput, FiletimeToInteger((*eventLogEntry)->date));

			// Print the Type
			switch ((*eventLogEntry)->level)
			{
			case EventLogEntry::EvtLevelCritical: logOutput << L", Critical: "; break;
			case EventLogEntry::EvtLevelError: logOutput << L", Error: "; break;
			}
			 
			// Print the Source
			logOutput << (*eventLogEntry)->GetSource() << L" [";
			 
			// Print the EventID
			logOutput << (*eventLogEntry)->eventId << L"] ";

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

		CComPtr<IWbemServices> wbemServices(GetWbemServices());

		CComPtr<IWbemServices> namespaceCimv2;
		ThrowIfFailed(wbemServices->OpenNamespace(
			BSTR(L"cimv2"), 0, NULL, &namespaceCimv2, NULL));

		CComPtr<IEnumWbemClassObject> enumWin32_OperatingSystem;
		ThrowIfFailed(namespaceCimv2->CreateInstanceEnum(
			BSTR(L"Win32_OperatingSystem"), WBEM_FLAG_FORWARD_ONLY, NULL, &enumWin32_OperatingSystem));

		HRESULT hr;
		ULONG returnCount = 0;
		CComPtr<IWbemClassObject> response;
		CComVariant variant;

		hr = enumWin32_OperatingSystem->Next(WBEM_INFINITE, 1, &response, &returnCount);
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

		ThrowIfFailed(variant.ChangeType(VT_BSTR));
		ThrowIfFailed(response->Get(L"SystemDrive", 0, &variant, NULL, NULL));
		logOutput << L"Boot Device: " << std::wstring(variant.bstrVal, SysStringLen(variant.bstrVal)) << L'\n';
		variant.Clear();

		ThrowIfFailed(response->Get(L"InstallDate", 0, &variant, NULL, NULL));
		logOutput << L"Install Date: ";
		WriteMillisecondDateFormat(logOutput, FiletimeToInteger(WmiDateStringToFiletime(std::wstring(variant.bstrVal, SysStringLen(variant.bstrVal)))));
		logOutput << L'\n';
	}

	void MachineSpecifications::PerfFormattedData_PerfOS_System( std::wostream &logOutput ) const
	{
		NtQuerySystemInformationFunc ntQuerySysInfo = 
            SystemFacades::GetNtDll().GetProcAddress<NtQuerySystemInformationFunc>("NtQuerySystemInformation");

        union
        {
            unsigned __int64 timeArray[3];
            unsigned char c;
        };

		NTSTATUS errorCheck = ntQuerySysInfo(SystemTimeOfDayInformation, &c, sizeof(timeArray), 0);
		if (errorCheck != 0)
		{
			SystemFacades::Win32Exception::ThrowFromNtError(errorCheck);
		}
        unsigned __int64 bootTime = timeArray[0];
        unsigned __int64 currentTime = timeArray[1];
        unsigned __int64 uptime = currentTime - bootTime;
        logOutput << L"Booted at: ";
        WriteDefaultDateFormat(logOutput, bootTime);
        logOutput << L" (Up ";
        const unsigned __int64 ticksPerDay = 10000000ull * 60ull * 60ull * 24ull;
        const unsigned __int64 ticksPerHour = 10000000ull * 60ull * 60ull;
        const unsigned __int64 ticksPerMinute = 10000000ull * 60ull;
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

		CComPtr<IWbemServices> wbemServices(GetWbemServices());

		CComPtr<IWbemServices> namespaceCimv2;
		ThrowIfFailed(wbemServices->OpenNamespace(
			BSTR(L"cimv2"), 0, NULL, &namespaceCimv2, NULL));

		CComPtr<IEnumWbemClassObject> enumWin32_BaseBoard;
		ThrowIfFailed(namespaceCimv2->CreateInstanceEnum(
			BSTR(L"Win32_BaseBoard"), WBEM_FLAG_FORWARD_ONLY, NULL, &enumWin32_BaseBoard));

		HRESULT hr;
		ULONG returnCount = 0;
		CComPtr<IWbemClassObject> response;
		CComVariant variant;

		hr = enumWin32_BaseBoard->Next(WBEM_INFINITE, 1, &response, &returnCount);
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

		ThrowIfFailed(variant.ChangeType(VT_BSTR));
		ThrowIfFailed(response->Get(L"Manufacturer", 0, &variant, NULL, NULL));
		logOutput << L"Motherboard: " << std::wstring(variant.bstrVal, SysStringLen(variant.bstrVal));
		ThrowIfFailed(response->Get(L"Product", 0, &variant, NULL, NULL));
		logOutput << L" " << std::wstring(variant.bstrVal, SysStringLen(variant.bstrVal)) << L'\n';
		variant.Clear();
	}

	void MachineSpecifications::Processor( std::wostream &logOutput ) const
	{
		using namespace SystemFacades;

		CComPtr<IWbemServices> wbemServices(GetWbemServices());

		CComPtr<IWbemServices> namespaceCimv2;
		ThrowIfFailed(wbemServices->OpenNamespace(
			BSTR(L"cimv2"), 0, NULL, &namespaceCimv2, NULL));

		CComPtr<IEnumWbemClassObject> enumWin32_Processor;
		ThrowIfFailed(namespaceCimv2->CreateInstanceEnum(
			BSTR(L"Win32_Processor"), WBEM_FLAG_FORWARD_ONLY, NULL, &enumWin32_Processor));

		HRESULT hr;
		ULONG returnCount = 0;
		CComPtr<IWbemClassObject> response;
		CComVariant variant;

		hr = enumWin32_Processor->Next(WBEM_INFINITE, 1, &response, &returnCount);
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

		ThrowIfFailed(response->Get(L"Name", 0, &variant, NULL, NULL));
		logOutput << L"Processor: " << std::wstring(variant.bstrVal, SysStringLen(variant.bstrVal)) << L'\n';
		variant.Clear();
	}

	void MachineSpecifications::LogicalDisk( std::wostream &logOutput ) const
	{
		using namespace SystemFacades;

		CComPtr<IWbemServices> wbemServices(GetWbemServices());

		CComPtr<IWbemServices> namespaceCimv2;
		ThrowIfFailed(wbemServices->OpenNamespace(
			BSTR(L"cimv2"), 0, NULL, &namespaceCimv2, NULL));

		CComPtr<IEnumWbemClassObject> enumWin32_LogicalDisk;
		ThrowIfFailed(namespaceCimv2->CreateInstanceEnum(
			BSTR(L"Win32_LogicalDisk"), WBEM_FLAG_FORWARD_ONLY, NULL, &enumWin32_LogicalDisk));

		for (;;)
		{
			HRESULT hr;
			ULONG returnCount = 0;
			CComPtr<IWbemClassObject> response;
			CComVariant variant;

			hr = enumWin32_LogicalDisk->Next(WBEM_INFINITE, 1, &response, &returnCount);
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

			ThrowIfFailed(variant.ChangeType(VT_BSTR));
			ThrowIfFailed(response->Get(L"DeviceID", 0, &variant, NULL, NULL));
			logOutput << std::wstring(variant.bstrVal, SysStringLen(variant.bstrVal)) << L" is ";
			variant.Clear();
			
			ThrowIfFailed(variant.ChangeType(VT_I4));
			ThrowIfFailed(response->Get(L"DriveType", 0, &variant, NULL, NULL));
			ULONG driveType = variant.ulVal;
			switch (driveType)
			{
			case 0: logOutput << L"UNKNOWN"; break;
			case 1: logOutput << L"NOROOT"; break;
			case 2: logOutput << L"REMOVABLE"; break;
			case 3: logOutput << L"LOCAL"; break;
			case 4: logOutput << L"NETWORK"; break;
			case 5: logOutput << L"CDROM"; break;
			case 6: logOutput << L"RAM"; break;
			}
			variant.Clear();

			ThrowIfFailed(variant.ChangeType(VT_BSTR));
			ThrowIfFailed(response->Get(L"Size", 0, &variant, NULL, NULL));
			ULONGLONG totalSize = _wtoi64(std::wstring(variant.bstrVal, SysStringLen(variant.bstrVal)).c_str());

			if (totalSize == 0)
			{
				logOutput << L'\n';
			}
			else
			{
				ThrowIfFailed(response->Get(L"FreeSpace", 0, &variant, NULL, NULL));
				ULONGLONG freeSpace = _wtoi64(std::wstring(variant.bstrVal, SysStringLen(variant.bstrVal)).c_str());

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

	// The output will then be sorted by creation date, then by modification date, then by size, then by
	// attribute string, and finally by file path.
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
	
	void FindStarM::Execute( std::wostream& logOutput, ScriptSection const& /*sectionData*/, std::vector<std::wstring> const& /*options*/ ) const
	{
		using SystemFacades::FindFiles;
		using SystemFacades::Win32Exception;

		static const wchar_t *searchDirectories[] = {
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

		FILETIME fileTime;
		GetSystemTimeAsFileTime(&fileTime);
        unsigned __int64 oneMonthAgo = FiletimeToInteger(fileTime);
		oneMonthAgo-= 25920000000000; /* 30 days of 100-nanosecond intervals */

		std::vector<WIN32_FIND_DATAW> fileData;

		std::vector<std::wstring> directories(
			searchDirectories,
			searchDirectories + (sizeof(searchDirectories)/sizeof(const wchar_t *))
			);

		for (auto directory = directories.begin(); directory != directories.end(); ++directory)
		{
			std::wstring fullDirectory(*directory);
			fullDirectory = Path::ExpandEnvStrings(fullDirectory);

			for (FindFiles files(std::wstring(fullDirectory).append(L"*")); files.IsValid(); files.Next())
			{
                unsigned __int64 createdTime = FiletimeToInteger(files.data.ftCreationTime);
				if (createdTime >= oneMonthAgo)
				{
					WIN32_FIND_DATAW findData = files.data;
					wcscpy_s(findData.cFileName, MAX_PATH, std::wstring(fullDirectory).append(findData.cFileName).c_str());

					fileData.emplace_back(std::move(findData));
				}
			}
		}

		std::sort(fileData.begin(), fileData.end(), SortWin32FindDataW);

		for (auto data = fileData.begin(); data < fileData.end(); ++data)
		{
			WriteFileListingFromFindData(logOutput, *data);
			logOutput << std::endl;
		}
	}

}