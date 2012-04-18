#include "pch.hpp"
#include <iostream>
#define _WIN32_DCOM
#include <windows.h>
#include <comdef.h>
#include <wbemidl.h>
#include "Win32Exception.hpp"
#include "SecurityCenter.hpp"

namespace Instalog { namespace SystemFacades {

static const wchar_t avCode[] = L"AV";
static const wchar_t fwCode[] = L"FW";
static const wchar_t asCode[] = L"AS";

static void SecCenterProductCheck( 
	CComPtr<IWbemServices> securityCenter, 
	BSTR productToCheck, 
	std::vector<SecurityProduct> &result, 
	wchar_t const* twoCode ) 
{
	CComPtr<IEnumWbemClassObject> objEnumerator;
	ThrowIfFailed(securityCenter->CreateInstanceEnum(
		productToCheck,
		WBEM_FLAG_FORWARD_ONLY,
		0,
		&objEnumerator
		));
	ULONG returnCount = 0;
	for(;;)
	{
		HRESULT hr;
		CComPtr<IWbemClassObject> obj;
		hr = objEnumerator->Next(WBEM_INFINITE, 1, &obj, &returnCount);
		if (hr == WBEM_S_FALSE)
		{
			break;
		}
		else if (FAILED(hr))
		{
			throw _com_error(hr);
		}
		else if (returnCount == 0)
		{
			throw std::runtime_error("Unexpected number of returned classes.");
		}
		CComVariant variant;
		ThrowIfFailed(obj->Get(L"instanceGuid",0,&variant,0,0));
		ThrowIfFailed(variant.ChangeType(VT_BSTR));
		std::wstring guid(variant.bstrVal, SysStringLen(variant.bstrVal));
		variant.Clear();
		ThrowIfFailed(obj->Get(L"displayName",0,&variant,0,0));
		std::wstring name(variant.bstrVal, SysStringLen(variant.bstrVal));
		ThrowIfFailed(variant.ChangeType(VT_BSTR));

		ThrowIfFailed(obj->Get(L"onAccessScanningEnabled",0,&variant,0,0));
		ThrowIfFailed(variant.ChangeType(VT_BOOL));
		bool onAccessEnabled = variant.boolVal != 0;
		ThrowIfFailed(obj->Get(L"productUpToDate", 0, &variant, 0, 0));
		ThrowIfFailed(variant.ChangeType(VT_BOOL));
		SecurityProduct::UpdateStatusValues updateStatus;
		if (variant.boolVal)
		{
			updateStatus = SecurityProduct::UpToDate;
		}
		else
		{
			updateStatus = SecurityProduct::OutOfDate;
		}
		result.push_back(SecurityProduct(
			name,
			guid,
			onAccessEnabled,
			updateStatus,
			twoCode));
	}
}

static void SecCenter2ProductCheck( 
	CComPtr<IWbemServices> securityCenter2, 
	BSTR productToCheck, 
	std::vector<SecurityProduct> &result, 
	const wchar_t * twoCode ) 
{
	CComPtr<IEnumWbemClassObject> objEnumerator;
	ThrowIfFailed(securityCenter2->CreateInstanceEnum(
		productToCheck,
		WBEM_FLAG_FORWARD_ONLY,
		0,
		&objEnumerator
		));
	ULONG returnCount = 0;
	for(;;)
	{
		HRESULT hr;
		CComPtr<IWbemClassObject> obj;
		hr = objEnumerator->Next(WBEM_INFINITE, 1, &obj, &returnCount);
		if (hr == WBEM_S_FALSE)
		{
			break;
		}
		else if (FAILED(hr))
		{
			throw _com_error(hr);
		}
		else if (returnCount == 0)
		{
			throw std::runtime_error("Unexpected number of returned classes.");
		}
		CComVariant variant;
		ThrowIfFailed(obj->Get(L"instanceGuid",0,&variant,0,0));
		ThrowIfFailed(variant.ChangeType(VT_BSTR));
		std::wstring guid(variant.bstrVal, SysStringLen(variant.bstrVal));
		variant.Clear();
		ThrowIfFailed(obj->Get(L"displayName",0,&variant,0,0));
		std::wstring name(variant.bstrVal, SysStringLen(variant.bstrVal));
		ThrowIfFailed(variant.ChangeType(VT_BSTR));
		ThrowIfFailed(obj->Get(L"productState",0,&variant,0,0));
		ThrowIfFailed(variant.ChangeType(VT_UINT));
		UINT32 productState = variant.uintVal;
		char productType = static_cast<char>(
			(productState & 0x00FF0000ul) >> 16);
		char enabledBits = static_cast<char>(
			(productState & 0x0000FF00ul) >> 8);
		char updateBits = productState & 0x000000FFul;
		SecurityProduct::UpdateStatusValues updateStatus;
		if ((productType & 2ul) == 0)
		{
			updateStatus =  SecurityProduct::UpdateNotRequired;
		}
		else
		{
			if (updateBits == 0)
			{
				updateStatus = SecurityProduct::UpToDate;
			}
			else
			{
				updateStatus = SecurityProduct::OutOfDate;
			}
		}
		result.push_back(SecurityProduct(
			name,
			guid,
			enabledBits == 16,
			updateStatus,
			twoCode));
	}
}

static void CheckSecurityCenter( CComPtr<IWbemServices> wbemServices, std::vector<SecurityProduct>& result )
{
	CComPtr<IWbemServices> securityCenter;
	ThrowIfFailed(wbemServices->OpenNamespace(
		BSTR(L"SecurityCenter"),0,0,&securityCenter,0));
	SecCenterProductCheck(securityCenter, BSTR(L"AntiVirusProduct"), result, 
		avCode);
	SecCenterProductCheck(securityCenter, BSTR(L"FireWallProduct"), result,
		fwCode);
	SecCenterProductCheck(securityCenter, BSTR(L"AntiSpywareProduct"), result, 
		asCode);
}
static void CheckSecurityCenter2( CComPtr<IWbemServices> wbemServices,
	std::vector<SecurityProduct>& result )
{
	CComPtr<IWbemServices> securityCenter2;
	ThrowIfFailed(wbemServices->OpenNamespace(
		BSTR(L"SecurityCenter2"),0,0,&securityCenter2,0));
	SecCenter2ProductCheck(securityCenter2, BSTR(L"AntiVirusProduct"), result, 
		avCode);
	SecCenter2ProductCheck(securityCenter2, BSTR(L"FireWallProduct"), result,
		fwCode);
	SecCenter2ProductCheck(securityCenter2, BSTR(L"AntiSpywareProduct"), result, 
		asCode);
}


std::vector<SecurityProduct> EnumerateSecurityProducts()
{
	OSVERSIONINFOW version;
	version.dwOSVersionInfoSize = sizeof(version);
	std::vector<SecurityProduct> result;
	GetVersionExW(&version);
	CComPtr<IWbemServices> wbemServices(GetWbemServices());
	if (version.dwMajorVersion >= 6)
	{
		CheckSecurityCenter2(wbemServices, result);
	}
	CheckSecurityCenter(wbemServices, result);	

	return result;
}
std::wostream& operator<<( std::wostream& lhs, const SecurityProduct& rhs )
{
	lhs << rhs.GetTwoLetterPrefix() << L": " << rhs.GetName();
	if (rhs.IsEnabled())
	{
		lhs << L" (Enabled";
	}
	else
	{
		lhs << L" (Disabled";
	}
	switch (rhs.GetUpdateStatus())
	{
	case SecurityProduct::OutOfDate:
		lhs << L"/Out Of Date) ";
		break;
	case SecurityProduct::UpToDate:
		lhs << L"/Up To Date) ";
		break;
	case SecurityProduct::UpdateNotRequired:
		lhs << L") ";
		break;
	}
	lhs << rhs.GetInstanceGuid() << std::endl;
	return lhs;
}

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
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		0,
		EOAC_NONE
		));
	return wbemServices;
}
void SecurityProduct::Delete()
{
	CComPtr<IWbemServices> wbemServices(GetWbemServices());
	CComPtr<IWbemServices> securityCenter2;
	ThrowIfFailed(wbemServices->OpenNamespace(
		BSTR(L"SecurityCenter2"),0,0,&securityCenter2,0));
	std::wstring path;
	if (wcscmp(GetTwoLetterPrefix(), avCode) == 0)
	{
		path = L"AntiVirusProduct";
	}
	else if (wcscmp(GetTwoLetterPrefix(), fwCode) == 0)
	{
		path = L"FirewallProduct";
	}
	else if (wcscmp(GetTwoLetterPrefix(), asCode) == 0)
	{
		path = L"AntiSpywareProduct";
	}
	else
	{
		throw std::exception("Invalid product type");
	}

	path.append(L".instanceGuid=\"");
	path.append(guid_);
	path.push_back(L'"');
	CComBSTR guid(path.c_str());
	ThrowIfFailed(securityCenter2->DeleteInstance(guid, 0, nullptr, nullptr));
}

}}

