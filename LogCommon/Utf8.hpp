// Copyright © 2013 Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <string>
#include <cstring>
#include <iterator>
#include <utf8/utf8.h>

namespace utf8
{
    inline std::string ToUtf8(std::wstring const& input)
    {
        std::string result;
        result.reserve(input.size());
        utf8::utf16to8(input.cbegin(), input.cend(), std::back_inserter(result));
        return result;
    }

    inline std::string ToUtf8(wchar_t const* const input, wchar_t const* const end)
    {
        std::string result;
        result.reserve(end - input);
        utf8::utf16to8(input, end, std::back_inserter(result));
        return result;
    }

    inline std::string ToUtf8(wchar_t const* const input)
    {
        return ToUtf8(input, input + std::wcslen(input));
    }

    inline std::wstring ToUtf16(std::string const& input)
    {
        std::wstring result;
        result.reserve(input.size());
        utf8::utf8to16(input.cbegin(), input.cend(), std::back_inserter(result));
        return result;
    }

    inline std::wstring ToUtf16(char const* const input)
    {
        char const* const end = input + std::strlen(input);
        std::wstring result;
        result.reserve(end - input);
        utf8::utf8to16(input, end, std::back_inserter(result));
        return result;
    }
}
