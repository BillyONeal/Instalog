#include "pch.hpp"
#include "Com.hpp"

namespace Instalog { namespace SystemFacades {

Com::Com(DWORD threadingType /* = COINIT_APARTMENTTHREADED */)
{
    ThrowIfFailed(::CoInitializeEx(nullptr, threadingType));
    ThrowIfFailed(::CoInitializeSecurity(
        NULL,     
        -1,      // COM negotiates service                  
        NULL,    // Authentication services
        NULL,    // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,    // authentication
        RPC_C_IMP_LEVEL_IMPERSONATE,  // Impersonation
        NULL,             // Authentication info 
        EOAC_NONE,        // Additional capabilities
        NULL              // Reserved
        ));
}

Com::~Com()
{
    ::CoUninitialize();
}

UniqueBstr::UniqueBstr() : wrapped(nullptr)
{ }

UniqueBstr::UniqueBstr( std::wstring const& source )
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

UniqueBstr::UniqueBstr( UniqueBstr && other ) : wrapped(other.wrapped)
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
// (The assert gets compiled out in release mode leading to the spurrious warning)
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

std::wstring UniqueVariant::AsString() const
{
    UniqueVariant bstrVariant;
    ThrowIfFailed(::VariantChangeType(bstrVariant.PassAsOutParameter(), &this->wrappedVariant, 0, VT_BSTR));
    BSTR asBstr = bstrVariant.Get().bstrVal;
    return std::wstring(asBstr, ::SysStringLen(asBstr));
}

UINT UniqueVariant::AsUint() const
{
    UniqueVariant uintVariant;
    ThrowIfFailed(::VariantChangeType(uintVariant.PassAsOutParameter(), &this->wrappedVariant, 0, VT_UINT));
    return uintVariant.Get().uintVal;
}

ULONG UniqueVariant::AsUlong() const
{
    UniqueVariant ulongVariant;
    ThrowIfFailed(::VariantChangeType(ulongVariant.PassAsOutParameter(), &this->wrappedVariant, 0, VT_UI4));
    return ulongVariant.Get().ulVal;
}

ULONGLONG UniqueVariant::AsUlonglong() const
{
    UniqueVariant ulongVariant;
    ThrowIfFailed(::VariantChangeType(ulongVariant.PassAsOutParameter(), &this->wrappedVariant, 0, VT_UI8));
    return ulongVariant.Get().ullVal;
}

bool UniqueVariant::AsBool() const
{
    UniqueVariant booleanVariant;
    ThrowIfFailed(::VariantChangeType(booleanVariant.PassAsOutParameter(), &this->wrappedVariant, 0, VT_BOOL));
    return booleanVariant.Get().boolVal != 0;
}

UniqueVariant::~UniqueVariant()
{
    this->Destroy();
}

}} // namespace Instalog::SystemFacades
