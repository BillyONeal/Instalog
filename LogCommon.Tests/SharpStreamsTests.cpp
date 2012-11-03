// Copyright © 2012 Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include "../LogCommon/SharpStreams.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Instalog::SharpStreams;

// I stole this list from the example table on http://en.wikipedia.org/wiki/UTF-1
// The revision was 7 September 2012‎

static const char *utf16LeExamples[] = {
    "\x7F\x00",
    "\x80\x00",
    "\x9F\x00",
    "\xA0\x00",
    "\xBF\x00",
    "\xC0\x00",
    "\xFF\x00",
    "\x00\x01",
    "\x5D\x01",
    "\x5E\x01",
    "\xBD\x01",
    "\xBE\x01",
    "\xFF\x07",
    "\x00\x08",
    "\xFF\x0F",
    "\x00\x10",
    "\x15\x40",
    "\x16\x40",
    "\xFF\xD7",
    "\x00\xE0",
    "\xFF\xF8",
    "\xD0\xFD",
    "\xEF\xFD",
    "\xFF\xFE",
    "\xFD\xFF",
    "\xFE\xFF",
    "\xFF\xFF",
    "\x00\xD8\x00\xDC",
    "\xA3\xD8\x2D\xDE",
    "\xA3\xD8\x2E\xDE",
    "\xBF\xDB\xFF\xDF",
    "\xC0\xDB\x00\xDC",
    "\xFF\xDB\xFF\xDF"
};

static const char *utf8Examples[] = {
    "\x7F",
    "\xC2\x80",
    "\xC2\x9F",
    "\xC2\xA0",
    "\xC2\xBF",
    "\xC3\x80",
    "\xC3\xBF",
    "\xC4\x80",
    "\xC5\x9D",
    "\xC5\x9E",
    "\xC6\xBD",
    "\xC6\xBE",
    "\xDF\xBF",
    "\xE0\xA0\x80",
    "\xE0\xBF\xBF",
    "\xE1\x80\x80",
    "\xE4\x80\x95",
    "\xE4\x80\x96",
    "\xED\x9F\xBF",
    "\xEE\x80\x80",
    "\xEF\xA3\xBF",
    "\xEF\xB7\x90",
    "\xEF\xB7\xAF",
    "\xEF\xBB\xBF",
    "\xEF\xBF\xBD",
    "\xEF\xBF\xBE",
    "\xEF\xBF\xBF",
    "\xF0\x90\x80\x80",
    "\xF0\xB8\xB8\xAD",
    "\xF0\xB8\xB8\xAE",
    "\xF3\xBF\xBF\xBF",
    "\xF4\x80\x80\x80",
    "\xF4\x8F\xBF\xBF"
};

TEST_CLASS(Utf8EncodersTest)
{
public:
    TEST_METHOD(RoundTripToUtf8)
    {
        Utf8Encoder encoder;
        for (std::size_t idx = 0; idx < _countof(utf16LeExamples); ++idx)
        {
            auto example = reinterpret_cast<const wchar_t*>(utf16LeExamples[idx]);
            auto exampleLength = std::wcslen(example);
            auto answer = encoder.GetBytes(example, static_cast<uint32_t>(exampleLength));
            auto rounded = encoder.GetChars(answer.data(), static_cast<uint32_t>(answer.size()));
            Assert::IsTrue(rounded.size() == exampleLength);
            Assert::IsTrue(std::equal(rounded.begin(), rounded.end(), example));
        }
    }

    TEST_METHOD(RoundTripToUtf16)
    {
        Utf8Encoder encoder;
        for (std::size_t idx = 0; idx < _countof(utf8Examples); ++idx)
        {
            auto example = utf8Examples[idx];
            auto exampleLength = std::strlen(example);
            auto answer = encoder.GetChars(reinterpret_cast<const unsigned char*>(example), static_cast<uint32_t>(exampleLength));
            auto rounded = encoder.GetBytes(answer.data(), static_cast<uint32_t>(answer.size()));
            Assert::IsTrue(rounded.size() == exampleLength);
            Assert::IsTrue(std::equal(rounded.begin(), rounded.end(), reinterpret_cast<const unsigned char*>(example)));
        }
    }
};
