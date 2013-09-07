// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <stdlib.h>
#include "Wmi.hpp"
#include "Win32Exception.hpp"
#include "RestorePoints.hpp"

namespace Instalog
{
namespace SystemFacades
{

std::vector<RestorePoint> EnumerateRestorePoints()
{
    UniqueComPtr<IWbemServices> wbemServices(GetWbemServices());

    UniqueComPtr<IWbemServices> namespaceDefault;
    ThrowIfFailed(
        wbemServices->OpenNamespace(BSTR(L"default"),
                                    0,
                                    NULL,
                                    namespaceDefault.PassAsOutParameter(),
                                    NULL));

    UniqueComPtr<IEnumWbemClassObject> systemRestoreEnumerator;
    ThrowIfFailed(namespaceDefault->CreateInstanceEnum(
        BSTR(L"SystemRestore"),
        WBEM_FLAG_FORWARD_ONLY,
        NULL,
        systemRestoreEnumerator.PassAsOutParameter()));

    std::vector<RestorePoint> restorePoints;

    for (;;)
    {
        HRESULT hr;
        UniqueComPtr<IWbemClassObject> systemRestore;
        ULONG returnCount = 0;
        hr = systemRestoreEnumerator->Next(
            WBEM_INFINITE, 1, systemRestore.PassAsOutParameter(), &returnCount);
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

        UniqueVariant variant;
        RestorePoint restorePoint;

        ThrowIfFailed(systemRestore->Get(
            L"Description", 0, variant.PassAsOutParameter(), NULL, NULL));
        restorePoint.Description = variant.AsString();
        ThrowIfFailed(systemRestore->Get(
            L"CreationTime", 0, variant.PassAsOutParameter(), NULL, NULL));
        restorePoint.CreationTime = variant.AsString();

        ThrowIfFailed(systemRestore->Get(
            L"RestorePointType", 0, variant.PassAsOutParameter(), NULL, NULL));
        restorePoint.RestorePointType = variant.AsUint();
        ThrowIfFailed(systemRestore->Get(
            L"EventType", 0, variant.PassAsOutParameter(), NULL, NULL));
        restorePoint.EventType = variant.AsUint();
        ThrowIfFailed(systemRestore->Get(
            L"SequenceNumber", 0, variant.PassAsOutParameter(), NULL, NULL));
        restorePoint.SequenceNumber = variant.AsUint();

        restorePoints.push_back(restorePoint);
    }

    return restorePoints;
}
}
}