#include <iostream>
#include <windows.h>
#include "LogCommon/UserInterface.hpp"

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

int main()
{

}
