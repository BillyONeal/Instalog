// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.
#include "pch.hpp"
#include <atlbase.h>
#include <comdef.h>
#include <Wbemidl.h>
#include "Com.hpp"
#include "SecurityCenter.hpp"
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

	static void SecurityCenterOutput( std::wostream& output )
	{
		using SystemFacades::SecurityProduct;
		auto products = SystemFacades::EnumerateSecurityProducts();
		for (auto it = products.cbegin(); it != products.cend(); ++it)
		{
			output << it->GetTwoLetterPrefix()
				<< L": [" << it->GetInstanceGuid() << L"] ";
			if (it->IsEnabled())
			{
				output << L'E';
			}
			else
			{
				output << L'D';
			}
			switch (it->GetUpdateStatus())
			{
			case SecurityProduct::OutOfDate:
				output << L'O';
				break;
			case SecurityProduct::UpToDate:
				output << L'U';
				break;
			}
			output << L' ' << it->GetName() << L'\n';
		}
	}

	void PseudoHjt::Execute(
		std::wostream& output,
		ScriptSection const&,
		std::vector<std::wstring> const&
	) const
	{
		SecurityCenterOutput(output);

	}

}
