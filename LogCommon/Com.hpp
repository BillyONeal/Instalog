// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.
#include <Windows.h>
#include "Win32Exception.hpp"

namespace Instalog { namespace SystemFacades {
	struct Com
	{
		Com(DWORD threadingType = COINIT_APARTMENTTHREADED)
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
		~Com()
		{
			::CoUninitialize();
		}
	};
}}
