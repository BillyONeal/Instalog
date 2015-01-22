// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.
#pragma once
#include <cstring>
#include <cstdint>
#include <string>
#include <boost/noncopyable.hpp>
#include <Windows.h>
#include "ErrorReporter.hpp"

namespace Instalog
{
namespace SystemFacades
{

class Com : boost::noncopyable
{
    bool setupComplete;
public:
    /// <summary>Initializes a new instance of the Com class. Initializes the
    /// Component Object Model.</summary>
    Com(DWORD threadingType, IErrorReporter& errorReporter);

    /// <summary>Finalizes an instance of the Com class. Shuts down Component
    /// Object Model.</summary>
    ~Com();
};

/// <summary>
/// Unique com pointer. Similar to ATL's CComPtr, except allows only a unique
/// reference with move
/// semantics, and plays nicer with auto.
/// </summary>
/// <typeparam name="T">The COM interface wrapped by this
/// UniqueComPtr.</typeparam>
template <typename T> class UniqueComPtr
{
    T* pointer;
    UniqueComPtr(UniqueComPtr const&);
    UniqueComPtr& operator=(UniqueComPtr const&);

    public:
    T* operator->()
    {
        return this->pointer;
    }

    UniqueComPtr() : pointer(nullptr)
    {
    }

    explicit UniqueComPtr(T* wrap) : pointer(wrap)
    {
    }

    UniqueComPtr(UniqueComPtr&& other) : pointer(other.pointer)
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

    static UniqueComPtr<T> Create(REFCLSID clsid, DWORD clsCtx, IErrorReporter& errorReporter)
    {
        T* resultPtr;
        HRESULT hr = ::CoCreateInstance(clsid,
                                         nullptr,
                                         clsCtx,
                                         __uuidof(T),
                                         reinterpret_cast<void**>(&resultPtr));
        if (FAILED(hr))
        {
            errorReporter.ReportHresult(hr, "CoCreateInstance");
            resultPtr = nullptr;
        }

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
    UniqueBstr(UniqueBstr&& other);
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
    void Destroy();

    public:
    UniqueVariant();
    VARIANT* PassAsOutParameter();
    VARIANT& Get();
    VARIANT const& Get() const;

    // Convert this variant into the indicated type. If an error is
    // reported, and the IErrorReporter implementation supplied ignores it,
    // a default value (e.g. zero for numbers, empty string, false) is returned.
    std::wstring AsString(IErrorReporter&) const;
    UINT AsUint(IErrorReporter&) const;
    ULONG AsUlong(IErrorReporter&) const;
    ULONGLONG AsUlonglong(IErrorReporter&) const;
    bool AsBool(IErrorReporter&) const;
    bool IsNull() const;
    ~UniqueVariant();
};
}
}
