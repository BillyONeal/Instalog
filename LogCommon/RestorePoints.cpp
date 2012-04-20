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

	SYSTEMTIME RestorePoint::CreationTimeAsSystemTime()
	{
		SYSTEMTIME utcTime;
		utcTime.wYear	= static_cast<WORD>(_wtoi(CreationTime.substr(0, 4).c_str()));
		utcTime.wMonth	= static_cast<WORD>(_wtoi(CreationTime.substr(4, 2).c_str()));
		utcTime.wDay	= static_cast<WORD>(_wtoi(CreationTime.substr(6, 2).c_str()));
		utcTime.wHour	= static_cast<WORD>(_wtoi(CreationTime.substr(8, 2).c_str()));
		utcTime.wMinute	= static_cast<WORD>(_wtoi(CreationTime.substr(10, 2).c_str()));
		utcTime.wSecond	= static_cast<WORD>(_wtoi(CreationTime.substr(12, 2).c_str()));
		utcTime.wMilliseconds = 0;

		SYSTEMTIME localTime;
		if (SystemTimeToTzSpecificLocalTime(NULL, &utcTime, &localTime) == false)
		{
			Win32Exception::ThrowFromLastError();
		}

		return localTime;
	}

}}