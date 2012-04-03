#include "pch.hpp"
#include "PseudoHjt.hpp"

namespace Instalog {

	std::wstring PseudoHjt::GetScriptCommand() const
	{
		return L"pseudohijackthis";
	}

	std::wstring PseudoHjt::GetName() const
	{
		return L"Pseudo HijackThis";
	}

	LogSectionPriorities PseudoHjt::GetPriority() const
	{
		return SCANNING;
	}

	void PseudoHjt::Execute(
		std::wostream& logOutput,
		ScriptSection const&,
		std::vector<std::wstring> const&
	) const
	{
		logOutput << L"Example";
	}

}
