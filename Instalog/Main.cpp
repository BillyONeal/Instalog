// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <windows.h>
#pragma warning(push)
#pragma warning(disable: 4512)
#include <boost/locale.hpp>
#pragma warning(pop)
#include "LogCommon/Wow64.hpp"
#include "LogCommon/Win32Exception.hpp"
#include "LogCommon/UserInterface.hpp"
#include "LogCommon/Scripting.hpp"
#include "LogCommon/ScanningSections.hpp"
#include "LogCommon/PseudoHjt.hpp"
#include "LogCommon/Com.hpp"

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
	Instalog::SystemFacades::Com com;
#ifdef NDEBUG
	try
	{
#endif
		if (Instalog::SystemFacades::IsWow64())
		{
			std::cerr << "This program is not designed to be run under WOW64 mode. Please download the x64 copy of Instalog instead.";
			return -1;
		}
		std::wofstream outFile(L"Instalog.txt", std::ios::trunc | std::ios::out);
        outFile.imbue(boost::locale::generator().generate(std::locale(), "en_US.UTF-8"));
		ScriptParser sd;
		sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new RunningProcesses));
		sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new PseudoHjt));
		sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new ServicesDrivers));
		sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new EventViewer));
		sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new MachineSpecifications));
		sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new RestorePoints));
		sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new InstalledPrograms));
		wchar_t const defaultScript[] =
			L":RunningProcesses\n:PseudoHijackThis\n:ServicesDrivers\n:EventViewer\n:MachineSpecifications\n:RestorePoints\n:InstalledPrograms\n";
		Script s = sd.Parse(defaultScript);
		std::unique_ptr<IUserInterface> ui(new ConsoleInterface);
		s.Run(outFile, ui.get());
#ifdef NDEBUG
	}
	catch (Instalog::SystemFacades::HresultException const& ex)
	{
		std::wcerr << L"Hresult Error: 0x" << std::hex << ex.GetErrorCode() << L": " << ex.GetErrorStringW() << std::endl;
	}
	catch (Instalog::SystemFacades::Win32Exception const& ex)
	{
		std::wcerr << L"Win32 Error: 0x" << std::hex << ex.GetErrorCode() << L": " << ex.GetWideMessage() << std::endl;
	}
	catch (std::exception const& ex)
	{
		std::cerr << "Exception: " << ex.what() << std::endl;
	}
#endif

	//system("notepad.exe Instalog.txt");
}
