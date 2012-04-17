// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.
#include "pch.hpp"
#include <atlbase.h>
#include <comdef.h>
#include <Wbemidl.h>
# pragma comment(lib, "wbemuuid.lib")
#include "Com.hpp"
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
		std::wostream&,
		ScriptSection const&,
		std::vector<std::wstring> const&
	) const
	{
		//Security Center
		SystemFacades::Com com;
		CComPtr<IWbemLocator> wmiLocator;
		SystemFacades::ThrowFromHResult(wmiLocator.CoCreateInstance(
			CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER));
		CComPtr<IWbemServices> wmiServices;
		SystemFacades::ThrowFromHResult(wmiLocator->ConnectServer(BSTR("root"), 0, 0, 0, 0, 0, 0, 0));
		CComPtr<IWbemServices> securityCenter, securityCenter2;
		SystemFacades::ThrowFromHResult(wmiServices->OpenNamespace(L"SecurityCenter", 0, 0, &securityCenter, 0));
	}

}
