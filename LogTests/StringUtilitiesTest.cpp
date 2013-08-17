// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <string>
#include <limits>
#include <algorithm>
#include "../LogCommon/StringUtilities.hpp"

using namespace Instalog;

TEST(StringUtilities, General_EscapeCharacter)
{
    std::wstring str = L"#";
    GeneralEscape(str);
    EXPECT_EQ(L"##", str);
}

TEST(StringUtilities, General_RightDelimiter)
{
    std::wstring str;

    str = L"[]";
    GeneralEscape(str);
    EXPECT_EQ(L"[]", str);

    str = L"[]";
    GeneralEscape(str, L'#', L']');
    EXPECT_EQ(L"[#]", str);
}

TEST(StringUtilities, General_NullCharacter)
{
    std::wstring str;
    str.push_back(0x00);
    GeneralEscape(str);
    EXPECT_EQ(L"#0", str);
}

TEST(StringUtilities, General_BackspaceCharacter)
{
    std::wstring str;
    str.push_back(0x08);
    GeneralEscape(str);
    EXPECT_EQ(L"#b", str);
}

TEST(StringUtilities, General_FormFeed)
{
    std::wstring str;
    str.push_back(0x0c);
    GeneralEscape(str);
    EXPECT_EQ(L"#f", str);
}

TEST(StringUtilities, General_Newline)
{
    std::wstring str;
    str.push_back(0x0A);
    GeneralEscape(str);
    EXPECT_EQ(L"#n", str);
}

TEST(StringUtilities, General_CarriageReturn)
{
    std::wstring str;
    str.push_back(0x0D);
    GeneralEscape(str);
    EXPECT_EQ(L"#r", str);
}

TEST(StringUtilities, General_HorizontalTab)
{
    std::wstring str;
    str.push_back(0x09);
    GeneralEscape(str);
    EXPECT_EQ(L"#t", str);
}

TEST(StringUtilities, General_VerticalTab)
{
    std::wstring str;
    str.push_back(0x0B);
    GeneralEscape(str);
    EXPECT_EQ(L"#v", str);
}

TEST(StringUtilities, General_OtherASCII)
{
    for (wchar_t c = 0x00; c < 0x1F; ++c)
    {
        if (c == 0x00 ||
            c == 0x08 ||
            c == 0x0C ||
            c == 0x0A ||
            c == 0x0D ||
            c == 0x09 ||
            c == 0x0B)
        {
            continue;
        }

        std::wstring str(1, c);
        GeneralEscape(str);
        wchar_t expected[5];
        swprintf_s(expected, 5, L"#x%02X", c);
        EXPECT_EQ(std::wstring(expected), str);
    }

    {
        std::wstring str(1, 0x7F);
        GeneralEscape(str);
        EXPECT_EQ(L"#x7F", str);
    }

    {
        std::wstring str(1, 0x7F);
        str.append(L"end");
        GeneralEscape(str);
        EXPECT_EQ(L"#x7Fend", str);
    }
}

TEST(StringUtilities, General_Unicode)
{
    srand((int)time(NULL));
    wchar_t increment = rand() % 10000;
    for (wchar_t c = 0x0080; c < std::numeric_limits<wchar_t>::max() - 10000; c += increment)
    {
        std::wstring str(1, c);
        GeneralEscape(str);
        wchar_t expected[7];
        swprintf_s(expected, 7, L"#u%04X", c);
        EXPECT_EQ(std::wstring(expected), str);
        increment = rand() % 10000;
    }

    // Check the last 
    std::wstring str(1, std::numeric_limits<wchar_t>::max());
    GeneralEscape(str);
    wchar_t expected[7];
    swprintf_s(expected, 7, L"#u%04X", std::numeric_limits<wchar_t>::max());
    EXPECT_EQ(std::wstring(expected), str);
}

