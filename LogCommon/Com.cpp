#include "Com.hpp"
#include <cassert>

namespace Instalog
{
namespace SystemFacades
{

Com::Com(DWORD threadingType, IErrorReporter& errorReporter)
    : setupComplete(false)
{
    HRESULT hr;
    hr = ::CoInitializeEx(nullptr, threadingType);
    if (FAILED(hr))
    {
        errorReporter.ReportHresult(hr, "CoInitializeEx");
        return;
    }

    hr = ::CoInitializeSecurity(
        NULL,
        -1,                          // COM negotiates service
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // authentication
        RPC_C_IMP_LEVEL_IMPERSONATE, // Impersonation
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities
        NULL                         // Reserved
        );

    if (FAILED(hr))
    {
        errorReporter.ReportHresult(hr, "CoInitializeSecurity");
        return;
    }

    this->setupComplete = true;
}

Com::~Com()
{
    if (this->setupComplete)
    {
        ::CoUninitialize();
    }
}

UniqueBstr::UniqueBstr() : wrapped(nullptr)
{
}

UniqueBstr::UniqueBstr(std::wstring const& source)
{
    if (source.empty())
    {
        this->wrapped = nullptr;
        return;
    }

    assert(source.size() <= std::numeric_limits<UINT>::max());
    UINT lengthPrefix = static_cast<UINT>(source.size());
    this->wrapped = ::SysAllocStringLen(source.data(), lengthPrefix);
    if (this->wrapped == nullptr)
    {
        throw std::bad_alloc();
    }
}

UniqueBstr::UniqueBstr(UniqueBstr&& other) : wrapped(other.wrapped)
{
    other.wrapped = nullptr;
}

BSTR UniqueBstr::AsInput() const
{
    return this->wrapped;
}

BSTR* UniqueBstr::AsTarget()
{
    ::SysFreeString(this->wrapped);
    this->wrapped = nullptr;
    return &this->wrapped;
}

uint32_t UniqueBstr::Length()
{
    return ::SysStringLen(this->wrapped);
}

std::wstring UniqueBstr::AsString()
{
    return std::wstring(this->wrapped, this->Length());
}

UniqueBstr::~UniqueBstr()
{
    ::SysFreeString(this->wrapped);
}

#pragma warning(push)
#pragma warning(disable: 4189) // local variable is initialized but not referenced
// (The assert gets compiled out in release mode leading to the spurrious
// warning)
void UniqueVariant::Destroy()
{
    HRESULT result = ::VariantClear(&wrappedVariant);
    assert(result == S_OK);
}
#pragma warning(pop)

UniqueVariant::UniqueVariant()
{
    ::VariantInit(&this->wrappedVariant);
}

VARIANT* UniqueVariant::PassAsOutParameter()
{
    this->Destroy();
    ::VariantInit(&this->wrappedVariant);
    return &this->wrappedVariant;
}

VARIANT& UniqueVariant::Get()
{
    return this->wrappedVariant;
}

VARIANT const& UniqueVariant::Get() const
{
    return this->wrappedVariant;
}

std::wstring UniqueVariant::AsString(IErrorReporter& errorReporter) const
{
    UniqueVariant bstrVariant;
    HRESULT hr = ::VariantChangeType(
        bstrVariant.PassAsOutParameter(), &this->wrappedVariant, 0, VT_BSTR);

    BSTR asBstr;
    std::size_t bstrLen;
    if (SUCCEEDED(hr))
    {
        asBstr = bstrVariant.Get().bstrVal;
        bstrLen = ::SysStringLen(asBstr);
    }
    else
    {
        errorReporter.ReportHresult(hr, "VariantChangeType (string)");
        asBstr = nullptr;
        bstrLen = 0;
    }

    return std::wstring(asBstr, bstrLen);
}

UINT UniqueVariant::AsUint(IErrorReporter& errorReporter) const
{
    UniqueVariant uintVariant;
    HRESULT hr = ::VariantChangeType(
        uintVariant.PassAsOutParameter(), &this->wrappedVariant, 0, VT_UINT);
    if (FAILED(hr))
    {
        errorReporter.ReportHresult(hr, "VariantChangeType (unsigned int)");
        return 0u;
    }

    return uintVariant.Get().uintVal;
}

ULONG UniqueVariant::AsUlong(IErrorReporter& errorReporter) const
{
    UniqueVariant ulongVariant;
    HRESULT hr = ::VariantChangeType(
        ulongVariant.PassAsOutParameter(), &this->wrappedVariant, 0, VT_UI4);
    if (FAILED(hr))
    {
        errorReporter.ReportHresult(hr, "VariantChangeType (unsigned long)");
        return 0ul;
    }

    return ulongVariant.Get().ulVal;
}

ULONGLONG UniqueVariant::AsUlonglong(IErrorReporter& errorReporter) const
{
    UniqueVariant ulongVariant;

    HRESULT hr = ::VariantChangeType(
        ulongVariant.PassAsOutParameter(), &this->wrappedVariant, 0, VT_UI8);
    if (FAILED(hr))
    {
        errorReporter.ReportHresult(hr, "VariantChangeType (unsigned long long)");
        return 0ull;
    }

    return ulongVariant.Get().ullVal;
}

bool UniqueVariant::AsBool(IErrorReporter& errorReporter) const
{
    UniqueVariant booleanVariant;

    HRESULT hr = ::VariantChangeType(
        booleanVariant.PassAsOutParameter(), &this->wrappedVariant, 0, VT_BOOL);
    if (FAILED(hr))
    {
        errorReporter.ReportHresult(hr, "VariantChangeType (bool)");
        return false;
    }

    return booleanVariant.Get().boolVal != 0;
}

bool UniqueVariant::IsNull() const
{
    return this->wrappedVariant.vt == VT_NULL;
}

UniqueVariant::~UniqueVariant()
{
    this->Destroy();
}
}
} // namespace Instalog::SystemFacades
