// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <string>
#include <exception>
#include <windows.h>

namespace Instalog
{
/// @brief    Escapes strings according to the general escape scheme
///
/// @param [in,out]    target     String to escape
/// @param    escapeCharacter    (optional) the escape character.
/// @param    rightDelimiter     (optional) the right delimiter.
void GeneralEscape(std::string& target,
                   unsigned char escapeCharacter = '#',
                   unsigned char rightDelimiter = '\0');

/// @brief    Escapes strings according to the url escape scheme
///
/// @param [in,out]    target     String to escape
/// @param    escapeCharacter    (optional) the escape character.
/// @param    rightDelimiter     (optional) the right delimiter.
void HttpEscape(std::string& target,
                unsigned char escapeCharacter = '#',
                unsigned char rightDelimiter = '\0');

/// @brief    Malformed escaped sequence
class MalformedEscapedSequence : public std::exception
{
    virtual char const* what() const;
};

/// @brief    Invalid hexadecimal character.
class InvalidHexCharacter : public std::exception
{
    virtual char const* what() const;
};

/// @brief    Appends the given character in a hexadecimal representation to
///         the target string.
///
/// @param    characterToHex    The character to print in hexadecimal form.
/// @param [in,out]    target    Target where the character should be written.
inline void HexCharacter(unsigned char characterToHex, std::string& target)
{
    static const char chars[] = "0123456789ABCDEF";
    target.push_back(chars[characterToHex >> 4]);
    target.push_back(chars[characterToHex & 0x0F]);
}

/// @brief    Extracts a numeric character from a hexadecimal representation.
///
/// @exception    InvalidHexCharacter    Thrown when an invalid hexadecimal
/// character
///                                 appears in the source text.
///
/// @param    hexCharacter    The hexadecimal character to interpret as an
/// integer.
///
/// @return    The character un-hexed.
inline char UnHexCharacter(char hexCharacter)
{
    if (hexCharacter >= '0' && hexCharacter <= '9')
    {
        return hexCharacter - '0';
    }
    if (hexCharacter >= 'A' && hexCharacter <= 'F')
    {
        return hexCharacter - 'A' + 10;
    }
    if (hexCharacter >= 'a' && hexCharacter <= 'a')
    {
        return hexCharacter - 'a' + 10;
    }
    throw InvalidHexCharacter();
}

/// @brief    Unescapes a given string
///
/// @exception    MalformedEscapedSequence    Thrown when a malformed escaped
/// sequence is passed in
///
/// @param    begin               An iterator at the beginning of the string to
/// escape
/// @param    end                   An iterator one past the end of the string
/// to escape
/// @param    target               Target iterator to write to.  Should not be
/// overlapping with the escaped string or the behavior is undefined
/// @param    escapeCharacter    (optional) the escape character.
/// @param    endDelimiter       (optional) the end delimiter.
///
/// @return    An iterator to where unescaping stopped (end of the string or
/// endDelimiter)
template <typename InIter, typename OutIter>
inline InIter Unescape(InIter begin,
                       InIter end,
                       OutIter target,
                       char escapeCharacter = '#',
                       char endDelimiter = '\0')
{
    char temp;
    for (; begin != end && *begin != endDelimiter; ++begin)
    {
        if (*begin == escapeCharacter)
        {
            begin++;
            if (begin == end)
            {
                throw MalformedEscapedSequence();
            }

            switch (*begin)
            {
            case '0':
                *target = 0x00;
                break;
            case 'b':
                *target = 0x08;
                break;
            case 'f':
                *target = 0x0C;
                break;
            case 'n':
                *target = 0x0A;
                break;
            case 'r':
                *target = 0x0D;
                break;
            case 't':
                *target = 0x09;
                break;
            case 'v':
                *target = 0x0B;
                break;
            case 'x':
                if (std::distance(begin, end) < 3)
                    throw MalformedEscapedSequence();
                temp = 0;
                for (unsigned int idx = 0; idx < 2; ++idx)
                {
                    temp <<= 4;
                    temp |= UnHexCharacter(*++begin);
                }
                *target = temp;
                break;
            default:
                *target = *begin;
                break;
            }
        }
        else
        {
            *target = *begin;
        }
        ++target;
    }
    return begin;
}

/// @brief    Unescapes a command line argv escaped string
///
/// @exception    MalformedEscapedSequence    Thrown when a malformed escaped
/// sequence is supplied
///
/// @param    begin               An iterator at the beginning of the string to
/// escape
/// @param    end                   An iterator one past the end of the string
/// to escape
/// @param    target               Target iterator to write to.  Should not be
/// overlapping with the escaped string or the behavior is undefined
///
/// @return    An iterator to where unescaping stopped (end of the string or
/// endDelimiter)
template <typename InIter, typename OutIter>
inline InIter CmdLineToArgvWUnescape(InIter begin, InIter end, OutIter target)
{
    if (std::distance(begin, end) < 2 || *begin != '"')
    {
        // ""s are required
        throw MalformedEscapedSequence();
    }
    ++begin; // Skip "
    std::size_t backslashCount = 0;
    for (; begin != end; ++begin)
    {
        switch (*begin)
        {
        case '\\':
            backslashCount++;
            break;
        case '"':
            if (backslashCount)
            {
                std::fill_n(target, backslashCount / 2, '\\');
                *target++ = '"';
                backslashCount = 0;
            }
            else
            {
                return ++begin;
            }
            break;
        default:
            if (backslashCount)
            {
                std::fill_n(target, backslashCount, '\\');
                backslashCount = 0;
            }
            *target++ = *begin;
        }
    }
    throw MalformedEscapedSequence();
}

/// @brief    Prints a header.
///
/// @param [in,out]    headerText    The header text.
/// @param    headerWidth              (optional) The width of the header.
void Header(std::string& headerText, std::size_t headerWidth = 50);
}
