// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include "Scripting.hpp"

namespace Instalog
{
	/// @brief	Running processes scanning section
	struct RunningProcesses : public ISectionDefinition
	{
		virtual std::wstring GetScriptCommand() const
		{
			return L"runningprocesses";
		}
		virtual std::wstring GetName() const
		{
			return L"Running Processes";
		}
		virtual LogSectionPriorities GetPriority() const
		{
			return SCANNING;
		}
		virtual void Execute(std::wostream& logOutput, ScriptSection const& sectionData, std::vector<std::wstring> const& options) const;
	};

	/// @brief	Services/drivers scanning section
	struct ServicesDrivers : public ISectionDefinition
	{
		virtual std::wstring GetScriptCommand() const
		{
			return L"servicesdrivers";
		}
		virtual std::wstring GetName() const
		{
			return L"Services/Drivers";
		}
		virtual LogSectionPriorities GetPriority() const
		{
			return SCANNING;
		}
		virtual void Execute(std::wostream& logOutput, ScriptSection const& sectionData, std::vector<std::wstring> const& options) const;
	};

	/// @brief	Event viewer scanning section
	struct EventViewer : public ISectionDefinition
	{
		virtual std::wstring GetScriptCommand() const
		{
			return L"eventviewer";
		}
		virtual std::wstring GetName() const
		{
			return L"Event Viewer";
		}
		virtual LogSectionPriorities GetPriority() const
		{
			return SCANNING;
		}
		virtual void Execute(std::wostream& logOutput, ScriptSection const& sectionData, std::vector<std::wstring> const& options) const;
	};

	/// @brief	Machine specifications scanning section
	struct MachineSpecifications : public ISectionDefinition
	{
		void test();
		virtual std::wstring GetScriptCommand() const
		{
			return L"machinespecifications";
		}
		virtual std::wstring GetName() const
		{
			return L"Machine Specifications";
		}
		virtual LogSectionPriorities GetPriority() const
		{
			return SCANNING;
		}
		virtual void Execute(std::wostream& logOutput, ScriptSection const& sectionData, std::vector<std::wstring> const& options) const;

	private:
		void OperatingSystem(std::wostream &logOutput) const;
		void PerfFormattedData_PerfOS_System(std::wostream &logOutput) const;
		void BaseBoard(std::wostream &logOutput) const;
		void Processor(std::wostream &logOutput) const;
		void LogicalDisk(std::wostream &logOutput) const;
	};

	/// @brief	Restore points scanning section
	struct RestorePoints : public ISectionDefinition
	{
		virtual std::wstring GetScriptCommand() const
		{
			return L"restorepoints";
		}
		virtual std::wstring GetName() const
		{
			return L"Restore Points";
		}
		virtual LogSectionPriorities GetPriority() const
		{
			return SCANNING;
		}
		virtual void Execute(std::wostream& logOutput, ScriptSection const& sectionData, std::vector<std::wstring> const& options) const;
	};

	/// @brief	Installed programs scanning sections
	struct InstalledPrograms : public ISectionDefinition
	{
		virtual std::wstring GetScriptCommand() const
		{
			return L"installedprograms";
		}
		virtual std::wstring GetName() const
		{
			return L"Installed Programs";
		}
		virtual LogSectionPriorities GetPriority() const
		{
			return SCANNING;
		}
		virtual void Execute(std::wostream& logOutput, ScriptSection const& sectionData, std::vector<std::wstring> const& options) const;
	private:
		void Enumerate(std::wostream& logOutput, std::wstring const& rootKeyPath) const;
	};

	/// @brief	Created Last 30 and Find3M
	struct FindStarM : public ISectionDefinition
	{
		virtual std::wstring GetScriptCommand() const
		{
			return L"findstarm";
		}
		virtual std::wstring GetName() const
		{
			return L"Find*M";
		}
		virtual LogSectionPriorities GetPriority() const
		{
			return SCANNING;
		}
		virtual void Execute(std::wostream& logOutput, ScriptSection const& sectionData, std::vector<std::wstring> const& options) const;
	};
}