TEST(StringUtilities, General_Whitespace)
{
    std::wstring str;

    str = L"start  end";
    GeneralEscape(str);
    EXPECT_EQ(L"start # end", str);

    str = L"start   end";
    GeneralEscape(str);
    EXPECT_EQ(L"start # # end", str);

    str = L" end";
    GeneralEscape(str);
    EXPECT_EQ(L"# end", str);

    str = L"  end";
    GeneralEscape(str);
    EXPECT_EQ(L"# # end", str);

    str = L"start  ";
    GeneralEscape(str);
    EXPECT_EQ(L"start # ", str);
}

TEST(StringUtilities, General_NonEscaped)
{
    for (wchar_t c = 0x1F + 1; c < 0x79; ++c)
    {
        if (c == L'#' || c == L' ')
        {
            continue;
        }
        std::wstring str(1, c);
        GeneralEscape(str);
        EXPECT_EQ(std::wstring(1, c), str);
    }
}

TEST(StringUtilities, Url_EscapeCharacter)
{
    std::wstring str = L"#";
    HttpEscape(str);
    EXPECT_EQ(L"##", str);
}

TEST(StringUtilities, Url_RightDelimiter)
{
    std::wstring str;
    
    str = L"[]";
    HttpEscape(str);
    EXPECT_EQ(L"[]", str);

    str = L"[]";
    HttpEscape(str, L'#', L']');
    EXPECT_EQ(L"[#]", str);
}

TEST(StringUtilities, Url_NullCharacter)
{
    std::wstring str;
    str.push_back(0x00);
    HttpEscape(str);
    EXPECT_EQ(L"#0", str);
}

TEST(StringUtilities, Url_BackspaceCharacter)
{
    std::wstring str;
    str.push_back(0x08);
    HttpEscape(str);
    EXPECT_EQ(L"#b", str);
}

TEST(StringUtilities, Url_FormFeed)
{
    std::wstring str;
    str.push_back(0x0c);
    HttpEscape(str);
    EXPECT_EQ(L"#f", str);
}

TEST(StringUtilities, Url_Newline)
{
    std::wstring str;
    str.push_back(0x0A);
    HttpEscape(str);
    EXPECT_EQ(L"#n", str);
}

TEST(StringUtilities, Url_CarriageReturn)
{
    std::wstring str;
    str.push_back(0x0D);
    HttpEscape(str);
    EXPECT_EQ(L"#r", str);
}

TEST(StringUtilities, Url_HorizontalTab)
{
    std::wstring str;
    str.push_back(0x09);
    HttpEscape(str);
    EXPECT_EQ(L"#t", str);
}

TEST(StringUtilities, Url_VerticalTab)
{
    std::wstring str;
    str.push_back(0x0B);
    HttpEscape(str);
    EXPECT_EQ(L"#v", str);
}

TEST(StringUtilities, Url_OtherASCII)
{
    for (wchar_t c = 0x00; c < 0x1F; ++c)
    {
        if (c == 0x00 ||
            c == 0x08 ||
            c == 0x0C ||
            c == 0x0A ||
            c == 0x0D ||
            c == 0x09 ||
            c == 0x0B)
        {
            continue;
        }

        std::wstring str(1, c);
        HttpEscape(str);
        wchar_t expected[5];
        swprintf_s(expected, 5, L"#x%02X", c);
        EXPECT_EQ(std::wstring(expected), str);
    }

    {
        std::wstring str(1, 0x7F);
        HttpEscape(str);
        EXPECT_EQ(L"#x7F", str);
    }
    
    {
        std::wstring str(1, 0x7F);
        str.append(L"end");
        HttpEscape(str);
        EXPECT_EQ(L"#x7Fend", str);
    }
}

