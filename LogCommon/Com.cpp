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

}} // namespace Instalog::SystemFacades
