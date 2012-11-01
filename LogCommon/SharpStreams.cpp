// Copyright © 2012 Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.
#include "pch.hpp"
#include <limits>
#include <stdexcept>
#include "Win32Exception.hpp"
#include "SharpStreams.hpp"

namespace Instalog { namespace SharpStreams {

#ifdef WIN32
using Instalog::SystemFacades::Win32Exception;

static std::vector<wchar_t> WindowsGetCharsWithCodepage( const UINT codepage, unsigned char * target, std::uint32_t length )
{
    if (length > static_cast<std::uint32_t>(std::numeric_limits<int>::max()))
    {
        throw std::runtime_error("Codepage conversion exceeded the maximum length of an integer.");
    }

    const DWORD flags = 0;
    int requiredSize = ::MultiByteToWideChar(codepage, flags, reinterpret_cast<LPCCH>(target), length, nullptr, 0);
    if (requiredSize == 0)
    {
        Win32Exception::ThrowFromLastError();
    }

    std::vector<wchar_t> result(requiredSize);
    int errorResult = ::MultiByteToWideChar(codepage, flags, reinterpret_cast<LPCCH>(target), length, result.data(), requiredSize);
    if (errorResult == 0)
    {
        Win32Exception::ThrowFromLastError();
    }

    return std::move(result);
}

static std::vector<unsigned char> WindowsGetBytesWithCodepage( const UINT codepage, wchar_t * target, std::uint32_t length )
{
    const DWORD flags = WC_COMPOSITECHECK | WC_NO_BEST_FIT_CHARS;
    if (length > static_cast<std::uint32_t>(std::numeric_limits<int>::max()))
    {
        throw std::runtime_error("Codepage conversion exceeded the maximum length of an integer.");
    }

    int requiredSize = ::WideCharToMultiByte(codepage, flags, target, length, nullptr, 0, nullptr, nullptr);
    if (requiredSize == 0)
    {
        Win32Exception::ThrowFromLastError();
    }

    std::vector<unsigned char> result(requiredSize);
    int errorResult = ::WideCharToMultiByte(codepage, flags, target, length, reinterpret_cast<LPSTR>(result.data()), requiredSize, nullptr, nullptr);
    if (errorResult == 0)
    {
        Win32Exception::ThrowFromLastError();
    }

    return std::move(result);
}

std::wstring AcpEncoder::Name()
{
    return L"CP_ACP";
}

std::vector<wchar_t> AcpEncoder::GetChars(unsigned char *target, std::uint32_t length)
{
    return WindowsGetCharsWithCodepage(CP_ACP, target, length);
}

std::vector<unsigned char> AcpEncoder::GetBytes(wchar_t *target, std::uint32_t length)
{
    return WindowsGetBytesWithCodepage(CP_ACP, target, length);
}

std::wstring OemEncoder::Name()
{
    return L"CP_OEM";
}

std::vector<wchar_t> OemEncoder::GetChars(unsigned char *target, std::uint32_t length)
{
    return WindowsGetCharsWithCodepage(CP_OEMCP, target, length);
}

std::vector<unsigned char> OemEncoder::GetBytes(wchar_t *target, std::uint32_t length)
{
    return WindowsGetBytesWithCodepage(CP_OEMCP, target, length);
}

#endif

static const std::uint32_t lowerOrderSixBits = 0x3F;
static const std::uint32_t lowerOrderTenBits = 0x3FF;
static const std::uint32_t replacementCharacter = 0xFFFD;

static bool EncodeUtf16(std::uint32_t codePoint, wchar_t& lowerCodePoint, wchar_t& upperCodePoint)
{
    // Encode a code point into UTF-16
    // See http://tools.ietf.org/html/rfc2781
    if (codePoint < 0x10000)
    {
        // No surrogate pair
        lowerCodePoint = static_cast<wchar_t>(codePoint);
        upperCodePoint = L'\0';
        return false;
    }
    else
    {
        codePoint -= 0x10000;
        lowerCodePoint = 0xD800;
        upperCodePoint = 0xDC00;
        lowerCodePoint |= (codePoint >> 10) & lowerOrderTenBits;
        upperCodePoint |= codePoint & lowerOrderTenBits;
        return true;
    }
}

static std::uint32_t DecodeUtf16(wchar_t *target, std::uint32_t length, std::uint32_t& idx)
{
    // Decode UTF-16 into a code point
    // See http://tools.ietf.org/html/rfc2781
    wchar_t lowerCodePoint = target[idx];
    if (lowerCodePoint < 0xD800  || lowerCodePoint > 0xDFFF)
    {
        // No surrogate pair
        return lowerCodePoint;
    }
    else
    {
        // Surrogate pair
        std::uint32_t codePoint;
        wchar_t upperCodePoint = L'0';
        bool valid = true; // Assume that it's a valid surrogate pair
        if (lowerCodePoint >= 0xD800 && lowerCodePoint <= 0xDBFF) // Character in reserved range
        {
            valid = false;
        }
        else if (idx + 1 == length) // No other half of the surrogate pair
        {
            valid = false;
        }
        else
        {
            upperCodePoint = target[++idx];
            if (upperCodePoint < 0xDC00 || upperCodePoint > 0xDFFF) // Invalid surrogate pair range
            {
                valid = false;
            }
        }

        // Check if we have valid data for the surrogate pair
        if (valid)
        {
            codePoint = ((lowerCodePoint & lowerOrderTenBits) << 10) | (upperCodePoint & lowerOrderTenBits);
            codePoint += 0x10000;
        }
        else
        {
            // Invalid UTF-16, use Unicode REPLACEMENT_CHARACTER
            // See: http://en.wikipedia.org/wiki/Replacement_character#Replacement_character
            codePoint = replacementCharacter;
        }

        return codePoint;
    }
}

static unsigned char EncodeUtf8(std::uint32_t codePoint, unsigned char buffer[4])
{
    // Encode a code point into UTF-8
    // http://tools.ietf.org/html/rfc3629
    if (codePoint < 0x80)
    {
        // Encode as one code unit
        buffer[0] = static_cast<unsigned char>(codePoint);
        return 1;
    }
    else if (codePoint < 0x800)
    {
        // Encode as two code units
        buffer[0] = static_cast<unsigned char>(0xC0 | codePoint >> 6);
        buffer[1] = static_cast<unsigned char>(0x80 | (codePoint & lowerOrderSixBits));
        return 2;
    }
    else if (codePoint >= 0xD800 && codePoint <= 0xDFFF)
    {
        // Unicode reserved UTF-16 range; invalid. Encode replacement character.
        buffer[0] = 0xEF;
        buffer[1] = 0xBF;
        buffer[2] = 0xBD;
        return 3;
    }
    else if (codePoint < 0x10000)
    {
        // Encode as three code units
        buffer[0] = static_cast<unsigned char>(0xE0 | codePoint >> 12);
        buffer[1] = static_cast<unsigned char>(0x80 | ((codePoint >> 6) & lowerOrderSixBits));
        buffer[2] = static_cast<unsigned char>(0x80 | (codePoint & lowerOrderSixBits));
        return 3;
    }
    else if (codePoint < 0x10FFFF)
    {
        // Encode as four code units
        buffer[0] = static_cast<unsigned char>(0xF0 | codePoint >> 18);
        buffer[1] = static_cast<unsigned char>(0x80 | ((codePoint >> 12) & lowerOrderSixBits));
        buffer[2] = static_cast<unsigned char>(0x80 | ((codePoint >> 6) & lowerOrderSixBits));
        buffer[3] = static_cast<unsigned char>(0x80 | (codePoint & lowerOrderSixBits));
        return 4;
    }
    else
    {
        // Invalid, encode replacement character as three code units
        buffer[0] = 0xEF;
        buffer[1] = 0xBF;
        buffer[2] = 0xBD;
        return 3;
    }
}

static bool ValidateUtf8SuffixCodeUnits(unsigned char *target, std::uint32_t length, std::uint32_t idx, std::uint32_t requestedLength)
{
    // Calculate the range of characters we are validating:
    std::uint32_t startIndex = length + idx;
    std::uint32_t endIndex = startIndex + requestedLength;
    if (endIndex >= length) // Not enough characters remaining
    {
        return false;
    }

    // We start at the second code unit because we assume the caller did the first one
    for (std::uint32_t currentIndex = startIndex + 1; currentIndex < endIndex; ++currentIndex)
    {
        // Make sure that the code unit begins with the 10b leading bits
        auto codeUnit = target[currentIndex];
        if ((codeUnit >> 6) != 0x02)
        {
            return false;
        }
    }

    return true;
}

static std::uint32_t DecodeUtf8(unsigned char *target, std::uint32_t length, std::uint32_t& idx)
{
    auto firstByte = target[idx];
    if (firstByte < 0x80)
    {
        // Single code unit -> code point
        return firstByte;
    }
    else if ((firstByte >> 5) == 0x6)
    {
        // Two code units -> code point
        if (!ValidateUtf8SuffixCodeUnits(target, length, idx, 2))
        {
            return replacementCharacter;
        }        std::uint32_t result = ((firstByte & 0x1F) << 5) | (target[idx + 1] & lowerOrderSixBits);
        // If two code units, range must be between 0x80-0x7FF inclusive
        if (result >= 0x80 && result <= 0x7FF)
        {
            ++idx; // We consumed one extra index
            return result;
        }
        else
        {
            // Invalid sequence. For instance, 11000000 10000000 should be an error, not U+0
            return replacementCharacter;
        }
    }
    else if ((firstByte >> 4) == 0xE)
    {
        // Three code units -> code point
        if (!ValidateUtf8SuffixCodeUnits(target, length, idx, 3))
        {
            return replacementCharacter;
        }
        std::uint32_t result = ((firstByte & 0xF) << 11) | ((target[idx + 1] & lowerOrderSixBits) << 6) | (target[idx + 2] & lowerOrderSixBits);
        // If two code units, range must be between 0x800-0xFFFF inclusive
        // but not the invalid UTF-16 range 0xD800-0xDFFF
        if (result >= 0x800 && result <= 0xFFFF && !(result >= 0xD800 && result <= 0xDFFF))
        {
            idx += 2; // We consumed two extra indicies
            return result;
        }
        else
        {
            // Invalid sequence. For instance, 11110000 10000000 10000000 should be an error, not U+0
            return replacementCharacter;
        }
    }
    else if ((firstByte >> 3) == 0x1E)
    {
        // Four code units -> code point
        if (!ValidateUtf8SuffixCodeUnits(target, length, idx, 4))
        {
            return replacementCharacter;
        }
        std::uint32_t result = ((firstByte & 0x7) << 18) | ((target[idx + 1] & lowerOrderSixBits) << 12) | ((target[idx + 2] & lowerOrderSixBits) << 6) |(target[idx + 3] & lowerOrderSixBits);
        // If two code units, range must be between 0x10000-0x10FFFF inclusive
        if (result >= 0x10000 && result <= 0x10FFFF)
        {
            idx += 3; // We consumed three extra indicies
            return result;
        }
        else
        {
            // Invalid sequence. For instance, 11110000 10000000 10000000 10000000 should be an error, not U+0
            return replacementCharacter;
        }
    }
    else
    {
        // Invalid code units -> replacement character
        return replacementCharacter;
    }
}

std::wstring Utf8Encoder::Name()
{
    return L"UTF-8";
}

std::vector<wchar_t> Utf8Encoder::GetChars(unsigned char *target, std::uint32_t length)
{
    std::vector<wchar_t> results;
    results.reserve(length); // Reasonable to have a starting assumption of 1:1 UTF16->UTF8
    for (std::uint32_t idx = 0; idx < length; ++idx)
    {
        auto codePoint = DecodeUtf8(target, length, idx);
        wchar_t lowerSurrogate;
        wchar_t upperSurrogate;
        if (EncodeUtf16(codePoint, lowerSurrogate, upperSurrogate))
        {
            results.push_back(lowerSurrogate);
            results.push_back(upperSurrogate);
        }
        else
        {
            results.push_back(lowerSurrogate);
        }
    }
    
    return std::move(results);
}

std::vector<unsigned char> Utf8Encoder::GetBytes(wchar_t *target, std::uint32_t length)
{
    std::vector<unsigned char> results;
    results.reserve(length); // Reasonable to have a starting assumption of 1:1 UTF16->UTF8
    for (std::uint32_t idx = 0; idx < length; ++idx)
    {
        auto codePoint = DecodeUtf16(target, length, idx);
        unsigned char points[4];
        auto codeUnitsUsed = EncodeUtf8(codePoint, points);
        for (unsigned int currentCodeUnit = 0; currentCodeUnit < codeUnitsUsed; ++currentCodeUnit)
        {
            results.push_back(points[currentCodeUnit]);
        }
    }

    return std::move(results);
}

std::wstring Utf16Encoder::Name()
{
    return L"UTF-16";
}

std::vector<wchar_t> Utf16Encoder::GetChars(unsigned char *target, std::uint32_t length)
{
    auto targetCast = reinterpret_cast<wchar_t *>(target);
    auto targetCastEnd = targetCast + length / sizeof(wchar_t);
    return std::vector<wchar_t>(targetCast, targetCastEnd);
}

std::vector<unsigned char> Utf16Encoder::GetBytes(wchar_t *target, std::uint32_t length)
{
    auto targetCast = reinterpret_cast<unsigned char *>(target);
    auto targetCastEnd = reinterpret_cast<unsigned char *>(target + length);
    return std::vector<unsigned char>(targetCast, targetCastEnd);
}

}} // Instalog::SharpStreams
