// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
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
			try
			{
				std::wstring executable = it->GetExecutablePath();
				if (w.IsOnWhitelist(executable))
				{
					continue;
				}
				Path::Prettify(executable.begin(), executable.end());
				if (std::find(fullPrintList.begin(), fullPrintList.end(), executable) != fullPrintList.end())
				{
					logOutput << it->GetCmdLine() << L"\n";
				}
				else
				{
					logOutput << executable << L"\n";
				}
			}
			catch (ErrorAccessDeniedException const&)
			{
				logOutput << L"[Access Denied (PID=" << it->GetProcessId() << L")]\n";
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

		for (auto service = services.begin(); service != services.end(); ++service)
		{
			if (IsOnServicesWhitelist(*service))
			{
				continue;
			}

			logOutput << service->GetState() << service->GetStart();
			if (service->IsDamagedSvchost())
			{
				logOutput << L'D';
			}
			logOutput << L' ' << service->GetServiceName() << L';' << service->GetDisplayName() << L';';
			if (service->IsSvchostService())
			{
				logOutput << service->GetSvchostGroup() << L"->";
				WriteDefaultFileOutput(logOutput, service->GetSvchostDll());
			}
			else
			{
				WriteDefaultFileOutput(logOutput, service->GetFilepath());
			}
			logOutput << L'\n';
		}
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
			logOutput << description << std::endl;	
		}
	}

}