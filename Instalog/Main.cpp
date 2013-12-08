// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include <fcntl.h>
#include <cstdio>
#include <conio.h>
#define NOMINMAX
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

/// @brief    Main entry-point for this application.
int main()
{
    std::puts(" ___           _        _\n"
        "|_ _|_ __  ___| |_ __ _| | ___   __ _\n"
        " | || '_ \\/ __| __/ _` | |/ _ \\ / _` |\n"
        " | || | | \\__ \\ || (_| | | (_) | (_| |\n"
        "|___|_| |_|___/\\__\\__,_|_|\\___/ \\__, |\n"
        "by Jacob Snyder and Billy ONeal |___/");

    Instalog::SystemFacades::Com com;
    if (Instalog::SystemFacades::IsWow64())
    {
        std::puts("This program is not designed to be run under WOW64 mode.\n"
                  "Please download the x64 copy of Instalog instead.\n"
                  "Press any key to terminate.");
        _getche();
        return -1;
    }
    file_sink outFile("Instalog.txt");
    ScriptParser sd;
    sd.AddSectionDefinition(
        std::unique_ptr<ISectionDefinition>(new RunningProcesses));
    sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new LoadPointsReport));
    sd.AddSectionDefinition(
        std::unique_ptr<ISectionDefinition>(new ServicesDrivers));
    sd.AddSectionDefinition(
        std::unique_ptr<ISectionDefinition>(new EventViewer));
    sd.AddSectionDefinition(
        std::unique_ptr<ISectionDefinition>(new MachineSpecifications));
    sd.AddSectionDefinition(
        std::unique_ptr<ISectionDefinition>(new RestorePoints));
    sd.AddSectionDefinition(
        std::unique_ptr<ISectionDefinition>(new InstalledPrograms));
    sd.AddSectionDefinition(std::unique_ptr<ISectionDefinition>(new FindStarM));
    char const defaultScript[] =
        ":RunningProcesses\n:Loadpoints\n:ServicesDrivers\n:FindStarM\n:EventViewer\n:MachineSpecifications\n:RestorePoints\n:InstalledPrograms\n";
    Script s = sd.Parse(defaultScript);
    std::unique_ptr<IUserInterface> ui(new ConsoleInterface);
    s.Run(outFile, ui.get());
    std::puts("Press enter to close this window.");
    _getche();
    return 0;
}
