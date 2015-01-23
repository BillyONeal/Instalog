// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <boost/noncopyable.hpp>
#include "Library.hpp"
#include "Win32Exception.hpp"

#pragma once

namespace Instalog
{
namespace SystemFacades
{

/// @brief    Query if the current process is running under WOW64.
///
/// @return    true if running under WOW64, false otherwise
inline bool IsWow64()
{
    try
    {
        typedef BOOL(WINAPI * IsWow64ProcessT)(__in HANDLE hProcess,
                                               __out PBOOL Wow64Process);
        auto IsWow64ProcessFunc =
            GetKernel32().GetProcAddress<IsWow64ProcessT>(GetThrowingErrorReporter(), "IsWow64Process");
        BOOL answer;
        BOOL errorCheck = IsWow64ProcessFunc(GetCurrentProcess(), &answer);
        if (errorCheck == 0)
        {
            Win32Exception::ThrowFromLastError();
        }
        return answer != 0;
    }
    catch (ErrorProcedureNotFoundException const&)
    {
        return false;
    }
}

struct NativeFilePathScope : boost::noncopyable
{
    NativeFilePathScope()
        : ptr(nullptr)
    {
        try
        {
            typedef BOOL(WINAPI * IsWow64ProcessT)(__in HANDLE hProcess,
                                                   __out PBOOL Wow64Process);
            auto IsWow64ProcessFunc =
                GetKernel32().GetProcAddress<IsWow64ProcessT>(
                    GetThrowingErrorReporter(),
                    "IsWow64Process");
            BOOL isWow64;
            BOOL errorCheck =
                IsWow64ProcessFunc(::GetCurrentProcess(), &isWow64);
            if (errorCheck == 0)
            {
                Win32Exception::ThrowFromLastError();
            }

            if (isWow64 == 0)
            {
                return;
            }

            typedef BOOL(WINAPI * Wow64DisableWow64FsRedirectionFunc)(void**);
            auto disableFunc =
                GetKernel32()
                    .GetProcAddress<Wow64DisableWow64FsRedirectionFunc>(
                        GetThrowingErrorReporter(),
                        "Wow64DisableWow64FsRedirection");
            errorCheck = disableFunc(&this->ptr);
            if (errorCheck == 0)
            {
                Win32Exception::ThrowFromLastError();
            }
        }
        catch (ErrorProcedureNotFoundException const&)
        {
            // That's okay :)
            this->ptr = nullptr;
        }
    }

    void Revert()
    {
        try
        {
            typedef BOOL(WINAPI * Wow64RevertWow64FsRedirectionFunc)(void*);
            auto revertFunc =
                GetKernel32().GetProcAddress<Wow64RevertWow64FsRedirectionFunc>(
                        GetThrowingErrorReporter(),
                        "Wow64RevertWow64FsRedirection");
            revertFunc(this->ptr);
            this->ptr = nullptr;
        }
        catch (ErrorProcedureNotFoundException const&)
        {
            // That's okay :)
        }
    }

    ~NativeFilePathScope()
    {
        this->Revert();
    }

    private:
    void* ptr;
};
}
}