TEST(StringUtilities, Url_Unicode)
{
    srand((int)time(NULL));
    wchar_t increment = rand() % 10000;
    for (wchar_t c = 0x0080; c < std::numeric_limits<wchar_t>::max() - 10000; c += increment)
    {
        std::wstring str(1, c);
        HttpEscape(str);
        wchar_t expected[7];
        swprintf_s(expected, 7, L"#u%04X", c);
        EXPECT_EQ(std::wstring(expected), str);
        increment = rand() % 10000;
    }

    // Check the last 
    std::wstring str(1, std::numeric_limits<wchar_t>::max());
    HttpEscape(str);
    wchar_t expected[7];
    swprintf_s(expected, 7, L"#u%04X", std::numeric_limits<wchar_t>::max());
    EXPECT_EQ(std::wstring(expected), str);
}

TEST(StringUtilities, Url_Whitespace)
{
    std::wstring str;

    str = L"start  end";
    HttpEscape(str);
    EXPECT_EQ(L"start # end", str);

    str = L"start   end";
    HttpEscape(str);
    EXPECT_EQ(L"start # # end", str);

    str = L" end";
    HttpEscape(str);
    EXPECT_EQ(L"# end", str);

    str = L"  end";
    HttpEscape(str);
    EXPECT_EQ(L"# # end", str);

    str = L"start  ";
    HttpEscape(str);
    EXPECT_EQ(L"start # ", str);
}

TEST(StringUtilities, Url_NonEscaped)
{
    for (wchar_t c = 0x1F + 1; c < 0x79; ++c)
    {
        if (c == L'#' || c == L' ')
        {
            continue;
        }
        std::wstring str(1, c);
        HttpEscape(str);
        EXPECT_EQ(std::wstring(1, c), str);
    }
}

TEST(StringUtilities, Url_UrlEscape)
{
    std::wstring str;

    str = L"http";
    HttpEscape(str);
    EXPECT_EQ(L"htt#p", str);

    str = L"HTTP";
    HttpEscape(str);
    EXPECT_EQ(L"htt#p", str);

    str = L"hTtP";
    HttpEscape(str);
    EXPECT_EQ(L"htt#p", str);
    
    str = L"hthttptp";
    HttpEscape(str);
    EXPECT_EQ(L"hthtt#ptp", str);

    str = L"htt#p";
    HttpEscape(str);
    EXPECT_EQ(L"htt##p", str);

    str = L"http://go.microsoft.com/";
    HttpEscape(str);
    EXPECT_EQ(L"htt#p://go.microsoft.com/", str);

    str = L"HTTp://go.microsoft.com/";
    HttpEscape(str);
    EXPECT_EQ(L"htt#p://go.microsoft.com/", str);

    str = L"hTtp://go.microsoft.com/";
    HttpEscape(str);
    EXPECT_EQ(L"htt#p://go.microsoft.com/", str);

    str = L"hthttptp://go.microsoft.com/";
    HttpEscape(str);
    EXPECT_EQ(L"hthtt#ptp://go.microsoft.com/", str);

    str = L"htt#p://go.microsoft.com/";
    HttpEscape(str);
    EXPECT_EQ(L"htt##p://go.microsoft.com/", str);
}

TEST(StringUtilities, UnescapeEmpty)
{
    std::wstring escaped = L"";
    std::wstring unescaped;
    std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(L"", unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeEndAtRightDelimiter)
{
    std::wstring escaped;
    std::wstring unescaped;
    std::wstring::iterator it;

    escaped = (L"string of interest]extra");
    it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped), L'#', L']');
    EXPECT_EQ(L"string of interest", unescaped);
    EXPECT_EQ(find(escaped.begin(), escaped.end(), L']'), it);

    escaped = (L"##a]extra");
    unescaped.clear();
    it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped), L'#', L']');
    EXPECT_EQ(L"#a", unescaped);
    EXPECT_EQ(find(escaped.begin(), escaped.end(), L']'), it);
}

TEST(StringUtilities, UnescapeMalformed)
{
    std::wstring escaped = L"#";
    std::wstring unescaped;
    EXPECT_THROW(Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped)), MalformedEscapedSequence);
}

