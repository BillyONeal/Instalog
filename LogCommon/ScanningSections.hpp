// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
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
}
