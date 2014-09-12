//          Copyright Billy O'Neal 2013
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <string>
#include <algorithm>
#include <Windows.h>
#include "../LogCommon/Utf8.hpp"

inline std::string GetTestBinaryPath()
{
    std::string (*ptr)() = GetTestBinaryPath;
    wchar_t const* ptrVoid = reinterpret_cast<wchar_t const*>(ptr);
    HMODULE hMod;
    ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         ptrVoid,
                         &hMod);
    std::wstring result;
    result.resize(MAX_PATH);
    result.resize(::GetModuleFileNameW(hMod, &result[0], MAX_PATH));
    return utf8::ToUtf8(result);
}

inline std::string GetTestBinaryDir()
{
    std::string result(GetTestBinaryPath());
    result.replace(std::find(result.crbegin(), result.crend(), '\\').base(),
                   result.cend(),
                   "..\\TestData\\");
    return result;
}

inline std::string GetTestFilePath(std::string const& file)
{
    return GetTestBinaryDir().append(file);
}
