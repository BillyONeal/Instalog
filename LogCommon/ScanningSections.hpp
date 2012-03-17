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
}
