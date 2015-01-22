// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include <windows.h>
#include <comdef.h>
#include <wbemidl.h>
#include "Win32Exception.hpp"
#include "Wmi.hpp"
#include "SecurityCenter.hpp"
#include "Com.hpp"

namespace Instalog
{
namespace SystemFacades
{

static const char avCode[] = "AV";
static const char fwCode[] = "FW";
static const char asCode[] = "AS";

static void SecCenterProductCheck(UniqueComPtr<IWbemServices>& securityCenter,
                                  BSTR productToCheck,
                                  std::vector<SecurityProduct>& result,
                                  char const* twoCode,
                                  wchar_t const* enabledPropertyName,
                                  wchar_t const* upToDatePropertyName = nullptr)
{
    HRESULT hr;
    UniqueComPtr<IEnumWbemClassObject> objEnumerator;
    hr = securityCenter->CreateInstanceEnum(productToCheck,
                                            WBEM_FLAG_FORWARD_ONLY,
                                            0,
                                            objEnumerator.PassAsOutParameter());
    if (hr == WBEM_E_INVALID_CLASS)
    {
        return; // Expected error on XP x64 machines
    }
    ThrowIfFailed(hr);
    ULONG returnCount = 0;
    for (;;)
    {
        UniqueComPtr<IWbemClassObject> obj;
        hr = objEnumerator->Next(
            WBEM_INFINITE, 1, obj.PassAsOutParameter(), &returnCount);
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
        UniqueVariant variant;
        ThrowIfFailed(
            obj->Get(L"instanceGuid", 0, variant.PassAsOutParameter(), 0, 0));
        std::wstring guid(variant.AsString(GetThrowingErrorReporter()));
        ThrowIfFailed(
            obj->Get(L"displayName", 0, variant.PassAsOutParameter(), 0, 0));
        std::wstring name(variant.AsString(GetThrowingErrorReporter()));

        ThrowIfFailed(obj->Get(
            enabledPropertyName, 0, variant.PassAsOutParameter(), 0, 0));
        bool productEnabled = variant.AsBool(GetThrowingErrorReporter());
        SecurityProduct::UpdateStatusValues updateStatus =
            SecurityProduct::UpdateNotRequired;
        if (upToDatePropertyName != nullptr)
        {
            ThrowIfFailed(obj->Get(
                upToDatePropertyName, 0, variant.PassAsOutParameter(), 0, 0));
            if (variant.AsBool(GetThrowingErrorReporter()))
            {
                updateStatus = SecurityProduct::UpToDate;
            }
            else
            {
                updateStatus = SecurityProduct::OutOfDate;
            }
        }
        result.emplace_back(std::move(name),
                            std::move(guid),
                            productEnabled,
                            updateStatus,
                            twoCode);
    }
}

static void SecCenter2ProductCheck(UniqueComPtr<IWbemServices>& securityCenter2,
                                   BSTR productToCheck,
                                   std::vector<SecurityProduct>& result,
                                   const char* twoCode)
{
    UniqueComPtr<IEnumWbemClassObject> objEnumerator;
    ThrowIfFailed(securityCenter2->CreateInstanceEnum(
        productToCheck,
        WBEM_FLAG_FORWARD_ONLY,
        0,
        objEnumerator.PassAsOutParameter()));
    ULONG returnCount = 0;
    for (;;)
    {
        HRESULT hr;
        UniqueComPtr<IWbemClassObject> obj;
        hr = objEnumerator->Next(
            WBEM_INFINITE, 1, obj.PassAsOutParameter(), &returnCount);
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
        ThrowIfFailed(
            obj->Get(L"instanceGuid", 0, variant.PassAsOutParameter(), 0, 0));
        std::wstring guid(variant.AsString(GetThrowingErrorReporter()));
        ThrowIfFailed(
            obj->Get(L"displayName", 0, variant.PassAsOutParameter(), 0, 0));
        std::wstring name(variant.AsString(GetThrowingErrorReporter()));
        ThrowIfFailed(
            obj->Get(L"productState", 0, variant.PassAsOutParameter(), 0, 0));
        UINT productState = variant.AsUint(GetThrowingErrorReporter());
        char productType =
            static_cast<char>((productState & 0x00FF0000ul) >> 16);
        char enabledBits =
            static_cast<char>((productState & 0x0000FF00ul) >> 8);
        char updateBits = productState & 0x000000FFul;
        SecurityProduct::UpdateStatusValues updateStatus;
        if ((productType & 2ul) == 0)
        {
            updateStatus = SecurityProduct::UpdateNotRequired;
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
        result.emplace_back(std::move(name),
                            std::move(guid),
                            enabledBits == 16,
                            updateStatus,
                            twoCode);
    }
}

static void CheckSecurityCenter(UniqueComPtr<IWbemServices>& wbemServices,
                                std::vector<SecurityProduct>& result)
{
    UniqueComPtr<IWbemServices> securityCenter;
    HRESULT errorCheck = wbemServices->OpenNamespace(
        BSTR(L"SecurityCenter"), 0, 0, securityCenter.PassAsOutParameter(), 0);
    // On versions of Windows prior to XP SP2, there is no security center to
    // query; so this would be
    // an expected failure.
    if (errorCheck == WBEM_E_INVALID_NAMESPACE)
    {
        return;
    }
    ThrowIfFailed(errorCheck);
    SecCenterProductCheck(securityCenter,
                          BSTR(L"AntiVirusProduct"),
                          result,
                          avCode,
                          BSTR(L"onAccessScanningEnabled"),
                          BSTR(L"productUpToDate"));
    SecCenterProductCheck(securityCenter,
                          BSTR(L"FireWallProduct"),
                          result,
                          fwCode,
                          BSTR(L"enabled"));
    SecCenterProductCheck(securityCenter,
                          BSTR(L"AntiSpywareProduct"),
                          result,
                          asCode,
                          BSTR(L"productEnabled"),
                          BSTR(L"productUpToDate"));
}
static bool CheckSecurityCenter2(UniqueComPtr<IWbemServices>& wbemServices,
                                 std::vector<SecurityProduct>& result)
{
    UniqueComPtr<IWbemServices> securityCenter2;
    HRESULT errorCheck = wbemServices->OpenNamespace(BSTR(L"SecurityCenter2"),
                                    0,
                                    0,
                                    securityCenter2.PassAsOutParameter(),
                                    0);
    if (errorCheck == WBEM_E_INVALID_NAMESPACE)
    {
        return true;
    }
    ThrowIfFailed(errorCheck);

    SecCenter2ProductCheck(
        securityCenter2, BSTR(L"AntiVirusProduct"), result, avCode);
    SecCenter2ProductCheck(
        securityCenter2, BSTR(L"FireWallProduct"), result, fwCode);
    SecCenter2ProductCheck(
        securityCenter2, BSTR(L"AntiSpywareProduct"), result, asCode);

    return false;
}

std::vector<SecurityProduct> EnumerateSecurityProducts()
{
    OSVERSIONINFOW version;
    version.dwOSVersionInfoSize = sizeof(version);
    std::vector<SecurityProduct> result;
    GetVersionExW(&version);
    UniqueComPtr<IWbemServices> wbemServices(GetWbemServices());
    if (CheckSecurityCenter2(wbemServices, result))
    {
        CheckSecurityCenter(wbemServices, result);
    }
    return result;
}

void SecurityProduct::Delete()
{
    UniqueComPtr<IWbemServices> wbemServices(GetWbemServices());
    UniqueComPtr<IWbemServices> securityCenter2;
    ThrowIfFailed(
        wbemServices->OpenNamespace(BSTR(L"SecurityCenter2"),
                                    0,
                                    0,
                                    securityCenter2.PassAsOutParameter(),
                                    0));
    std::wstring path;
    if (strcmp(GetTwoLetterPrefix(), avCode) == 0)
    {
        path = L"AntiVirusProduct";
    }
    else if (strcmp(GetTwoLetterPrefix(), fwCode) == 0)
    {
        path = L"FirewallProduct";
    }
    else if (strcmp(GetTwoLetterPrefix(), asCode) == 0)
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
    UniqueBstr guid(path);
    ThrowIfFailed(
        securityCenter2->DeleteInstance(guid.AsInput(), 0, nullptr, nullptr));
}
}
}
