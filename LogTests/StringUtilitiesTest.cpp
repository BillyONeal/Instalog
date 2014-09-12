// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "../LogCommon/StringUtilities.hpp"
#include "gtest/gtest.h"
#include <string>
#include <limits>
#include <algorithm>
#include "../LogCommon/LogSink.hpp"

using namespace Instalog;

TEST(StringUtilities, General_EscapeCharacter)
{
    std::string str = "#";
    GeneralEscape(str);
    EXPECT_EQ("##", str);
}

TEST(StringUtilities, General_RightDelimiter)
{
    std::string str;

    str = "[]";
    GeneralEscape(str);
    EXPECT_EQ("[]", str);

    str = "[]";
    GeneralEscape(str, '#', ']');
    EXPECT_EQ("[#]", str);
}

TEST(StringUtilities, General_NullCharacter)
{
    std::string str;
    str.push_back(0x00);
    GeneralEscape(str);
    EXPECT_EQ("#0", str);
}

TEST(StringUtilities, General_BackspaceCharacter)
{
    std::string str;
    str.push_back(0x08);
    GeneralEscape(str);
    EXPECT_EQ("#b", str);
}

TEST(StringUtilities, General_FormFeed)
{
    std::string str;
    str.push_back(0x0c);
    GeneralEscape(str);
    EXPECT_EQ("#f", str);
}

TEST(StringUtilities, General_Newline)
{
    std::string str;
    str.push_back(0x0A);
    GeneralEscape(str);
    EXPECT_EQ("#n", str);
}

TEST(StringUtilities, General_CarriageReturn)
{
    std::string str;
    str.push_back(0x0D);
    GeneralEscape(str);
    EXPECT_EQ("#r", str);
}

TEST(StringUtilities, General_HorizontalTab)
{
    std::string str;
    str.push_back(0x09);
    GeneralEscape(str);
    EXPECT_EQ("#t", str);
}

TEST(StringUtilities, General_VerticalTab)
{
    std::string str;
    str.push_back(0x0B);
    GeneralEscape(str);
    EXPECT_EQ("#v", str);
}

static void TestGeneralOtherAscii(unsigned char c)
{
    std::string expected;
    std::string str;

    // No pre/post
    write(expected, "#x", hex(c));
    str.push_back(c);
    GeneralEscape(str);
    ASSERT_EQ(expected, str);

    // Pre
    expected.clear();
    write(expected, "before #x", hex(c));
    str.assign("before ");
    str.push_back(c);
    GeneralEscape(str);
    ASSERT_EQ(expected, str);

    // Post
    expected.clear();
    write(expected, "#x", hex(c), " after");
    str.resize(1);
    str[0] = c;
    str.append(" after");
    GeneralEscape(str);
    ASSERT_EQ(expected, str);

    // Both
    expected.clear();
    write(expected, "before #x", hex(c), " after");
    str.assign("before ");
    str.push_back(c);
    str.append(" after");
    GeneralEscape(str);
    ASSERT_EQ(expected, str);
}

TEST(StringUtilities, General_OtherASCII)
{
    unsigned char ranges[][2] = {
        // #0
        {0x01u, 0x08u}, // #b, #t, #n, #v, #f, #r
        {0x0Eu, 0x20u}, // (printables),
        {0x7Fu, 0x00u}, // wrap around to 0
    };

    for (auto const& range : ranges)
    {
        for (unsigned char c = range[0]; c != range[1]; ++c)
        {
            TestGeneralOtherAscii(c);
        }
    }
}

TEST(StringUtilities, General_Whitespace)
{
    std::string str;

    str = "start  end";
    GeneralEscape(str);
    EXPECT_EQ("start # end", str);

    str = "start   end";
    GeneralEscape(str);
    EXPECT_EQ("start # # end", str);

    str = " end";
    GeneralEscape(str);
    EXPECT_EQ("# end", str);

    str = "  end";
    GeneralEscape(str);
    EXPECT_EQ("# # end", str);

    str = "start  ";
    GeneralEscape(str);
    EXPECT_EQ("start # ", str);
}

TEST(StringUtilities, General_NonEscaped)
{
    for (char c = 0x1F + 1; c < 0x79; ++c)
    {
        if (c == '#' || c == ' ')
        {
            continue;
        }
        std::string str(1, c);
        GeneralEscape(str);
        EXPECT_EQ(std::string(1, c), str);
    }
}

