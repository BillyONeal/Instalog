// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include "LogCommon/Win32Glue.hpp"

using namespace Instalog;

TEST(Win32Glue, SecondsSince1970)
{
    SYSTEMTIME jan1970plusDay = { 1970, 1, 4,2,0,0,0,0};
    ASSERT_EQ(86400, SecondsSince1970(jan1970plusDay));
}

TEST(Win32Glue, SystemtimeFromSecondsSince1970Zero)
{
    SYSTEMTIME expected = { 1970, 1, 4,1,0,0,0,0};
    SYSTEMTIME actual = SystemtimeFromSecondsSince1970(SecondsSince1970(expected));
    EXPECT_EQ(expected.wYear, actual.wYear);
    EXPECT_EQ(expected.wMonth, actual.wMonth);
    EXPECT_EQ(expected.wDayOfWeek, actual.wDayOfWeek);
    EXPECT_EQ(expected.wDay, actual.wDay);
    EXPECT_EQ(expected.wHour, actual.wHour);
    EXPECT_EQ(expected.wMinute, actual.wMinute);
    EXPECT_EQ(expected.wSecond, actual.wSecond);
    EXPECT_EQ(expected.wMilliseconds, actual.wMilliseconds);
}

TEST(Win32Glue, SystemtimeFromSecondsSince1970Ones)
{
    SYSTEMTIME expected = { 1971, 2, 1,1,1,1,1,0};
    SYSTEMTIME actual = SystemtimeFromSecondsSince1970(SecondsSince1970(expected));
    EXPECT_EQ(expected.wYear, actual.wYear);
    EXPECT_EQ(expected.wMonth, actual.wMonth);
    EXPECT_EQ(expected.wDayOfWeek, actual.wDayOfWeek);
    EXPECT_EQ(expected.wDay, actual.wDay);
    EXPECT_EQ(expected.wHour, actual.wHour);
    EXPECT_EQ(expected.wMinute, actual.wMinute);
    EXPECT_EQ(expected.wSecond, actual.wSecond);
    EXPECT_EQ(expected.wMilliseconds, actual.wMilliseconds);
}