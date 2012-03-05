#include <iostream>
#include <LogCommon/Win32Exception.hpp>
#include <LogCommon/Process.hpp>

int main()
{
	using Instalog::SystemFacades::ProcessEnumerator;
	using Instalog::SystemFacades::ErrorAccessDeniedException;
	ProcessEnumerator enumerator;
	for (ProcessEnumerator::iterator it = enumerator.begin(); it != enumerator.end(); ++it)
	{
		try
		{
			std::wcout << it->GetExecutablePath() << L" -> " << it->GetCmdLine() << L"\n";
		}
		catch (ErrorAccessDeniedException const&)
		{
			std::wcout << L"[Access Denied] -> [Access Denied]\n";
		}
	}
}
