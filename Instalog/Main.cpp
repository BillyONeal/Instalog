#include <iostream>
#include <windows.h>
#include "LogCommon/UserInterface.hpp"
#include "LogCommon/Scripting.hpp"
#include "LogCommon/ScanningSections.hpp"

struct ConsoleInterface : public Instalog::IUserInterface
{
	virtual void ReportProgressPercent(std::size_t progress)
	{
		std::cout << progress << " percent complete.\n";
	}
	virtual void ReportFinished()
	{
		std::cout << "Complete.\n";
	}
	virtual void LogMessage(std::wstring const& str)
	{
		std::wcout << str << L"\n";
	}
};

using namespace Instalog;

int main()
{
	ScriptDispatcher sd;
	sd.AddSectionType(std::unique_ptr<ISectionDefinition>(new RunningProcesses));
	wchar_t const defaultScript[] =
		L":RunningProcesses";
	Script s = sd.Parse(defaultScript);
	std::unique_ptr<IUserInterface> ui(new ConsoleInterface);
	s.Run(std::wcout, ui.get());
}
