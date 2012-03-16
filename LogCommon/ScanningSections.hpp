#include "Scripting.hpp"

namespace Instalog
{
	struct RunningProcesses : public ISectionDefinition
	{
		virtual std::wstring GetName() const
		{
			return L"Running Processes";
		}
		virtual LogSectionPriorities GetPriority() const
		{
			return SCANNING;
		}
		virtual void Execute(std::wostream& logOutput, IUserInterface *ui, ScriptSection const& sectionData, std::vector<std::wstring> const& options) const;
	};

	struct ServicesDrivers : public ISectionDefinition
	{
		virtual std::wstring GetName() const
		{
			return L"Services/Drivers";
		}
		virtual LogSectionPriorities GetPriority() const
		{
			return SCANNING;
		}
		virtual void Execute(std::wostream& logOutput, IUserInterface *ui, ScriptSection const& sectionData, std::vector<std::wstring> const& options) const;
	};
}
