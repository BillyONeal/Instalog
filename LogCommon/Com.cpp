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

}} // namespace Instalog::SystemFacades
