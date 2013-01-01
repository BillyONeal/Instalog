// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <LogCommon/Win32Exception.hpp>

#pragma warning(disable:4702) // Unreachable code

using namespace Instalog::SystemFacades;

TEST(Win32Exception, CanGetNarrowName)
{
    try
    {
        Win32Exception::Throw(ERROR_ACCESS_DENIED);
    }
    catch (Win32Exception const& ex)
    {
        ASSERT_EQ(0, std::strcmp("Access is denied.\r\n", ex.what()));
        ASSERT_EQ("Access is denied.\r\n", ex.GetCharMessage());
    }
}

TEST(Win32Exception, CanGetWideName)
{
    try
    {
        Win32Exception::Throw(ERROR_ACCESS_DENIED);
    }
    catch (Win32Exception const& ex)
    {
        ASSERT_EQ(L"Access is denied.\r\n", ex.GetWideMessage());
    }
}

TEST(Win32Exception, CanGetErrorCode)
{
    try
    {
        Win32Exception::Throw(ERROR_ACCESS_DENIED);
    }
    catch (Win32Exception const& ex)
    {
        ASSERT_EQ(ERROR_ACCESS_DENIED, ex.GetErrorCode());
    }
}

TEST(Win32Exception, CanThrowFromErrorCode)
{
    ASSERT_THROW(Win32Exception::Throw(ERROR_ACCESS_DENIED), ErrorAccessDeniedException);
}

TEST(Win32Exception, CanThrowFromLastError)
{
    ::SetLastError(ERROR_ACCESS_DENIED);
    ASSERT_THROW(Win32Exception::ThrowFromLastError(), ErrorAccessDeniedException);
}

TEST(Win32Exception, CanThrowFromNtError)
{
    ASSERT_THROW(Win32Exception::ThrowFromNtError(0xC0000022), ErrorAccessDeniedException);
}

TEST(Win32Exception, CanThrowGenericException)
{
    try
    {
        Win32Exception::Throw(static_cast<DWORD>(-1));
    }
    catch (Win32Exception const& ex)
    {
        ASSERT_EQ(-1, ex.GetErrorCode());
    }
}

TEST(Win32Exception, CanThrowSuccess)
{
    try
    {
        Win32Exception::Throw(ERROR_SUCCESS);
        ASSERT_TRUE(false);
    }
    catch (ErrorSuccessException const& ex)
    {
        ASSERT_EQ(ERROR_SUCCESS, ex.GetErrorCode());
    }
}

TEST(Win32Exception, CanThrowFileNotFound)
{
    try
    {
        Win32Exception::Throw(ERROR_FILE_NOT_FOUND);
        ASSERT_TRUE(false);
    }
    catch (ErrorFileNotFoundException const& ex)
    {
        ASSERT_EQ(ERROR_FILE_NOT_FOUND, ex.GetErrorCode());
    }
}

TEST(Win32Exception, CanThrowAccessDenied)
{
    try
    {
        Win32Exception::Throw(ERROR_ACCESS_DENIED);
        ASSERT_TRUE(false);
    }
    catch (ErrorAccessDeniedException const& ex)
    {
        ASSERT_EQ(ERROR_ACCESS_DENIED, ex.GetErrorCode());
    }
}

TEST(Win32Exception, CanThrowAlreadyExists)
{
    try
    {
        Win32Exception::Throw(ERROR_ALREADY_EXISTS);
        ASSERT_TRUE(false);
    }
    catch (ErrorAlreadyExistsException const& ex)
    {
        ASSERT_EQ(ERROR_ALREADY_EXISTS, ex.GetErrorCode());
    }
}

TEST(Win32Exception, CanThrowPathNotFound)
{
    try
    {
        Win32Exception::Throw(ERROR_PATH_NOT_FOUND);
        ASSERT_TRUE(false);
    }
    catch (ErrorPathNotFoundException const& ex)
    {
        ASSERT_EQ(ERROR_PATH_NOT_FOUND, ex.GetErrorCode());
    }
}

TEST(Win32Exception, CanThrowInvalidParameter)
{
    try
    {
        Win32Exception::Throw(ERROR_INVALID_PARAMETER);
        ASSERT_TRUE(false);
    }
    catch (ErrorInvalidParameterException const& ex)
    {
        ASSERT_EQ(ERROR_INVALID_PARAMETER, ex.GetErrorCode());
    }
}

TEST(Win32Exception, CanThrowModuleNotFound)
{
    try
    {
        Win32Exception::Throw(ERROR_MOD_NOT_FOUND);
        ASSERT_TRUE(false);
    }
    catch (ErrorModuleNotFoundException const& ex)
    {
        ASSERT_EQ(ERROR_MOD_NOT_FOUND, ex.GetErrorCode());
    }
}
