#include "pch.hpp"
#include <vector>
#include <boost/algorithm/string/case_conv.hpp>
#include "Process.hpp"
#include "ScopedPrivilege.hpp"
#include "Win32Exception.hpp"
#include "ScanningSections.hpp"

namespace Instalog
{
	void RunningProcesses::Execute( std::wostream& logOutput, IUserInterface *ui, ScriptSection const& sectionData, std::vector<std::wstring> const& options ) const
	{
		using Instalog::SystemFacades::ProcessEnumerator;
		using Instalog::SystemFacades::ErrorAccessDeniedException;
		using Instalog::SystemFacades::ScopedPrivilege;

		std::vector<std::wstring> fullPrintList;
		std::wstring winDir;
		winDir.resize(MAX_PATH);
		UINT winDirSize = ::GetWindowsDirectoryW(&winDir[0], MAX_PATH);
		winDir.resize(winDirSize);
		fullPrintList.push_back(winDir + L"\\system32\\svchost.exe");
		fullPrintList.push_back(winDir + L"\\System32\\svchost.exe");
		fullPrintList.push_back(winDir + L"\\system32\\svchost");
		fullPrintList.push_back(winDir + L"\\System32\\svchost");
		fullPrintList.push_back(winDir + L"\\system32\\rundll32.exe");
		fullPrintList.push_back(winDir + L"\\System32\\rundll32.exe");
		std::sort(fullPrintList.begin(), fullPrintList.end());

		ScopedPrivilege privilegeHolder(SE_DEBUG_NAME);
		ProcessEnumerator enumerator;
		for (ProcessEnumerator::iterator it = enumerator.begin(); it != enumerator.end(); ++it)
		{
			try
			{
				std::wstring executable = it->GetExecutablePath();
				if (std::binary_search(fullPrintList.begin(), fullPrintList.end(), executable))
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

}