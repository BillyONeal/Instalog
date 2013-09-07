// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.
#include "pch.hpp"
#include <string>
#include "gtest/gtest.h"
#include "../LogCommon/Expected.hpp"

using namespace Instalog;

TEST(ExpectedOfT, DefaultConstructedThrowsOnGet)
{
    expected<std::wstring> uut;
    ASSERT_THROW(uut.get(), uninitalized_expected);
}

TEST(ExpectedOfT, DefaultConstructedThrowsOnRethrow)
{
    expected<std::wstring> uut;
    ASSERT_THROW(uut.rethrow(), uninitalized_expected);
}

TEST(ExpectedOfT, DefaultConstructedIsNotValid)
{
    expected<std::wstring> uut;
    ASSERT_FALSE(uut.is_valid());
}

TEST(ExpectedOfT, IfNoExceptionInFlightFromExceptionIsUninitialized)
{
    ASSERT_THROW(expected<std::wstring>::from_exception().rethrow(),
                 uninitalized_expected);
}

TEST(ExpectedOfT, CanContainCopiedT)
{
    std::wstring lvalue(L"This is a string");
    expected<std::wstring> uut(lvalue);
    ASSERT_STREQ(L"This is a string", uut.get().c_str());
}

TEST(ExpectedOfT, CanContainMovedT)
{
    expected<std::wstring> uut(L"This is a string");
    ASSERT_STREQ(L"This is a string", uut.get().c_str());
}

TEST(ExpectedOfT, CanContainExceptionInFlight)
{
    expected<std::wstring> uut;
    try
    {
        throw std::bad_alloc();
    }
    catch (...)
    {
        uut = expected<std::wstring>::from_exception();
    }
    ASSERT_THROW(uut.rethrow(), std::bad_alloc);
}

TEST(ExpectedOfT, CanCopyT)
{
    expected<std::wstring> uut(L"I am a string");
    expected<std::wstring> copied(uut);
    ASSERT_STREQ(uut.get().c_str(), copied.get().c_str());
}

TEST(ExpectedOfT, CanMoveT)
{
    expected<std::wstring> uut(L"I am a string");
    expected<std::wstring> moved(std::move(uut));
    ASSERT_STREQ(L"", uut.get().c_str());
    ASSERT_STREQ(L"I am a string", moved.get().c_str());
}

TEST(ExpectedOfT, CanCopyConstruct)
{
    expected<std::wstring> uut(
        expected<std::wstring>::from_exception(std::bad_alloc()));
    expected<std::wstring> copy(uut);
    ASSERT_THROW(uut.rethrow(), std::bad_alloc);
}

TEST(ExpectedOfT, SwapValids)
{
    expected<std::wstring> uutOne(L"Example");
    expected<std::wstring> uutTwo(L"Other");
    swap(uutOne, uutTwo);
    ASSERT_STREQ(L"Other", uutOne.get().c_str());
    ASSERT_STREQ(L"Example", uutTwo.get().c_str());
}

TEST(ExpectedOfT, SwapLeftValidRightInvalid)
{
    expected<std::wstring> uutOne(L"Foo");
    expected<std::wstring> uutTwo(
        expected<std::wstring>::from_exception(std::bad_alloc()));
    uutOne.swap(uutTwo);
    ASSERT_STREQ(L"Foo", uutTwo.get().c_str());
    ASSERT_THROW(uutOne.rethrow(), std::bad_alloc);
}

TEST(ExpectedOfT, SwapLeftInvalidRightValid)
{
    expected<std::wstring> uutOne(L"Foo");
    expected<std::wstring> uutTwo(
        expected<std::wstring>::from_exception(std::bad_alloc()));
    uutTwo.swap(uutOne);
    ASSERT_STREQ(L"Foo", uutTwo.get().c_str());
    ASSERT_THROW(uutOne.rethrow(), std::bad_alloc);
}
