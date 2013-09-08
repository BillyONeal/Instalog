//          Copyright Billy O'Neal 2013
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <string>
#include <algorithm>
#include <Windows.h>

inline std::wstring GetTestBinaryPath()
{
    std::wstring (*ptr)() = GetTestBinaryPath;
    wchar_t const* ptrVoid = reinterpret_cast<wchar_t const*>(ptr);
    HMODULE hMod;
    ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         ptrVoid,
                         &hMod);
    std::wstring result;
    result.resize(MAX_PATH);
    result.resize(::GetModuleFileNameW(hMod, &result[0], MAX_PATH));
    return result;
}

inline std::wstring GetTestBinaryDir()
{
    std::wstring result(GetTestBinaryPath());
    result.replace(std::find(result.crbegin(), result.crend(), L'\\').base(),
                   result.cend(),
                   L"TestData\\");
    return result;
}

inline std::wstring GetTestFilePath(std::wstring const& file)
{
    return GetTestBinaryDir().append(file);
}

inline std::string Str(char const* val)
{
    return val;
}

inline std::wstring Str(wchar_t const* val)
{
    return val;
}