TEST(StringUtilities, Url_EscapeCharacter)
{
    std::string str = "#";
    HttpEscape(str);
    EXPECT_EQ("##", str);
}

TEST(StringUtilities, Url_RightDelimiter)
{
    std::string str;

    str = "[]";
    HttpEscape(str);
    EXPECT_EQ("[]", str);

    str = "[]";
    HttpEscape(str, '#', ']');
    EXPECT_EQ("[#]", str);
}

TEST(StringUtilities, Url_NullCharacter)
{
    std::string str;
    str.push_back(0x00);
    HttpEscape(str);
    EXPECT_EQ("#0", str);
}

TEST(StringUtilities, Url_BackspaceCharacter)
{
    std::string str;
    str.push_back(0x08);
    HttpEscape(str);
    EXPECT_EQ("#b", str);
}

TEST(StringUtilities, Url_FormFeed)
{
    std::string str;
    str.push_back(0x0c);
    HttpEscape(str);
    EXPECT_EQ("#f", str);
}

TEST(StringUtilities, Url_Newline)
{
    std::string str;
    str.push_back(0x0A);
    HttpEscape(str);
    EXPECT_EQ("#n", str);
}

TEST(StringUtilities, Url_CarriageReturn)
{
    std::string str;
    str.push_back(0x0D);
    HttpEscape(str);
    EXPECT_EQ("#r", str);
}

TEST(StringUtilities, Url_HorizontalTab)
{
    std::string str;
    str.push_back(0x09);
    HttpEscape(str);
    EXPECT_EQ("#t", str);
}

TEST(StringUtilities, Url_VerticalTab)
{
    std::string str;
    str.push_back(0x0B);
    HttpEscape(str);
    EXPECT_EQ("#v", str);
}

TEST(StringUtilities, Url_OtherASCII)
{
    for (char c = 0x00; c < 0x1F; ++c)
    {
        if (c == 0x00 || c == 0x08 || c == 0x0C || c == 0x0A || c == 0x0D ||
            c == 0x09 || c == 0x0B)
        {
            continue;
        }

        std::string str(1, c);
        HttpEscape(str);
        char expected[5];
        sprintf_s(expected, 5, "#x%02X", c);
        EXPECT_EQ(std::string(expected), str);
    }

    {
        std::string str(1, 0x7F);
        HttpEscape(str);
        EXPECT_EQ("#x7F", str);
    }

    {
        std::string str(1, 0x7F);
        str.append("end");
        HttpEscape(str);
        EXPECT_EQ("#x7Fend", str);
    }
}

TEST(StringUtilities, Url_Whitespace)
{
    std::string str;

    str = "start  end";
    HttpEscape(str);
    EXPECT_EQ("start # end", str);

    str = "start   end";
    HttpEscape(str);
    EXPECT_EQ("start # # end", str);

    str = " end";
    HttpEscape(str);
    EXPECT_EQ("# end", str);

    str = "  end";
    HttpEscape(str);
    EXPECT_EQ("# # end", str);

    str = "start  ";
    HttpEscape(str);
    EXPECT_EQ("start # ", str);
}

TEST(StringUtilities, Url_NonEscaped)
{
    for (char c = 0x1F + 1; c < 0x79; ++c)
    {
        if (c == '#' || c == ' ')
        {
            continue;
        }
        std::string str(1, c);
        HttpEscape(str);
        EXPECT_EQ(std::string(1, c), str);
    }
}

TEST(StringUtilities, Url_UrlEscape)
{
    std::string str;

    str = "http";
    HttpEscape(str);
    EXPECT_EQ("htt#p", str);

    str = "HTTP";
    HttpEscape(str);
    EXPECT_EQ("HTT#P", str);

    str = "hTtP";
    HttpEscape(str);
    EXPECT_EQ("hTt#P", str);

    str = "hthttptp";
    HttpEscape(str);
    EXPECT_EQ("hthtt#ptp", str);

    str = "htt#p";
    HttpEscape(str);
    EXPECT_EQ("htt##p", str);

    str = "http://go.microsoft.com/";
    HttpEscape(str);
    EXPECT_EQ("htt#p://go.microsoft.com/", str);

    str = "HTTp://go.microsoft.com/";
    HttpEscape(str);
    EXPECT_EQ("HTT#p://go.microsoft.com/", str);

    str = "hTtp://go.microsoft.com/";
    HttpEscape(str);
    EXPECT_EQ("hTt#p://go.microsoft.com/", str);

    str = "hthttptp://go.microsoft.com/";
    HttpEscape(str);
    EXPECT_EQ("hthtt#ptp://go.microsoft.com/", str);

    str = "htt#p://go.microsoft.com/";
    HttpEscape(str);
    EXPECT_EQ("htt##p://go.microsoft.com/", str);
}

