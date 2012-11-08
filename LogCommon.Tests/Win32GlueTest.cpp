// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include "../LogCommon/Win32Glue.hpp"

using namespace Instalog;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_CLASS(Win32GlueTest)
{
public:
    TEST_METHOD(SecondsSince1970Test)
    {
        SYSTEMTIME jan1970plusDay = { 1970, 1, 4,2,0,0,0,0};
        Assert::AreEqual<DWORD>(86400, SecondsSince1970(jan1970plusDay));
    }

    TEST_METHOD(SystemtimeFromSecondsSince1970Zero)
    {
        SYSTEMTIME expected = { 1970, 1, 4,1,0,0,0,0};
        SYSTEMTIME actual = SystemtimeFromSecondsSince1970(SecondsSince1970(expected));
        Assert::AreEqual<long>(expected.wYear, actual.wYear);
        Assert::AreEqual<long>(expected.wMonth, actual.wMonth);
        Assert::AreEqual<long>(expected.wDayOfWeek, actual.wDayOfWeek);
        Assert::AreEqual<long>(expected.wDay, actual.wDay);
        Assert::AreEqual<long>(expected.wHour, actual.wHour);
        Assert::AreEqual<long>(expected.wMinute, actual.wMinute);
        Assert::AreEqual<long>(expected.wSecond, actual.wSecond);
        Assert::AreEqual<long>(expected.wMilliseconds, actual.wMilliseconds);
    }

    TEST_METHOD(SystemtimeFromSecondsSince1970Ones)
    {
        SYSTEMTIME expected = { 1971, 2, 1,1,1,1,1,0};
        SYSTEMTIME actual = SystemtimeFromSecondsSince1970(SecondsSince1970(expected));
        Assert::AreEqual<long>(expected.wYear, actual.wYear);
        Assert::AreEqual<long>(expected.wMonth, actual.wMonth);
        Assert::AreEqual<long>(expected.wDayOfWeek, actual.wDayOfWeek);
        Assert::AreEqual<long>(expected.wDay, actual.wDay);
        Assert::AreEqual<long>(expected.wHour, actual.wHour);
        Assert::AreEqual<long>(expected.wMinute, actual.wMinute);
        Assert::AreEqual<long>(expected.wSecond, actual.wSecond);
        Assert::AreEqual<long>(expected.wMilliseconds, actual.wMilliseconds);
    }
};
