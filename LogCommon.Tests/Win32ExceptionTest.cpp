// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include "../LogCommon/Win32Exception.hpp"

#pragma warning(disable:4702) // Unreachable code

using namespace Instalog::SystemFacades;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_CLASS(Win32ExceptionTests)
{
public:
    TEST_METHOD(CanGetNarrowName)
    {
        try
        {
            Win32Exception::Throw(ERROR_ACCESS_DENIED);
        }
        catch (Win32Exception const& ex)
        {
            Assert::AreEqual(0, std::strcmp("Access is denied.\r\n", ex.what()));
            Assert::AreEqual<std::string>("Access is denied.\r\n", ex.GetCharMessage());
        }
    }

    TEST_METHOD(CanGetWideName)
    {
        try
        {
            Win32Exception::Throw(ERROR_ACCESS_DENIED);
        }
        catch (Win32Exception const& ex)
        {
            Assert::AreEqual<std::wstring>(L"Access is denied.\r\n", ex.GetWideMessage());
        }
    }

    TEST_METHOD(CanGetErrorCode)
    {
        try
        {
            Win32Exception::Throw(ERROR_ACCESS_DENIED);
        }
        catch (Win32Exception const& ex)
        {
            Assert::AreEqual<DWORD>(ERROR_ACCESS_DENIED, ex.GetErrorCode());
        }
    }

    TEST_METHOD(CanThrowFromErrorCode)
    {
        Assert::ExpectException<ErrorAccessDeniedException>([] { Win32Exception::Throw(ERROR_ACCESS_DENIED); } );
    }

    TEST_METHOD(CanThrowFromLastError)
    {
        ::SetLastError(ERROR_ACCESS_DENIED);
        Assert::ExpectException<ErrorAccessDeniedException>([] { Win32Exception::ThrowFromLastError(); } );
    }

    TEST_METHOD(CanThrowFromNtError)
    {
        Assert::ExpectException<ErrorAccessDeniedException>([] { Win32Exception::ThrowFromNtError(0xC0000022); } );
    }

    TEST_METHOD(CanThrowGenericException)
    {
        try
        {
            Win32Exception::Throw(static_cast<DWORD>(-1));
        }
        catch (Win32Exception const& ex)
        {
            Assert::AreEqual<DWORD>(-1, ex.GetErrorCode());
        }
    }

    TEST_METHOD(CanThrowSuccess)
    {
        try
        {
            Win32Exception::Throw(ERROR_SUCCESS);
            Assert::Fail(L"Should not be reached");
        }
        catch (ErrorSuccessException const& ex)
        {
            Assert::AreEqual<DWORD>(ERROR_SUCCESS, ex.GetErrorCode());
        }
    }

    TEST_METHOD(CanThrowFileNotFound)
    {
        try
        {
            Win32Exception::Throw(ERROR_FILE_NOT_FOUND);
            Assert::Fail(L"Should not be reached");
        }
        catch (ErrorFileNotFoundException const& ex)
        {
            Assert::AreEqual<DWORD>(ERROR_FILE_NOT_FOUND, ex.GetErrorCode());
        }
    }

    TEST_METHOD(CanThrowAccessDenied)
    {
        try
        {
            Win32Exception::Throw(ERROR_ACCESS_DENIED);
            Assert::Fail(L"Should not be reached");
        }
        catch (ErrorAccessDeniedException const& ex)
        {
            Assert::AreEqual<DWORD>(ERROR_ACCESS_DENIED, ex.GetErrorCode());
        }
    }

    TEST_METHOD(CanThrowAlreadyExists)
    {
        try
        {
            Win32Exception::Throw(ERROR_ALREADY_EXISTS);
            Assert::Fail(L"Should not be reached");
        }
        catch (ErrorAlreadyExistsException const& ex)
        {
            Assert::AreEqual<DWORD>(ERROR_ALREADY_EXISTS, ex.GetErrorCode());
        }
    }

    TEST_METHOD(CanThrowPathNotFound)
    {
        try
        {
            Win32Exception::Throw(ERROR_PATH_NOT_FOUND);
            Assert::Fail(L"Should not be reached");
        }
        catch (ErrorPathNotFoundException const& ex)
        {
            Assert::AreEqual<DWORD>(ERROR_PATH_NOT_FOUND, ex.GetErrorCode());
        }
    }

    TEST_METHOD(CanThrowInvalidParameter)
    {
        try
        {
            Win32Exception::Throw(ERROR_INVALID_PARAMETER);
            Assert::Fail(L"Should not be reached");
        }
        catch (ErrorInvalidParameterException const& ex)
        {
            Assert::AreEqual<DWORD>(ERROR_INVALID_PARAMETER, ex.GetErrorCode());
        }
    }

    TEST_METHOD(CanThrowModuleNotFound)
    {
        try
        {
            Win32Exception::Throw(ERROR_MOD_NOT_FOUND);
            Assert::Fail(L"Should not be reached");
        }
        catch (ErrorModuleNotFoundException const& ex)
        {
            Assert::AreEqual<DWORD>(ERROR_MOD_NOT_FOUND, ex.GetErrorCode());
        }
    }
};
