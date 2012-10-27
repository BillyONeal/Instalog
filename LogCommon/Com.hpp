// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.
#pragma once
#include <cstring>
#include <boost/noncopyable.hpp>
#include <Windows.h>
#include "Win32Exception.hpp"

namespace Instalog { namespace SystemFacades {

	struct Com : boost::noncopyable
	{
        /// <summary>Initializes a new instance of the Com class. Initializes the Component Object Model.</summary>
		Com(DWORD threadingType = COINIT_APARTMENTTHREADED);

        /// <summary>Finalizes an instance of the Com class. Shuts down Component Object Model.</summary>
		~Com();
	};

    /// <summary>
    /// Unique com pointer. Similar to ATL's CComPtr, except allows only a unique reference with move
    /// semantics, and plays nicer with auto.
    /// </summary>
    /// <typeparam name="T">The COM interface wrapped by this UniqueComPtr.</typeparam>
    template <typename T>
    class UniqueComPtr
    {
        T* pointer;
        UniqueComPtr(UniqueComPtr const&);
        UniqueComPtr& operator=(UniqueComPtr const&);
    public:
        T* operator->()
        {
            return this->pointer;
        }

        UniqueComPtr()
            : pointer(nullptr)
        { }

        UniqueComPtr(T* wrap)
            : pointer(wrap)
        { }

        UniqueComPtr(UniqueComPtr && other)
            : pointer(other.pointer)
        {
            other.pointer = nullptr;
        }

        ~UniqueComPtr()
        {
            if (this->pointer != nullptr)
            {
                this->pointer->Release();
            }
        }

        static UniqueComPtr<T> Create(REFCLSID clsid, DWORD clsCtx)
        {
            T* resultPtr;
            ThrowIfFailed(::CoCreateInstance(clsid, nullptr, clsCtx, __uuidof(T), reinterpret_cast<void**>(&resultPtr)));
            return UniqueComPtr<T>(resultPtr);
        }

        T* Get()
        {
            return this->pointer;
        }

        T const* Get() const
        {
            return this->pointer;
        }

        T** PassAsOutParameter()
        {
            if (this->pointer != nullptr)
            {
                this->pointer->Release();
                this->pointer = nullptr;
            }
            return &this->pointer;
        }
    };

    class UniqueBstr
    {
        BSTR wrapped;
        UniqueBstr(UniqueBstr const&);
        UniqueBstr& operator=(UniqueBstr const&);
    public:
        UniqueBstr();
        UniqueBstr(std::wstring const& source);
        UniqueBstr(UniqueBstr && other);
        BSTR AsInput() const;
        BSTR* AsTarget();
        uint32_t Length();
        std::wstring AsString();
        ~UniqueBstr();
    };

    class UniqueVariant
    {
        VARIANT wrappedVariant;
        UniqueVariant(UniqueVariant const&);
        UniqueVariant& operator=(UniqueVariant const&);
        void Destroy()
        {
            HRESULT result = ::VariantClear(&wrappedVariant);
            assert(result == S_OK);
        }
    public:
        UniqueVariant()
        {
            ::VariantInit(&this->wrappedVariant);
        }
        VARIANT* PassAsOutParameter()
        {
            this->Destroy();
            ::VariantInit(&this->wrappedVariant);
            return &this->wrappedVariant;
        }
        VARIANT& Get()
        {
            return this->wrappedVariant;
        }
        VARIANT const& Get() const
        {
            return this->wrappedVariant;
        }
        std::wstring AsString() const
        {
            UniqueVariant bstrVariant;
            ThrowIfFailed(::VariantChangeType(bstrVariant.PassAsOutParameter(), &this->wrappedVariant, 0, VT_BSTR));
            BSTR asBstr = bstrVariant.Get().bstrVal;
            return std::wstring(asBstr, ::SysStringLen(asBstr));
        }
        UINT AsUint() const
        {
            UniqueVariant uintVariant;
            ThrowIfFailed(::VariantChangeType(uintVariant.PassAsOutParameter(), &this->wrappedVariant, 0, VT_UINT));
            return uintVariant.Get().uintVal;
        }
        ULONG AsUlong() const
        {
            UniqueVariant ulongVariant;
            ThrowIfFailed(::VariantChangeType(ulongVariant.PassAsOutParameter(), &this->wrappedVariant, 0, VT_UI4));
            return ulongVariant.Get().ulVal;
        }
        ULONGLONG AsUlonglong() const
        {
            UniqueVariant ulongVariant;
            ThrowIfFailed(::VariantChangeType(ulongVariant.PassAsOutParameter(), &this->wrappedVariant, 0, VT_UI8));
            return ulongVariant.Get().ullVal;
        }
        bool AsBool() const
        {
            UniqueVariant booleanVariant;
            ThrowIfFailed(::VariantChangeType(booleanVariant.PassAsOutParameter(), &this->wrappedVariant, 0, VT_BOOL));
            return booleanVariant.Get().boolVal != 0;
        }
        ~UniqueVariant()
        {
            this->Destroy();
        }
    };
}}
