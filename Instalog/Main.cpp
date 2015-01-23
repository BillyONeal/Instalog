// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include <fcntl.h>
#include <cstdio>
#include <conio.h>
#include <windows.h>

#include "../LogCommon/Wow64.hpp"
#include "../LogCommon/Win32Exception.hpp"
#include "../LogCommon/UserInterface.hpp"
#include "../LogCommon/Scripting.hpp"
#include "../LogCommon/ScanningSections.hpp"
#include "../LogCommon/LoadPointsReport.hpp"
#include "../LogCommon/Com.hpp"

/// @brief    Console "user interface"
struct ConsoleInterface : public Instalog::IUserInterface
{
    virtual void ReportProgressPercent(std::size_t progress)
    {
        std::printf("%d percent complete\n", progress);
    }
    virtual void ReportFinished()
    {
        std::puts("Complete.");
    }
    virtual void LogMessage(std::string const& str)
    {
        std::puts(str.c_str());
    }
};

using namespace Instalog;

typedef BOOL (WINAPI *SetProcessDEPPolicyFunc)(
  _In_  DWORD dwFlags
);

typedef BOOL (WINAPI *SetProcessMitigationPolicyFunc)(
    _In_  PROCESS_MITIGATION_POLICY MitigationPolicy,
    _In_  PVOID lpBuffer,
    _In_  SIZE_T dwLength
    );

static void DisableBackCompat()
{
    // Windows Vista or later:
    auto setProcessDep = SystemFacades::GetKernel32()
        .GetProcAddress<SetProcessDEPPolicyFunc>(GetIgnoreReporter(), "SetProcessDEPPolicy");
    if (setProcessDep == nullptr)
    {
        return;
    }

    // Make sure DEP failures kill the process.
    setProcessDep(0x3); // Turns on all the things.

    // Windows 8 or later:
    auto setProcMitigation = SystemFacades::GetKernel32()
        .GetProcAddress<SetProcessMitigationPolicyFunc>(GetIgnoreReporter(), "SetProcessMitigationPolicy");
    if (setProcMitigation == nullptr)
    {
        return;
    }
    
    // Make sure passing around invalid handles kill the process.
    PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY strictHandle = {};
    strictHandle.RaiseExceptionOnInvalidHandleReference = true;
    strictHandle.HandleExceptionsPermanentlyEnabled = true;
    setProcMitigation(ProcessStrictHandleCheckPolicy, &strictHandle, sizeof(strictHandle));
}

/// @brief    Main entry-point for this application.
int main()
{
    DisableBackCompat();

    std::puts(" ___           _        _\n"
        "|_ _|_ __  ___| |_ __ _| | ___   __ _\n"
        " | || '_ \\/ __| __/ _` | |/ _ \\ / _` |\n"
        " | || | | \\__ \\ || (_| | | (_) | (_| |\n"
        "|___|_| |_|___/\\__\\__,_|_|\\___/ \\__, |\n"
        "by Jacob Snyder and Billy ONeal |___/");

    Instalog::SystemFacades::Com com(COINIT_APARTMENTTHREADED, GetThrowingErrorReporter());
    if (Instalog::SystemFacades::IsWow64())
    {
        std::puts("This program is not designed to be run under WOW64 mode.\n"
                  "Please download the x64 copy of Instalog instead.\n"
                  "Press any key to terminate.");
        _getche();
        return -1;
    }

    file_sink outFile("%USERPROFILE%\\Desktop\\Instalog.txt");
    ScriptParser sd;
    sd.AddSectionDefinition(std::make_unique<RunningProcesses>());
    sd.AddSectionDefinition(std::make_unique<LoadPointsReport>());
    sd.AddSectionDefinition(std::make_unique<ServicesDrivers>());
    sd.AddSectionDefinition(std::make_unique<EventViewer>());
    sd.AddSectionDefinition(std::make_unique<MachineSpecifications>());
    sd.AddSectionDefinition(std::make_unique<RestorePoints>());
    sd.AddSectionDefinition(std::make_unique<InstalledPrograms>());
    sd.AddSectionDefinition(std::make_unique<FindStarM>());
    char const defaultScript[] =
        ":RunningProcesses\n:Loadpoints\n:ServicesDrivers\n:FindStarM\n:EventViewer\n:MachineSpecifications\n:RestorePoints\n:InstalledPrograms\n";
    Script s = sd.Parse(defaultScript);
    std::unique_ptr<IUserInterface> ui(new ConsoleInterface);
    s.Run(outFile, ui.get());
    std::puts("Press enter to close this window.");
    _getche();
    return 0;
}
