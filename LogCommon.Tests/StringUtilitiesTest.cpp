// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include <string>
#include <limits>
#include <algorithm>
#include "../LogCommon/StringUtilities.hpp"

using namespace Instalog;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_CLASS(StringUtilities)
{
public:
    TEST_METHOD(General_EscapeCharacter)
    {
        std::wstring str = L"#";
        GeneralEscape(str);
        Assert::AreEqual<std::wstring>(L"##", str);
    }

    TEST_METHOD(General_RightDelimiter)
    {
        std::wstring str;

        str = L"[]";
        GeneralEscape(str);
        Assert::AreEqual<std::wstring>(L"[]", str);

        str = L"[]";
        GeneralEscape(str, L'#', L']');
        Assert::AreEqual<std::wstring>(L"[#]", str);
    }

    TEST_METHOD(General_NullCharacter)
    {
        std::wstring str;
        str.push_back(0x00);
        GeneralEscape(str);
        Assert::AreEqual<std::wstring>(L"#0", str);
    }

    TEST_METHOD(General_BackspaceCharacter)
    {
        std::wstring str;
        str.push_back(0x08);
        GeneralEscape(str);
        Assert::AreEqual<std::wstring>(L"#b", str);
    }

    TEST_METHOD(General_FormFeed)
    {
        std::wstring str;
        str.push_back(0x0c);
        GeneralEscape(str);
        Assert::AreEqual<std::wstring>(L"#f", str);
    }

    TEST_METHOD(General_Newline)
    {
        std::wstring str;
        str.push_back(0x0A);
        GeneralEscape(str);
        Assert::AreEqual<std::wstring>(L"#n", str);
    }

    TEST_METHOD(General_CarriageReturn)
    {
        std::wstring str;
        str.push_back(0x0D);
        GeneralEscape(str);
        Assert::AreEqual<std::wstring>(L"#r", str);
    }

    TEST_METHOD(General_HorizontalTab)
    {
        std::wstring str;
        str.push_back(0x09);
        GeneralEscape(str);
        Assert::AreEqual<std::wstring>(L"#t", str);
    }

    TEST_METHOD(General_VerticalTab)
    {
        std::wstring str;
        str.push_back(0x0B);
        GeneralEscape(str);
        Assert::AreEqual<std::wstring>(L"#v", str);
    }

    TEST_METHOD(General_OtherASCII)
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
            Assert::AreEqual<std::wstring>(std::wstring(expected), str);
        }

        {
            std::wstring str(1, 0x7F);
            GeneralEscape(str);
            Assert::AreEqual<std::wstring>(L"#x7F", str);
        }

        {
            std::wstring str(1, 0x7F);
            str.append(L"end");
            GeneralEscape(str);
            Assert::AreEqual<std::wstring>(L"#x7Fend", str);
        }
    }

    TEST_METHOD(General_Unicode)
    {
        srand((int)time(NULL));
        wchar_t increment = rand() % 10000;
        for (wchar_t c = 0x0080; c < std::numeric_limits<wchar_t>::max() - 10000; c += increment)
        {
            std::wstring str(1, c);
            GeneralEscape(str);
            wchar_t expected[7];
            swprintf_s(expected, 7, L"#u%04X", c);
            Assert::AreEqual<std::wstring>(std::wstring(expected), str);
            increment = rand() % 10000;
        }

        // Check the last 
        std::wstring str(1, std::numeric_limits<wchar_t>::max());
        GeneralEscape(str);
        wchar_t expected[7];
        swprintf_s(expected, 7, L"#u%04X", std::numeric_limits<wchar_t>::max());
        Assert::AreEqual<std::wstring>(std::wstring(expected), str);
    }

    TEST_METHOD(General_Whitespace)
    {
        std::wstring str;

        str = L"start  end";
        GeneralEscape(str);
        Assert::AreEqual<std::wstring>(L"start # end", str);

        str = L"start   end";
        GeneralEscape(str);
        Assert::AreEqual<std::wstring>(L"start # # end", str);

        str = L" end";
        GeneralEscape(str);
        Assert::AreEqual<std::wstring>(L"# end", str);

        str = L"  end";
        GeneralEscape(str);
        Assert::AreEqual<std::wstring>(L"# # end", str);

        str = L"start  ";
        GeneralEscape(str);
        Assert::AreEqual<std::wstring>(L"start # ", str);
    }

    TEST_METHOD(General_NonEscaped)
    {
        for (wchar_t c = 0x1F + 1; c < 0x79; ++c)
        {
            if (c == L'#' || c == L' ')
            {
                continue;
            }
            std::wstring str(1, c);
            GeneralEscape(str);
            Assert::AreEqual(std::wstring(1, c), str);
        }
    }

    TEST_METHOD(Url_EscapeCharacter)
    {
        std::wstring str = L"#";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"##", str);
    }

    TEST_METHOD(Url_RightDelimiter)
    {
        std::wstring str;
    
        str = L"[]";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"[]", str);

        str = L"[]";
        HttpEscape(str, L'#', L']');
        Assert::AreEqual<std::wstring>(L"[#]", str);
    }

    TEST_METHOD(Url_NullCharacter)
    {
        std::wstring str;
        str.push_back(0x00);
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"#0", str);
    }

    TEST_METHOD(Url_BackspaceCharacter)
    {
        std::wstring str;
        str.push_back(0x08);
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"#b", str);
    }

    TEST_METHOD(Url_FormFeed)
    {
        std::wstring str;
        str.push_back(0x0c);
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"#f", str);
    }

    TEST_METHOD(Url_Newline)
    {
        std::wstring str;
        str.push_back(0x0A);
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"#n", str);
    }

    TEST_METHOD(Url_CarriageReturn)
    {
        std::wstring str;
        str.push_back(0x0D);
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"#r", str);
    }

    TEST_METHOD(Url_HorizontalTab)
    {
        std::wstring str;
        str.push_back(0x09);
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"#t", str);
    }

    TEST_METHOD(Url_VerticalTab)
    {
        std::wstring str;
        str.push_back(0x0B);
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"#v", str);
    }

    TEST_METHOD(Url_OtherASCII)
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
            Assert::AreEqual(std::wstring(expected), str);
        }

        {
            std::wstring str(1, 0x7F);
            HttpEscape(str);
            Assert::AreEqual<std::wstring>(L"#x7F", str);
        }
    
        {
            std::wstring str(1, 0x7F);
            str.append(L"end");
            HttpEscape(str);
            Assert::AreEqual<std::wstring>(L"#x7Fend", str);
        }
    }

    TEST_METHOD(Url_Unicode)
    {
        srand((int)time(NULL));
        wchar_t increment = rand() % 10000;
        for (wchar_t c = 0x0080; c < std::numeric_limits<wchar_t>::max() - 10000; c += increment)
        {
            std::wstring str(1, c);
            HttpEscape(str);
            wchar_t expected[7];
            swprintf_s(expected, 7, L"#u%04X", c);
            Assert::AreEqual(std::wstring(expected), str);
            increment = rand() % 10000;
        }

        // Check the last 
        std::wstring str(1, std::numeric_limits<wchar_t>::max());
        HttpEscape(str);
        wchar_t expected[7];
        swprintf_s(expected, 7, L"#u%04X", std::numeric_limits<wchar_t>::max());
        Assert::AreEqual(std::wstring(expected), str);
    }

    TEST_METHOD(Url_Whitespace)
    {
        std::wstring str;

        str = L"start  end";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"start # end", str);

        str = L"start   end";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"start # # end", str);

        str = L" end";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"# end", str);

        str = L"  end";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"# # end", str);

        str = L"start  ";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"start # ", str);
    }

    TEST_METHOD(Url_NonEscaped)
    {
        for (wchar_t c = 0x1F + 1; c < 0x79; ++c)
        {
            if (c == L'#' || c == L' ')
            {
                continue;
            }
            std::wstring str(1, c);
            HttpEscape(str);
            Assert::AreEqual(std::wstring(1, c), str);
        }
    }

    TEST_METHOD(Url_UrlEscape)
    {
        std::wstring str;

        str = L"http";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"htt#p", str);

        str = L"HTTP";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"htt#p", str);

        str = L"hTtP";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"htt#p", str);
    
        str = L"hthttptp";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"hthtt#ptp", str);

        str = L"htt#p";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"htt##p", str);

        str = L"http://go.microsoft.com/";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"htt#p://go.microsoft.com/", str);

        str = L"HTTp://go.microsoft.com/";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"htt#p://go.microsoft.com/", str);

        str = L"hTtp://go.microsoft.com/";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"htt#p://go.microsoft.com/", str);

        str = L"hthttptp://go.microsoft.com/";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"hthtt#ptp://go.microsoft.com/", str);

        str = L"htt#p://go.microsoft.com/";
        HttpEscape(str);
        Assert::AreEqual<std::wstring>(L"htt##p://go.microsoft.com/", str);
    }

    TEST_METHOD(UnescapeEmpty)
    {
        std::wstring escaped = L"";
        std::wstring unescaped;
        std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual<std::wstring>(L"", unescaped);
        Assert::IsTrue(escaped.end() == it);
    }

    TEST_METHOD(UnescapeEndAtRightDelimiter)
    {
        std::wstring escaped;
        std::wstring unescaped;
        std::wstring::iterator it;

        escaped = (L"string of interest]extra");
        it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped), L'#', L']');
        Assert::AreEqual<std::wstring>(L"string of interest", unescaped);
        Assert::IsTrue(find(escaped.begin(), escaped.end(), L']') == it);

        escaped = (L"##a]extra");
        unescaped.clear();
        it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped), L'#', L']');
        Assert::AreEqual<std::wstring>(L"#a", unescaped);
        Assert::IsTrue(find(escaped.begin(), escaped.end(), L']') == it);
    }

    TEST_METHOD(UnescapeMalformed)
    {
        std::wstring escaped = L"#";
        std::wstring unescaped;
        Assert::ExpectException<MalformedEscapedSequence>([&] { Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped)); } );
    }

    TEST_METHOD(UnescapeEscapeCharacter)
    {
        std::wstring escaped = L"##";
        std::wstring unescaped;
        std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual<std::wstring>(L"#", unescaped);
        Assert::IsTrue(escaped.end() == it);
    }

    TEST_METHOD(UnescapeNullCharacter)
    {
        std::wstring escaped = L"#0";
        std::wstring unescaped;
        std::wstring expected;
        expected.push_back(L'\0');
        std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual(expected, unescaped);
        Assert::IsTrue(escaped.end() == it);
    }

    TEST_METHOD(UnescapeBackspaceCharacter)
    {
        std::wstring escaped = L"#b";
        std::wstring unescaped;
        std::wstring expected;
        expected.push_back(0x08);
        std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual(expected, unescaped);
        Assert::IsTrue(escaped.end() == it);
    }

    TEST_METHOD(UnescapeFormFeedCharacter)
    {
        std::wstring escaped = L"#f";
        std::wstring unescaped;
        std::wstring expected;
        expected.push_back(0x0C);
        std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual(expected, unescaped);
        Assert::IsTrue(escaped.end() == it);
    }

    TEST_METHOD(UnescapeNewlineCharacter)
    {
        std::wstring escaped = L"#n";
        std::wstring unescaped;
        std::wstring expected;
        expected.push_back(0x0A);
        std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual(expected, unescaped);
        Assert::IsTrue(escaped.end() == it);
    }

    TEST_METHOD(UnescapeCarriageReturnCharacter)
    {
        std::wstring escaped = L"#r";
        std::wstring unescaped;
        std::wstring expected;
        expected.push_back(0x0D);
        std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual(expected, unescaped);
        Assert::IsTrue(escaped.end() == it);
    }

    TEST_METHOD(UnescapeHorizontalTabCharacter)
    {
        std::wstring escaped = L"#t";
        std::wstring unescaped;
        std::wstring expected;
        expected.push_back(0x09);
        std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual(expected, unescaped);
        Assert::IsTrue(escaped.end() == it);
    }

    TEST_METHOD(UnescapeVerticalTabCharacter)
    {
        std::wstring escaped = L"#v";
        std::wstring unescaped;
        std::wstring expected;
        expected.push_back(0x0B);
        std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual(expected, unescaped);
        Assert::IsTrue(escaped.end() == it);
    }

    TEST_METHOD(UnescapeOtherASCII)
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
            Assert::AreEqual(expected, unescaped);
            Assert::IsTrue(escaped.end() == it);
        }

        {
            std::wstring escaped = L"#x7F";
            std::wstring unescaped;
            std::wstring expected;
            expected.push_back(0x7F);
            std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
            Assert::AreEqual(expected, unescaped);
            Assert::IsTrue(escaped.end() == it);
        }
    }

    TEST_METHOD(UnescapeUnicode)
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
            Assert::AreEqual(expected, unescaped);
            Assert::IsTrue(escaped.end() == it);
        }

        {
            std::wstring escaped = L"#uFFFF";
            std::wstring unescaped;
            std::wstring expected;
            expected.push_back(0xFFFF);
            std::wstring::iterator it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
            Assert::AreEqual(expected, unescaped);
            Assert::IsTrue(escaped.end() == it);
        }
    }

    TEST_METHOD(UnescapeWhitespace)
    {
        std::wstring escaped, unescaped;
        std::wstring::iterator it;

        escaped = L"start # end";
        it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual<std::wstring>(L"start  end", unescaped);
        Assert::IsTrue(escaped.end() == it);
        unescaped.clear();

        escaped = L"start # # end";
        it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual<std::wstring>(L"start   end", unescaped);
        Assert::IsTrue(escaped.end() == it);
        unescaped.clear();

        escaped = L"# end";
        it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual<std::wstring>(L" end", unescaped);
        Assert::IsTrue(escaped.end() == it);
        unescaped.clear();

        escaped = L"# # end";
        it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual<std::wstring>(L"  end", unescaped);
        Assert::IsTrue(escaped.end() == it);
        unescaped.clear();

        escaped = L"start # ";
        it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual<std::wstring>(L"start  ", unescaped);
        Assert::IsTrue(escaped.end() == it);
        unescaped.clear();
    }

    TEST_METHOD(UnescapeNonEscaped)
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
            Assert::AreEqual(escaped, unescaped);
            Assert::IsTrue(escaped.end() == it);
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
            Assert::AreEqual(escaped, unescaped);
            Assert::IsTrue(escaped.end() == it);
        }
    }

    TEST_METHOD(UnescapeUrl)
    {
        std::wstring escaped, unescaped;
        std::wstring::iterator it;

        escaped = L"htt#p";
        it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual<std::wstring>(L"http", unescaped);
        Assert::IsTrue(escaped.end() == it);
        unescaped.clear();

        escaped = L"htt##p";
        it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual<std::wstring>(L"htt#p", unescaped);
        Assert::IsTrue(escaped.end() == it);
        unescaped.clear();

        escaped = L"hthtt#ptp";
        it = Unescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual<std::wstring>(L"hthttptp", unescaped);
        Assert::IsTrue(escaped.end() == it);
        unescaped.clear();
    }

    static void TestCmdLineArgvUnescape(std::wstring const& escaped, std::wstring const& expected, std::wstring::const_iterator endingPosition)
    {
        std::wstring unescaped;
        unescaped.reserve(escaped.size());
        std::wstring::const_iterator actualEnd = CmdLineToArgvWUnescape(escaped.begin(), escaped.end(), std::back_inserter(unescaped));
        Assert::AreEqual(expected, unescaped);
        Assert::IsTrue(endingPosition == actualEnd);
    }

    static void TestCmdLineArgvUnescape(std::wstring const& escaped, std::wstring const& expected)
    {
        return TestCmdLineArgvUnescape(escaped, expected, escaped.end());
    }

    TEST_METHOD(CmdLineToArgvWUnescapeNoEscapes)
    {
        TestCmdLineArgvUnescape(L"\"nothing needs escaping here!\"", L"nothing needs escaping here!");
    }

    TEST_METHOD(CmdLineToArgvWUnescapeOneBackslash)
    {
        TestCmdLineArgvUnescape(L"\"start\\end\"", L"start\\end");
    }

    TEST_METHOD(CmdLineToArgvWUnescapeTwoBackslashes)
    {
        TestCmdLineArgvUnescape(L"\"start\\\\end\"", L"start\\\\end");
    }

    TEST_METHOD(CmdLineToArgvWUnescapedOneBackslashQuote)
    {
        TestCmdLineArgvUnescape(L"\"start\\\"end\"", L"start\"end");
    }

    TEST_METHOD(CmdLineToArgvWUnescapedTwoBackslashQuote)
    {
        TestCmdLineArgvUnescape(L"\"start\\\\\"end\"", L"start\\\"end");
    }

    TEST_METHOD(CmdLineToArgvWUnescapedThreeBackslashQuote)
    {
        TestCmdLineArgvUnescape(L"\"start\\\\\\\"end\"", L"start\\\"end");
    }

    TEST_METHOD(CmdLineToArgvWUnescapedAfterQuote)
    {
        std::wstring escaped = L"\"start end\"after";
        TestCmdLineArgvUnescape(escaped, L"start end", escaped.begin() + 11);
    }

    TEST_METHOD(CmdLineToArgvWUnescapedAfterQuoteWithEscapes)
    {
        std::wstring escaped = L"\"start\\\"end\"after";
        TestCmdLineArgvUnescape(escaped, L"start\"end", escaped.begin() + 12);
    }

    TEST_METHOD(NoQuotesCmdArgVThrows)
    {
        std::wstring escaped(L"example");
        Assert::ExpectException<MalformedEscapedSequence>([&] { CmdLineToArgvWUnescape(escaped.begin(), escaped.end(), escaped.begin()); } );
    }

    TEST_METHOD(EmptyCmdArgVThrows)
    {
        std::wstring escaped;
        Assert::ExpectException<MalformedEscapedSequence>([&] { CmdLineToArgvWUnescape(escaped.begin(), escaped.end(), escaped.begin()); } );
    }

    TEST_METHOD(SimpleHeaderCorrect)
    {
        std::wstring tested(L"Example");
        Header(tested);
        Assert::AreEqual<std::wstring>(L"===================== Example ====================", tested);
    }

    TEST_METHOD(EvenHeaderCorrect)
    {
        std::wstring tested(L"Foobar");
        Header(tested);
        Assert::AreEqual<std::wstring>(L"===================== Foobar =====================", tested);
    }

    TEST_METHOD(TooLong)
    {
        std::wstring tested(L"FoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobar");
        Header(tested);
        Assert::AreEqual<std::wstring>(L"FoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobarFoobar", tested);
    }
};
