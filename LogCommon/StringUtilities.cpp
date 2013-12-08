// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "StringUtilities.hpp"
#include <utf8/utf8.h>

namespace Instalog
{
static char http[] = "http";
static char HTTP[] = "HTTP";

template <typename Iterator>
static void WriteEscapeHex(Iterator& out, char escapeCharacter, char value)
{
    char const hexChars[] = "0123456789ABCDEF";
    *(out++) = escapeCharacter;
    *(out++) = hexChars[value >> 4];
    *(out++) = hexChars[value & 0x0F];
}

template <typename Iterator>
static void WriteEscapeChar(Iterator& out, char escapeCharacter, char value)
{
    *(out++) = escapeCharacter;
    *(out++) = value;
}

template <typename Iterator>
static void WriteChar(Iterator& out, char value)
{
    *(out++) = value;
}

static void GeneralEscapeImpl(std::string& target, char escapeCharacter, char rightDelimiter, bool escapeHttp)
{
    // Count the number of characters in the resulting string
    std::size_t resultSize = target.size();
    bool lastWasSpace = false;
    unsigned short httpState = 0;
    for (char c : target)
    {
        // These characters get escaped as 1 extra character.
        if (c == escapeCharacter || c == rightDelimiter)
        {
            resultSize++;
            continue;
        }

        if (lastWasSpace && c == ' ')
        {
            resultSize++;
            continue;
        }

        lastWasSpace = c == ' ';

        switch (c)
        {
        case '\x00': // #0
        case '\x08': // #b
        case '\x0C': // #f
        case '\x0A': // #n
        case '\x0D': // #r
        case '\x09': // #t
        case '\x0B': // #v
            ++resultSize;
            continue;
        }

        if (c <= 0x1F || c >= 0x80)
        {
            // #xXX
            resultSize += 3;
            continue;
        }

        if (escapeHttp)
        {
            if (c == http[0] || c == HTTP[0])
            {
                httpState = 1;
            }
            else if (c == http[httpState] || c == HTTP[httpState])
            {
                ++httpState;
            }
            else
            {
                httpState = 0;
            }

            if (httpState == 4)
            {
                resultSize++;
                continue;
            }
        }
    }

    // We expect most of the time that escapes are not necessary. If the resulting size is
    // unchanged at this point, none were and we can avoid an extra allocation.
    if (resultSize == target.size())
    {
        return;
    }

    // This switch allows the caller to avoid reallocating in some cases.
    std::string source(target);
    target.resize(resultSize);
    auto out = target.begin();
    lastWasSpace = false;
    for (char c : source)
    {
        if (c == escapeCharacter || c == rightDelimiter)
        {
            WriteEscapeChar(out, escapeCharacter, c);
            continue;
        }

        if (lastWasSpace && c == ' ')
        {
            WriteEscapeChar(out, escapeCharacter, ' ');
            continue;
        }

        lastWasSpace = c == ' ';

        switch (c)
        {
        case '\x00': // #0
            WriteEscapeChar(out, escapeCharacter, '0');
            continue;
        case '\x08': // #b
            WriteEscapeChar(out, escapeCharacter, 'b');
            continue;
        case '\x0C': // #f
            WriteEscapeChar(out, escapeCharacter, 'f');
            continue;
        case '\x0A': // #n
            WriteEscapeChar(out, escapeCharacter, 'n');
            continue;
        case '\x0D': // #r
            WriteEscapeChar(out, escapeCharacter, 'r');
            continue;
        case '\x09': // #t
            WriteEscapeChar(out, escapeCharacter, 't');
            continue;
        case '\x0B': // #v
            WriteEscapeChar(out, escapeCharacter, 'v');
            continue;
        }

        lastWasSpace = false;

        if (c <= 0x1F || c >= 0x80)
        {
            // #xXX
            WriteEscapeHex(out, escapeCharacter, c);
            continue;
        }

        if (escapeHttp)
        {
            if (c == http[0] || c == HTTP[0])
            {
                httpState = 1;
            }
            else if (c == http[httpState] || c == HTTP[httpState])
            {
                ++httpState;
            }
            else
            {
                httpState = 0;
            }

            if (httpState == 4)
            {
                WriteEscapeChar(out, escapeCharacter, c);
                continue;
            }
        }

        *(out++) = c;
    }
}

void GeneralEscape(std::string& target,
                   char escapeCharacter,
                   char rightDelimiter)
{
    return GeneralEscapeImpl(target, escapeCharacter, rightDelimiter, false);
}

void HttpEscape(std::string& target,
                char escapeCharacter /*= L'#'*/,
                char rightDelimiter /*= L'\0'*/)
{
    return GeneralEscapeImpl(target, escapeCharacter, rightDelimiter, true);
}

char const* InvalidHexCharacter::what() const
{
    return "Invalid hex character in supplied string";
}

char const* MalformedEscapedSequence::what() const
{
    return "Malformed escaped sequence in supplied string";
}

void Header(std::string& headerText, std::size_t headerWidth /*= 50*/)
{
    if (headerText.size() + 2 > headerWidth)
        return;
    headerText.reserve(headerWidth);
    std::size_t nonTextWidth = headerWidth - headerText.size();
    std::size_t totalEquals = nonTextWidth - 2;
    std::size_t rightEquals = totalEquals / 2;
    std::size_t leftEquals = totalEquals - rightEquals;
    headerText.insert(headerText.begin(), leftEquals + 1, '=');
    headerText[leftEquals] = ' ';
    headerText.append(1, ' ');
    headerText.append(rightEquals, '=');
}
}
