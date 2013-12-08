// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include "Scripting.hpp"

namespace Instalog
{
/// @brief    Running processes scanning section
struct RunningProcesses : public ISectionDefinition
{
    virtual std::string GetScriptCommand() const override
    {
        return "runningprocesses";
    }
    virtual std::string GetName() const override
    {
        return "Running Processes";
    }
    virtual LogSectionPriorities GetPriority() const override
    {
        return SCANNING;
    }
    virtual void Execute(log_sink& logOutput,
                         ScriptSection const& sectionData,
                         std::vector<std::string> const& options) const override;
};

/// @brief    Services/drivers scanning section
struct ServicesDrivers : public ISectionDefinition
{
    virtual std::string GetScriptCommand() const override
    {
        return "servicesdrivers";
    }
    virtual std::string GetName() const override
    {
        return "Services/Drivers";
    }
    virtual LogSectionPriorities GetPriority() const override
    {
        return SCANNING;
    }
    virtual void Execute(log_sink& logOutput,
        ScriptSection const& sectionData,
        std::vector<std::string> const& options) const override;
};

/// @brief    Event viewer scanning section
struct EventViewer : public ISectionDefinition
{
    virtual std::string GetScriptCommand() const override
    {
        return "eventviewer";
    }
    virtual std::string GetName() const override
    {
        return "Event Viewer";
    }
    virtual LogSectionPriorities GetPriority() const override
    {
        return SCANNING;
    }
    virtual void Execute(log_sink& logOutput,
                         ScriptSection const& sectionData,
                         std::vector<std::string> const& options) const override;
};

/// @brief    Machine specifications scanning section
struct MachineSpecifications : public ISectionDefinition
{
    void test();
    virtual std::string GetScriptCommand() const override
    {
        return "machinespecifications";
    }
    virtual std::string GetName() const override
    {
        return "Machine Specifications";
    }
    virtual LogSectionPriorities GetPriority() const override
    {
        return SCANNING;
    }
    virtual void Execute(log_sink& logOutput,
                         ScriptSection const& sectionData,
                         std::vector<std::string> const& options) const override;

    private:
    void OperatingSystem(log_sink& logOutput) const;
    void PerfFormattedData_PerfOS_System(log_sink& logOutput) const;
    void BaseBoard(log_sink& logOutput) const;
    void Processor(log_sink& logOutput) const;
    void LogicalDisk(log_sink& logOutput) const;
};

/// @brief    Restore points scanning section
struct RestorePoints : public ISectionDefinition
{
    virtual std::string GetScriptCommand() const override
    {
        return "restorepoints";
    }
    virtual std::string GetName() const override
    {
        return "Restore Points";
    }
    virtual LogSectionPriorities GetPriority() const override
    {
        return SCANNING;
    }
    virtual void Execute(log_sink& logOutput,
                         ScriptSection const& sectionData,
                         std::vector<std::string> const& options) const override;
};

/// @brief    Installed programs scanning sections
struct InstalledPrograms : public ISectionDefinition
{
    virtual std::string GetScriptCommand() const override
    {
        return "installedprograms";
    }
    virtual std::string GetName() const override
    {
        return "Installed Programs";
    }
    virtual LogSectionPriorities GetPriority() const override
    {
        return SCANNING;
    }
    virtual void Execute(log_sink& logOutput,
                         ScriptSection const& sectionData,
                         std::vector<std::string> const& options) const override;

    private:
    void Enumerate(log_sink& logOutput,
                   std::string const& rootKeyPath) const;
};

/// @brief    Created Last 30 and Find3M
struct FindStarM : public ISectionDefinition
{
    virtual std::string GetScriptCommand() const override
    {
        return "findstarm";
    }
    virtual std::string GetName() const override
    {
        return "Created Last 30";
    }
    virtual LogSectionPriorities GetPriority() const override
    {
        return SCANNING;
    }
    virtual void Execute(log_sink& logOutput,
                         ScriptSection const& sectionData,
                         std::vector<std::string> const& options) const override;
};
}
