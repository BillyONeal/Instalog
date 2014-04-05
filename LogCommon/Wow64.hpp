// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include "Library.hpp"

#pragma once

namespace Instalog
{

/// @brief    Query if the current process is running under WOW64.
///
/// @return    true if running under WOW64, false otherwise
inline bool IsWow64(IErrorReporter& errorReporter)
{
    typedef BOOL(WINAPI * IsWow64ProcessT)(__in HANDLE hProcess,
                                            __out PBOOL Wow64Process);
    auto IsWow64ProcessFunc = library::kernel32().get_function<
        IsWow64ProcessT>(GetIgnoreReporter(), "IsWow64Process");
    if (IsWow64ProcessFunc == nullptr)
    {
        DWORD const actualError = ::GetLastError();
        if (actualError != ERROR_PROC_NOT_FOUND)
        {
            errorReporter.ReportWinError(actualError, "GetProcAddress");
        }

        return false;
    }

    BOOL answer;
    BOOL errorCheck = IsWow64ProcessFunc(::GetCurrentProcess(), &answer);
    if (errorCheck == 0)
    {
        errorReporter.ReportWinError(::GetLastError(), "IsWow64Process");
    }

    return answer != 0;
}

struct NativeFilePathScope
{
    NativeFilePathScope(IErrorReporter& errorReporter)
        : ptr(nullptr)
    {
        if (!IsWow64(errorReporter))
        {
            return;
        }

        char const wow64FuncName[] = "Wow64DisableWow64FsRedirection";
        typedef BOOL(WINAPI * Wow64DisableWow64FsRedirectionFunc)(void**);
        auto disableFunc = library::kernel32().get_function<
            Wow64DisableWow64FsRedirectionFunc>(
            GetIgnoreReporter(),
            wow64FuncName
            );

        if (disableFunc == nullptr)
        {
            DWORD const actualError = ::GetLastError();
            if (actualError != ERROR_PROC_NOT_FOUND)
            {
                errorReporter.ReportWinError(actualError, wow64FuncName)
            }

            return;
        }

        BOOL const errorCheck = disableFunc(&this->ptr);
        if (errorCheck == 0)
        {
            errorReporter.ReportWinError(::GetLastError(), wow64FuncName);
        }
    }

    void Revert()
    {
        typedef BOOL(WINAPI * Wow64RevertWow64FsRedirectionFunc)(void*);
        auto revertFunc = library::kernel32()
            .get_function<Wow64RevertWow64FsRedirectionFunc>(
            GetIgnoreReporter(),
            "Wow64RevertWow64FsRedirection");
        if (revertFunc)
        {
            revertFunc(this->ptr);
            this->ptr = nullptr;
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
