// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include <iostream>
#include <fstream>
#include <fcntl.h>
#define NOMINMAX
#include <windows.h>

// HACK HACK HACK: In order to print the stack trace, we need to catch the exception as a SEH
// exception; before MSVC++ has done the translation back into a C++ exception for us.
// Include VC's own headers in order to decode that exception inside a SEH handler.
#pragma warning(push)
#pragma warning(disable: 4201) // Nonstandard extension: nameless struct/union
#define _VC_VER_INC
#include "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\crt\src\ehdata.h"
#pragma warning(pop)
#include "ThirdParty/StackWalker/StackWalker.h"
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

class InstalogStackWalker : public StackWalker, private boost::noncopyable
{
    bool insideInstalogCode;
public:
    InstalogStackWalker()
        : insideInstalogCode(false)
    {
    }
    virtual void OnLoadModule(LPCSTR, LPCSTR mod, DWORD64, DWORD, DWORD result, LPCSTR symType, LPCSTR pdbName, ULONGLONG)
    {
        // Only print messages about loaded symbols when the symbols are loaded from a PDB file (e.g. as Instalog's are)
        if (result == ERROR_SUCCESS && strcmp(symType, "PDB") == 0)
        {
            std::cerr << "Loaded symbols for " << mod << " (from " << pdbName << ")" << std::endl;
        }
    }
    virtual void OnCallstackEntry(CallstackEntryType, CallstackEntry &entry) override
    {
        std::string name(entry.undFullName);
        if (name == "_CxxThrowException")
        {
            // _CxxThrowException will be one past the top of the exception stack in Instalog's source
            insideInstalogCode = true;
            return;
        }
        else if (name == "__tmainCRTStartup")
        {
            // __tmainCRTStartup is the first function visible in the CRT; it is the function that calls
            // main().
            insideInstalogCode = false;
            return;
        }
        
        if (insideInstalogCode)
        {
            std::cerr << entry.lineFileName << " (" << entry.lineNumber << "): " << entry.undFullName << std::endl;
        }
    }
};

LONG WINAPI UnhandledExceptionHandler(LPEXCEPTION_POINTERS pointers)
{
    if (pointers->ExceptionRecord->ExceptionCode != EH_EXCEPTION_NUMBER)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    std::wcerr << "FAILURE!\n";
    auto ehExceptionRecord = reinterpret_cast<EHExceptionRecord *>(pointers->ExceptionRecord);
    auto ehObjectPtr = ehExceptionRecord->params.pExceptionObject;
    auto stdException = static_cast<std::exception*>(ehObjectPtr);
    auto hresultExt = dynamic_cast<Instalog::SystemFacades::HresultException*>(stdException);
    auto win32Ext = dynamic_cast<Instalog::SystemFacades::Win32Exception*>(stdException);
	if (hresultExt != nullptr)
	{
		std::wcerr << L"HRESULT Error: 0x" << std::hex << hresultExt->GetErrorCode() << L": " << hresultExt->GetErrorStringW() << std::endl;
	}
    else if (win32Ext != nullptr)
	{
		std::wcerr << L"Win32 Error: 0x" << std::hex << win32Ext->GetErrorCode() << L": " << win32Ext->GetWideMessage() << std::endl;
	}
    else
	{
		std::cerr << "C++ Exception: " << stdException->what() << std::endl;
	}
    std::wcerr << "Call Stack:\n";
    InstalogStackWalker walker;
    walker.ShowCallstack(::GetCurrentThread(), pointers->ContextRecord);
    std::cerr << "Press enter to close this window...";
    std::wcin.get();
    ::TerminateProcess(::GetCurrentProcess(), 1);
    return 0;
}

/// @brief	Main entry-point for this application.
int main()
{
    std::wcout <<
        L" ___           _        _\n"
        L"|_ _|_ __  ___| |_ __ _| | ___   __ _\n"
        L" | || '_ \\/ __| __/ _` | |/ _ \\ / _` |\n"
        L" | || | | \\__ \\ || (_| | | (_) | (_| |\n"
        L"|___|_| |_|___/\\__\\__,_|_|\\___/ \\__, |\n"
        L"by Jacob Snyder and Billy ONeal |___/\n";

    ::SetUnhandledExceptionFilter(UnhandledExceptionHandler);
	Instalog::SystemFacades::Com com;
    if (Instalog::SystemFacades::IsWow64())
    {
        std::cerr << "This program is not designed to be run under WOW64 mode. Please download the x64 copy of Instalog instead.";
        return -1;
    }
    std::wofstream outFile(L"Instalog.txt", std::ios::trunc | std::ios::out);
    ScriptParser sd;
    sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new RunningProcesses));
    sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new PseudoHjt));
    sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new ServicesDrivers));
    sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new EventViewer));
    sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new MachineSpecifications));
    sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new RestorePoints));
    sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new InstalledPrograms));
    sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new FindStarM));
    wchar_t const defaultScript[] =
        L":RunningProcesses\n:PseudoHijackThis\n:ServicesDrivers\n:FindStarM\n:EventViewer\n:MachineSpecifications\n:RestorePoints\n:InstalledPrograms\n";
    Script s = sd.Parse(defaultScript);
    std::unique_ptr<IUserInterface> ui(new ConsoleInterface);
    s.Run(outFile, ui.get());
    outFile.close();
    std::wcout << L"Press enter to close this window.";
    std::wcin.get();
}
