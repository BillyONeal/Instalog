#include <iostream>
#include <LogCommon/Win32Exception.hpp>
#include <LogCommon/Process.hpp>
#include <LogCommon/ScopedPrivilege.hpp>

int main()
{
	using Instalog::SystemFacades::ProcessEnumerator;
	using Instalog::SystemFacades::ErrorAccessDeniedException;
	using Instalog::SystemFacades::ScopedPrivilege;
	ScopedPrivilege privilegeHolder(SE_DEBUG_NAME);
	ProcessEnumerator enumerator;
	for (ProcessEnumerator::iterator it = enumerator.begin(); it != enumerator.end(); ++it)
	{
		try
		{
			std::wstring executable = it->GetExecutablePath();
			std::wstring cmdLine = it->GetCmdLine();
			std::wcout << executable << L" -> " << cmdLine << L"\n";
		}
		catch (ErrorAccessDeniedException const&)
		{
			std::wcout << L"[Access Denied] -> [Access Denied]\n";
		}
	}
}
