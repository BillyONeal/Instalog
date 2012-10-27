// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.
#include <cstring>
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

    class UniqueBstr
    {
        BSTR wrapped;
        UniqueBstr(UniqueBstr const&);
        UniqueBstr& operator=(UniqueBstr const&);
    public:
        UniqueBstr()
            : wrapped(nullptr)
        { }
        UniqueBstr(std::wstring const& source)
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
        UniqueBstr(UniqueBstr && other)
            : wrapped(other.wrapped)
        {
            other.wrapped = nullptr;
        }
        BSTR AsInput() const
        {
            return this->wrapped;
        }
        BSTR* AsTarget()
        {
            ::SysFreeString(this->wrapped);
            this->wrapped = nullptr;
        }
        wchar_t * AsNullTerminated()
        {
            return this->wrapped;
        }
        uint32_t Length()
        {
            return ::SysStringLen(this->wrapped);
        }
        std::wstring AsString()
        {
            return std::wstring(this->wrapped, this->Length());
        }
        ~UniqueBstr()
        {
            ::SysFreeString(this->wrapped);
        }
    };
}}