TEST(StringUtilities, UnescapeEmpty)
{
    std::string escaped = "";
    std::string unescaped;
    std::string::iterator it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ("", unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeEndAtRightDelimiter)
{
    std::string escaped;
    std::string unescaped;
    std::string::iterator it;

    escaped = ("string of interest]extra");
    it = Unescape(escaped.begin(),
                  escaped.end(),
                  std::back_inserter(unescaped),
                  '#',
                  ']');
    EXPECT_EQ("string of interest", unescaped);
    EXPECT_EQ(find(escaped.begin(), escaped.end(), ']'), it);

    escaped = ("##a]extra");
    unescaped.clear();
    it = Unescape(escaped.begin(),
                  escaped.end(),
                  std::back_inserter(unescaped),
                  '#',
                  ']');
    EXPECT_EQ("#a", unescaped);
    EXPECT_EQ(find(escaped.begin(), escaped.end(), ']'), it);
}

TEST(StringUtilities, UnescapeMalformed)
{
    std::string escaped = "#";
    std::string unescaped;
    EXPECT_THROW(
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped)),
        MalformedEscapedSequence);
}

TEST(StringUtilities, UnescapeEscapeCharacter)
{
    std::string escaped = "##";
    std::string unescaped;
    std::string::iterator it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ("#", unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeNullCharacter)
{
    std::string escaped = "#0";
    std::string unescaped;
    std::string expected;
    expected.push_back('\0');
    std::string::iterator it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeBackspaceCharacter)
{
    std::string escaped = "#b";
    std::string unescaped;
    std::string expected;
    expected.push_back(0x08);
    std::string::iterator it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeFormFeedCharacter)
{
    std::string escaped = "#f";
    std::string unescaped;
    std::string expected;
    expected.push_back(0x0C);
    std::string::iterator it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeNewlineCharacter)
{
    std::string escaped = "#n";
    std::string unescaped;
    std::string expected;
    expected.push_back(0x0A);
    std::string::iterator it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeCarriageReturnCharacter)
{
    std::string escaped = "#r";
    std::string unescaped;
    std::string expected;
    expected.push_back(0x0D);
    std::string::iterator it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeHorizontalTabCharacter)
{
    std::string escaped = "#t";
    std::string unescaped;
    std::string expected;
    expected.push_back(0x09);
    std::string::iterator it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeVerticalTabCharacter)
{
    std::string escaped = "#v";
    std::string unescaped;
    std::string expected;
    expected.push_back(0x0B);
    std::string::iterator it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(escaped.end(), it);
}

TEST(StringUtilities, UnescapeOtherASCII)
{
    for (char c = 0x00; c < 0x1F; ++c)
    {
        char escapedChar[5];
        sprintf_s(escapedChar, 5, "#x%02X", c);
        std::string escaped = escapedChar;
        std::string unescaped;
        std::string expected;
        expected.push_back(c);
        std::string::iterator it = Unescape(
            escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        EXPECT_EQ(expected, unescaped);
        EXPECT_EQ(escaped.end(), it);
    }

    {
        std::string escaped = "#x7F";
        std::string unescaped;
        std::string expected;
        expected.push_back(0x7F);
        std::string::iterator it = Unescape(
            escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        EXPECT_EQ(expected, unescaped);
        EXPECT_EQ(escaped.end(), it);
    }
}

TEST(StringUtilities, UnescapeWhitespace)
{
    std::string escaped, unescaped;
    std::string::iterator it;

    escaped = "start # end";
    it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ("start  end", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();

    escaped = "start # # end";
    it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ("start   end", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();

    escaped = "# end";
    it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(" end", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();

    escaped = "# # end";
    it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ("  end", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();

    escaped = "start # ";
    it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ("start  ", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();
}

TEST(StringUtilities, UnescapeNonEscaped)
{
    for (char c = 0x1F + 1; c < 0x79; ++c)
    {
        if (c == '#' || c == ' ')
        {
            continue;
        }
        std::string escaped(1, c);
        std::string unescaped;
        std::string::iterator it = Unescape(
            escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        EXPECT_EQ(escaped, unescaped);
        EXPECT_EQ(escaped.end(), it);
    }

    for (char c = 0x1F + 1; c < 0x79; ++c)
    {
        if (c == '#' || c == ' ')
        {
            continue;
        }
        std::string escaped(1, c);
        std::string unescaped;
        std::string::iterator it = Unescape(
            escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        EXPECT_EQ(escaped, unescaped);
        EXPECT_EQ(escaped.end(), it);
    }
}

TEST(StringUtilities, UnescapeUrl)
{
    std::string escaped, unescaped;
    std::string::iterator it;

    escaped = "htt#p";
    it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ("http", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();

    escaped = "htt##p";
    it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ("htt#p", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();

    escaped = "hthtt#ptp";
    it =
        Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ("hthttptp", unescaped);
    EXPECT_EQ(escaped.end(), it);
    unescaped.clear();
}

static void TestCmdLineArgvUnescape(std::string const& escaped,
                                    std::string const& expected,
                                    std::string::const_iterator endingPosition)
{
    std::string unescaped;
    unescaped.reserve(escaped.size());
    std::string::const_iterator actualEnd = CmdLineToArgvWUnescape(
        escaped.begin(), escaped.end(), std::back_inserter(unescaped));
    EXPECT_EQ(expected, unescaped);
    EXPECT_EQ(endingPosition, actualEnd);
}

static void TestCmdLineArgvUnescape(std::string const& escaped,
                                    std::string const& expected)
{
    return TestCmdLineArgvUnescape(escaped, expected, escaped.end());
}

TEST(StringUtilities, CmdLineToArgvWUnescapeNoEscapes)
{
    TestCmdLineArgvUnescape("\"nothing needs escaping here!\"",
                            "nothing needs escaping here!");
}

TEST(StringUtilities, CmdLineToArgvWUnescapeOneBackslash)
{
    TestCmdLineArgvUnescape("\"start\\end\"", "start\\end");
}

TEST(StringUtilities, CmdLineToArgvWUnescapeTwoBackslashes)
{
    TestCmdLineArgvUnescape("\"start\\\\end\"", "start\\\\end");
}

TEST(StringUtilities, CmdLineToArgvWUnescapedOneBackslashQuote)
{
    TestCmdLineArgvUnescape("\"start\\\"end\"", "start\"end");
}

TEST(StringUtilities, CmdLineToArgvWUnescapedTwoBackslashQuote)
{
    TestCmdLineArgvUnescape("\"start\\\\\"end\"", "start\\\"end");
}

TEST(StringUtilities, CmdLineToArgvWUnescapedThreeBackslashQuote)
{
    TestCmdLineArgvUnescape("\"start\\\\\\\"end\"", "start\\\"end");
}

TEST(StringUtilities, CmdLineToArgvWUnescapedAfterQuote)
{
    std::string escaped = "\"start end\"after";
    TestCmdLineArgvUnescape(escaped, "start end", escaped.begin() + 11);
}

TEST(StringUtiliites, CmdLineToArgvWUnescapedAfterQuoteWithEscapes)
{
    std::string escaped = "\"start\\\"end\"after";
    TestCmdLineArgvUnescape(escaped, "start\"end", escaped.begin() + 12);
}

TEST(StringUtilities, NoQuotesCmdArgVThrows)
{
    std::string escaped("example");
    ASSERT_THROW(
        CmdLineToArgvWUnescape(escaped.begin(), escaped.end(), escaped.begin()),
        MalformedEscapedSequence);
}

TEST(StringUtilities, EmptyCmdArgVThrows)
{
    std::string escaped;
    ASSERT_THROW(
        CmdLineToArgvWUnescape(escaped.begin(), escaped.end(), escaped.begin()),
        MalformedEscapedSequence);
}

TEST(StringUtilities, SimpleHeaderCorrect)
{
    std::string tested("Example");
    Header(tested);
    ASSERT_EQ("===================== Example ====================", tested);
}

TEST(StringUtilities, EvenHeaderCorrect)
{
    std::string tested("Foobar");
    Header(tested);
    ASSERT_EQ("===================== Foobar =====================", tested);
}

TEST(StringUtilities, TooLong)
{
    std::string tested(
        "FoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobar");
    Header(tested);
    ASSERT_EQ(
        "FoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobar",
        tested);
}
