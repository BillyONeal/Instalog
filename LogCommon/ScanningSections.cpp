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


	void ServicesDrivers::Execute( std::wostream& logOutput, ScriptSection const& /*sectionData*/, std::vector<std::wstring> const& /*options*/ ) const
	{
		using Instalog::SystemFacades::ServiceControlManager;
		using Instalog::SystemFacades::Service;

		ServiceControlManager scm;
		std::vector<Service> services = scm.GetServices();

		Whitelist whitelist(IDR_SERVICESDRIVERSWHITELIST);

		for (auto service = services.begin(); service != services.end(); ++service)
		{
			std::wstring whitelistcheck;
			if (service->isSvchostService())
			{
				whitelistcheck.append(service->getSvchostGroup());
			}
			else
			{
				whitelistcheck.append(service->getFilepath());			
			}
			whitelistcheck.append(L";");
			whitelistcheck.append(service->getServiceName());
			whitelistcheck.append(L";");
			whitelistcheck.append(service->getDisplayName());
			if (whitelist.IsOnWhitelist(whitelistcheck))
			{
				continue;
			}

			logOutput << service->getState() << service->getStart() << L' ' << service->getServiceName() << L';' << service->getDisplayName() << L';';
			if (service->isSvchostService())
			{
				logOutput << service->getSvchostGroup() << L"->";
				WriteDefaultFileOutput(logOutput, service->getSvchostDll());
			}
			else
			{
				WriteDefaultFileOutput(logOutput, service->getFilepath());
			}
			logOutput << L'\n';
		}
	}

	void EventViewer::Execute( std::wostream& logOutput, ScriptSection const& /*sectionData*/, std::vector<std::wstring> const& /*options*/ ) const
	{
		using Instalog::SystemFacades::EventLog;
		using Instalog::SystemFacades::EventLogEntry;

		SYSTEMTIME currentTime;
		GetSystemTime(&currentTime);
		DWORD oneWeekAgo = SecondsSince1970(currentTime) - 604800;

		EventLog eventLog;
		std::vector<EventLogEntry> eventLogEntries = eventLog.ReadEvents();

		for (auto eventLogEntry = eventLogEntries.begin(); eventLogEntry != eventLogEntries.end(); ++eventLogEntry)
		{
			// Whitelist all events that are older than this week
			if (eventLogEntry->timeGenerated < oneWeekAgo) continue;

			// Whitelist everything but "Critical" and "Error" messages ("Critical") doesn't exist with this API
			if (eventLogEntry->eventType != EVENTLOG_ERROR_TYPE) continue;

			// Whitelist EventIDs 1000, 8023, 10010
			DWORD eventIdCode = eventLogEntry->GetEventIdCode();
			if (eventIdCode == 1000 || eventIdCode == 8023 || eventIdCode == 10010) continue;

			eventLogEntry->OutputToLog(logOutput);
		}
	}

}