TEST(StringUtilities, UnescapeEscapeCharacter)
{
    std::wstring escaped = L"##";
    std::wstring unescaped;
    std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(L"#", unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeNullCharacter)
{
    std::wstring escaped = L"#0";
    std::wstring unescaped;
    std::wstring expected;
    expected.push_back(L'\0');
    std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeBackspaceCharacter)
{
    std::wstring escaped = L"#b";
    std::wstring unescaped;
    std::wstring expected;
    expected.push_back(0x08);
    std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeFormFeedCharacter)
{
    std::wstring escaped = L"#f";
    std::wstring unescaped;
    std::wstring expected;
    expected.push_back(0x0C);
    std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeNewlineCharacter)
{
    std::wstring escaped = L"#n";
    std::wstring unescaped;
    std::wstring expected;
    expected.push_back(0x0A);
    std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeCarriageReturnCharacter)
{
    std::wstring escaped = L"#r";
    std::wstring unescaped;
    std::wstring expected;
    expected.push_back(0x0D);
    std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeHorizontalTabCharacter)
{
    std::wstring escaped = L"#t";
    std::wstring unescaped;
    std::wstring expected;
    expected.push_back(0x09);
    std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeVerticalTabCharacter)
{
    std::wstring escaped = L"#v";
    std::wstring unescaped;
    std::wstring expected;
    expected.push_back(0x0B);
    std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeOtherASCII)
{
    for (wchar_t c = 0x00; c < 0x1F; ++c)
    {
        wchar_t escapedChar[5];
        swprintf_s(escapedChar, 5, L"#x%02X", c);
        std::wstring escaped = escapedChar;
        std::wstring unescaped;
        std::wstring expected;
        expected.push_back(c);
        std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        EXPECT_EQ(expected, unescaped);
        EXPECT_EQ(escaped.end(), it);
    }

    {
        std::wstring escaped = L"#x7F";
        std::wstring unescaped;
        std::wstring expected;
        expected.push_back(0x7F);
        std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        EXPECT_EQ(expected, unescaped);
        EXPECT_EQ(escaped.end(), it);
    }
}

TEST(StringUtilities, UnescapeUnicode)
{
    srand((int)time(NULL));
    wchar_t increment = rand() % 10000;
    for (wchar_t c = 0x0080; c < std::numeric_limits<wchar_t>::max() - 10000; c += increment)
    {
        wchar_t escapedChar[7];
        swprintf_s(escapedChar, 7, L"#u%04X", c);
        std::wstring escaped = escapedChar;
        std::wstring unescaped;
        std::wstring expected;
        expected.push_back(c);
        std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        EXPECT_EQ(expected, unescaped);
        EXPECT_EQ(escaped.end(), it);
    }

    {
        std::wstring escaped = L"#uFFFF";
        std::wstring unescaped;
        std::wstring expected;
        expected.push_back(0xFFFF);
        std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        EXPECT_EQ(expected, unescaped);
        EXPECT_EQ(escaped.end(), it);
    }
}

TEST(StringUtilities, UnescapeWhitespace)
{
    std::wstring escaped, unescaped;
    std::wstring::iterator it;

    escaped = L"start # end";
    it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(L"start  end", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();

    escaped = L"start # # end";
    it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(L"start   end", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();

    escaped = L"# end";
    it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(L" end", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();

    escaped = L"# # end";
    it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(L"  end", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();

    escaped = L"start # ";
    it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(L"start  ", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();
}

TEST(StringUtilities, UnescapeNonEscaped)
{
    for (wchar_t c = 0x1F + 1; c < 0x79; ++c)
    {
        if (c == L'#' || c == L' ')
        {
            continue;
        }
        std::wstring escaped(1, c);
        std::wstring unescaped;
        std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        EXPECT_EQ(escaped, unescaped);
        EXPECT_EQ(escaped.end(), it);
    }

    for (wchar_t c = 0x1F + 1; c < 0x79; ++c)
    {
        if (c == L'#' || c == L' ')
        {
            continue;
        }
        std::wstring escaped(1, c);
        std::wstring unescaped;
        std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        EXPECT_EQ(escaped, unescaped);
        EXPECT_EQ(escaped.end(), it);
    }
}

TEST(StringUtilities, UnescapeUrl)
{
    std::wstring escaped, unescaped;
    std::wstring::iterator it;

    escaped = L"htt#p";
    it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(L"http", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();

    escaped = L"htt##p";
    it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(L"htt#p", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();

    escaped = L"hthtt#ptp";
    it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(L"hthttptp", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();
}

static void TestCmdLineArgvUnescape(std::wstring const& escaped, std::wstring const& expected, std::wstring::const_iterator endingPosition)
{
    std::wstring unescaped;
    unescaped.reserve(escaped.size());
    std::wstring::const_iterator actualEnd = CmdLineToArgvWUnescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(endingPosition, actualEnd);
}

static void TestCmdLineArgvUnescape(std::wstring const& escaped, std::wstring const& expected)
{
    return TestCmdLineArgvUnescape(escaped, expected, escaped.end());
}

TEST(StringUtilities, CmdLineToArgvWUnescapeNoEscapes)
{
    TestCmdLineArgvUnescape(L"\"nothing needs escaping here!\"", L"nothing needs escaping here!");
}

TEST(StringUtilities, CmdLineToArgvWUnescapeOneBackslash)
{
    TestCmdLineArgvUnescape(L"\"start\\end\"", L"start\\end");
}

TEST(StringUtilities, CmdLineToArgvWUnescapeTwoBackslashes)
{
    TestCmdLineArgvUnescape(L"\"start\\\\end\"", L"start\\\\end");
}

TEST(StringUtilities, CmdLineToArgvWUnescapedOneBackslashQuote)
{
    TestCmdLineArgvUnescape(L"\"start\\\"end\"", L"start\"end");
}

TEST(StringUtilities, CmdLineToArgvWUnescapedTwoBackslashQuote)
{
    TestCmdLineArgvUnescape(L"\"start\\\\\"end\"", L"start\\\"end");
}

TEST(StringUtilities, CmdLineToArgvWUnescapedThreeBackslashQuote)
{
    TestCmdLineArgvUnescape(L"\"start\\\\\\\"end\"", L"start\\\"end");
}

TEST(StringUtilities, CmdLineToArgvWUnescapedAfterQuote)
{
    std::wstring escaped = L"\"start end\"after";
    TestCmdLineArgvUnescape(escaped, L"start end", escaped.begin() + 11);
}

TEST(StringUtiliites, CmdLineToArgvWUnescapedAfterQuoteWithEscapes)
{
    std::wstring escaped = L"\"start\\\"end\"after";
    TestCmdLineArgvUnescape(escaped, L"start\"end", escaped.begin() + 12);
}

TEST(StringUtilities, NoQuotesCmdArgVThrows)
{
    std::wstring escaped(L"example");
    ASSERT_THROW(CmdLineToArgvWUnescape(escaped.begin(), escaped.end(), escaped.begin()), MalformedEscapedSequence);
}

TEST(StringUtilities, EmptyCmdArgVThrows)
{
    std::wstring escaped;
    ASSERT_THROW(CmdLineToArgvWUnescape(escaped.begin(), escaped.end(), escaped.begin()), MalformedEscapedSequence);
}

TEST(StringUtilities, SimpleHeaderCorrect)
{
    std::wstring tested(L"Example");
    Header(tested);
    ASSERT_EQ(L"===================== Example ====================", tested);
}

TEST(StringUtilities, EvenHeaderCorrect)
{
    std::wstring tested(L"Foobar");
    Header(tested);
    ASSERT_EQ(L"===================== Foobar =====================", tested);
}

TEST(StringUtilities, TooLong)
{
    std::wstring tested(L"FoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobar");
    Header(tested);
    ASSERT_EQ(L"FoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobar", tested);
}

