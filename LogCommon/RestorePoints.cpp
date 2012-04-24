// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <stdlib.h>
#include "Wmi.hpp"
#include "Win32Exception.hpp"
#include "RestorePoints.hpp"

namespace Instalog { namespace SystemFacades {

	std::vector<RestorePoint> EnumerateRestorePoints()
	{
		CComPtr<IWbemServices> wbemServices(GetWbemServices());

		CComPtr<IWbemServices> namespaceDefault;
		ThrowIfFailed(wbemServices->OpenNamespace(
			BSTR(L"default"), 0, NULL, &namespaceDefault, NULL));

		CComPtr<IEnumWbemClassObject> systemRestoreEnumerator;
		ThrowIfFailed(namespaceDefault->CreateInstanceEnum(
			BSTR(L"SystemRestore"), WBEM_FLAG_FORWARD_ONLY, NULL, &systemRestoreEnumerator));

		std::vector<RestorePoint> restorePoints;

		for (;;)
		{
			HRESULT hr;
			CComPtr<IWbemClassObject> systemRestore;
			ULONG returnCount = 0;
			hr = systemRestoreEnumerator->Next(WBEM_INFINITE, 1, &systemRestore, &returnCount);
			if (hr == WBEM_S_FALSE)
			{
				break;
			}
			else if (FAILED(hr))
			{
				ThrowFromHResult(hr);
			}
			else if (returnCount == 0)
			{
				throw std::runtime_error("Unexpected number of returned classes.");
			}

			CComVariant variant;
			RestorePoint restorePoint;
			
			ThrowIfFailed(variant.ChangeType(VT_BSTR));
			ThrowIfFailed(systemRestore->Get(L"Description", 0, &variant, NULL, NULL));
			restorePoint.Description = std::wstring(variant.bstrVal, SysStringLen(variant.bstrVal));
			variant.Clear();
			ThrowIfFailed(systemRestore->Get(L"CreationTime", 0, &variant, NULL, NULL));
			restorePoint.CreationTime = std::wstring(variant.bstrVal, SysStringLen(variant.bstrVal));
			variant.Clear();

			ThrowIfFailed(variant.ChangeType(VT_UINT));
			ThrowIfFailed(systemRestore->Get(L"RestorePointType", 0, &variant, NULL, NULL));
			restorePoint.RestorePointType = variant.uintVal;
			variant.Clear();
			ThrowIfFailed(systemRestore->Get(L"EventType", 0, &variant, NULL, NULL));
			restorePoint.EventType = variant.uintVal;
			variant.Clear();
			ThrowIfFailed(systemRestore->Get(L"SequenceNumber", 0, &variant, NULL, NULL));
			restorePoint.SequenceNumber = variant.uintVal;
			variant.Clear();

			restorePoints.push_back(restorePoint);
		}

		return restorePoints;
	}

}}