#include <iostream>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include "LogCommon/Wow64.hpp"
#include "LogCommon/Win32Exception.hpp"
#include "LogCommon/UserInterface.hpp"
#include "LogCommon/Scripting.hpp"
#include "LogCommon/ScanningSections.hpp"

/// @brief	Console "user interface"
struct ConsoleInterface : public Instalog::IUserInterface
{
	virtual void ReportProgressPercent(std::size_t progress)
	{
		std::wcout << progress << L" percent complete.\n";
	}
	virtual void ReportFinished()
	{
		std::wcout << L"Complete.\n";
	}
	virtual void LogMessage(std::wstring const& str)
	{
		std::wcout << str << L"\n";
	}
};

using namespace Instalog;

/// @brief	Main entry-point for this application.
int main()
{
	try
	{
		if (Instalog::SystemFacades::IsWow64())
		{
			std::cerr << "This program is not designed to be run under WOW64 mode. Please download the x64 copy of Instalog instead.";
			return -1;
		}
		_setmode(_fileno(stdout), _O_WTEXT);
		ScriptDispatcher sd;
		sd.AddSectionType(std::unique_ptr<ISectionDefinition>(new RunningProcesses));
		sd.AddSectionType(std::unique_ptr<ISectionDefinition>(new ServicesDrivers));
		wchar_t const defaultScript[] =
			L":RunningProcesses\n:ServicesDrivers";
		Script s = sd.Parse(defaultScript);
		std::unique_ptr<IUserInterface> ui(new ConsoleInterface);
		s.Run(std::wcout, ui.get());
	}
	catch (Instalog::SystemFacades::Win32Exception const& ex)
	{
		std::wcerr << L"Win32 Error: 0x" << std::hex << ex.GetErrorCode() << L": " << ex.GetWideMessage() << std::endl;
	}
	catch (std::exception const& ex)
	{
		std::cerr << "Exception: " << ex.what() << std::endl;
	}
}
