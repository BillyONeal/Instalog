// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "Scripting.hpp"

namespace Instalog  {

	/// @brief	Pseudo HijackThis report generator.
	class PseudoHjt : public ISectionDefinition
	{
		virtual std::wstring GetScriptCommand() const;
		virtual std::wstring GetName() const;
		virtual LogSectionPriorities GetPriority() const;

		virtual void Execute(
			std::wostream& logOutput,
			ScriptSection const& sectionData,
			std::vector<std::wstring> const& options
		) const;

	};

}
