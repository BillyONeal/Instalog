// Copyright © Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include "../LogCommon/LogSink.hpp"

using namespace Instalog;

TEST(StringSink, WriteToStringSink)
{
    string_sink sink;
    sink.append("hello", 5);
    ASSERT_EQ("hello", sink.get());
}

TEST(StringSink, WriteIsNotNullTerminated)
{
    string_sink sink;
    std::string testData("hello");
    testData.push_back('\0');
    testData.append("world");
    sink.append(testData.c_str(), testData.size());
    ASSERT_EQ(testData, sink.get());
}

TEST(ValueFormatters, CanFormatStdString)
{
    std::string testData("example");
    auto const result = format_value(testData);
    ASSERT_EQ(testData.size(), result.size());
    std::string resultStr(result.data(), result.size());
    ASSERT_STREQ(testData.c_str(), resultStr.c_str());
}

TEST(ValueFormatters, CanFormatIntegers)
{
    auto const result = format_value(42);
    ASSERT_EQ(2, result.size());
    std::string resultStr(result.data(), result.size());
    ASSERT_STREQ("42", resultStr.c_str());
}

TEST(ValueFormatters, CanFormatBigIntegers)
{
    auto const result = format_value(std::numeric_limits<int>::min());
    ASSERT_EQ(11, result.size());
    std::string resultStr(result.data(), result.size());
    ASSERT_STREQ("-2147483648", resultStr.c_str());
}

TEST(WriteFormat, SanityConcat)
{
    string_sink sink;
    write(sink, "Hello", ' ', "world!");
    ASSERT_EQ(static_cast<std::string>("Hello world!"), sink.get());
}

TEST(WriteFormat, Integer)
{
    std::string sink;
    write(sink, "Value is: ", 1729);
    ASSERT_STREQ("Value is: 1729", sink.c_str());
}

TEST(WriteFormat, WithNewline)
{
    std::string sink;
    writeln(sink, "What follows is a newline:");
    write(sink, sink.size(), " bytes written so far");
#ifdef BOOST_WINDOWS
    char const* expected = "What follows is a newline:\r\n28 bytes written so far";
#else
    char const* expected = "What follows is a newline:\n27 bytes written so far";
#endif
    ASSERT_STREQ(expected, sink.c_str());
}

TEST(WriteFormat, AllIntegralTypes)
{
    short s = -1729;
    int i = -1730;
    long l = -1731;
    long long ll = -1732;
    std::string sink;
    write(sink, "Short: ", s, " Int: ", i, " Long: ", l, " Long Long: ", ll);
    ASSERT_STREQ("Short: -1729 Int: -1730 Long: -1731 Long Long: -1732", sink.c_str());
}

TEST(WriteFormat, AllUnsignedIntegralTypes)
{
    unsigned short s = 1729;
    unsigned int i = 1730;
    unsigned long l = 1731;
    unsigned long long ll = 1732;
    std::string sink;
    write(sink, "Short: ", s, " Int: ", i, " Long: ", l, " Long Long: ", ll);
    ASSERT_STREQ("Short: 1729 Int: 1730 Long: 1731 Long Long: 1732", sink.c_str());
}

TEST(WriteFormat, FloatingPointTypes)
{
    float f = 3.14f;
    double d = 2.718;
    std::string sink;
    write(sink, "Float: ", f, " Double: ", d);
    ASSERT_STREQ("Float: 3.14 Double: 2.718", sink.c_str());
}

TEST(WriteFormat, PaddedNumberSmall)
{
    std::string sink;
    write(sink, pad(10, 'A', 1729), pad(10, '0', 1730));
    ASSERT_STREQ("AAAAAA17290000001730", sink.c_str());
}

TEST(WriteFormat, PaddedNumberBig)
{
    std::string sink;
    write(sink, pad(2, 'A', 1729));
    ASSERT_STREQ("1729", sink.c_str());
}

TEST(WriteFormat, Hex)
{
    std::uint32_t example = 0xDEADBEEF;
    std::string sink;
    write(sink, hex(example));
    ASSERT_STREQ("DEADBEEF", sink.c_str());
}

TEST(WriteFormat, HexSigned)
{
    std::int32_t example = static_cast<std::int32_t>(0xDEADBEEF);
    std::string sink;
    write(sink, hex(example));
    ASSERT_STREQ("DEADBEEF", sink.c_str());
}

TEST(WriteFormat, Hex64)
{
    std::uint64_t example = 0xDEADBEEFC0FFEE00;
    std::string sink;
    write(sink, hex(example));
    ASSERT_STREQ("DEADBEEFC0FFEE00", sink.c_str());
}

TEST(WriteFormat, HexSigned64)
{
    std::int64_t example = static_cast<std::int64_t>(0xDEADBEEFC0FFEE00);
    std::string sink;
    write(sink, hex(example));
    ASSERT_STREQ("DEADBEEFC0FFEE00", sink.c_str());
}

static const wchar_t unicodeExample[] =
{
    0xD83D, 0xDD28, // Unicode hammer character
    L' ', L'i', L's', L' ', L'a', L' ', L'h', L'a', L'm', L'm', L'e', L'r', L' ',
    0x24B8, // Unicode copyright symbol
    L' ', L'i', L's', L' ', L'a', L' ', L'c', L'o', L'p', L'y', L'r', L'i', L'g', L'h', L't',
    L'\0'
};

static const char unicodeResult[] = "\xf0\x9f\x94\xa8 is a hammer \xe2\x92\xb8 is a copyright";

TEST(WriteFormat, WideStrings)
{
    std::wstring asString(unicodeExample);
    std::string sink;
    write(sink, asString);
    ASSERT_STREQ(unicodeResult, sink.c_str());
}

TEST(WriteFormat, WideCharacterArrays)
{
    std::string sink;
    write(sink, unicodeExample);
    ASSERT_STREQ(unicodeResult, sink.c_str());
}

TEST(WriteFormat, WideCharacter)
{
    std::string sink;
    write(sink, static_cast<wchar_t>(0x24B8)); // copyright symbol
    ASSERT_STREQ("\xe2\x92\xb8", sink.c_str());
}

TEST(WriteFormat, WideCharacterLeadingSurrogate)
{
    std::string sink;
    ASSERT_THROW(write(sink, static_cast<wchar_t>(0xD83D)), utf8::invalid_utf16);
}

TEST(WriteFormat, WideCharacterTrailingSurrogate)
{
    std::string sink;
    ASSERT_THROW(write(sink, static_cast<wchar_t>(0xDD28)), utf8::invalid_utf16);
}
