#include "pch.hpp"
#include "Win32Exception.hpp"
#include "Wmi.hpp"

namespace Instalog { namespace SystemFacades {

	CComPtr<IWbemServices> GetWbemServices()
	{
		CComPtr<IWbemLocator> locator;
		ThrowIfFailed(locator.CoCreateInstance(CLSID_WbemLocator, 0, 
			CLSCTX_INPROC_SERVER));
		CComPtr<IWbemServices> wbemServices;
		ThrowIfFailed(locator->ConnectServer(BSTR(L"ROOT"),0,0,0,0,0,0,&wbemServices));
		ThrowIfFailed(CoSetProxyBlanket(
			wbemServices,
			RPC_C_AUTHN_WINNT,
			RPC_C_AUTHZ_NONE,
			0,
			RPC_C_AUTHN_LEVEL_DEFAULT,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			0,
			EOAC_NONE
			));
		return wbemServices;
	}

}}