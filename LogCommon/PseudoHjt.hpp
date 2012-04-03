#include "Scripting.hpp"

namespace Instalog  {

	/// @brief	Pseudo HijackThis report generator.
	class PseudoHjt : public ISectionDefinition
	{
		virtual std::wstring GetScriptCommand() const;
		virtual std::wstring GetName() const;
		virtual LogSectionPriorities GetPriority() const;

		virtual void Execute(std::wostream& logOutput, ScriptSection const& sectionData, std::vector<std::wstring> const& options) const;
	};

}
