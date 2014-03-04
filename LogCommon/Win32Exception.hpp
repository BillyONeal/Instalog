// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <cstdint>
#include <string>
#include <exception>
#include <windows.h>
#include <comdef.h>
#include <boost/config.hpp>

namespace Instalog
{
namespace SystemFacades
{

/// @brief    A nice wrapper for throwing exceptions around Win32 errors
class Win32Exception : public std::exception
{
    DWORD errorCode_;

    public:
    /// @brief    Constructor
    ///
    /// @param    errorCode    The error code
    Win32Exception(DWORD errorCode) : errorCode_(errorCode) {};

    /// Gets from last error as an exception_ptr.
    /// @return The last error as an exception_ptr.
    static std::exception_ptr FromLastError() BOOST_NOEXCEPT_OR_NOTHROW;

    /// Gets the supplied error as an exception_ptr.
    /// @return The supplied error as an exception_ptr.
    static std::exception_ptr FromWinError(DWORD errorCode) BOOST_NOEXCEPT_OR_NOTHROW;

    /// Initializes a Win32Exception from the from the given NT error.
    /// @param errorCode The NT error code.
    /// @return The supplied NT error as an exception_ptr.
    static std::exception_ptr FromNtError(NTSTATUS errorCode) BOOST_NOEXCEPT_OR_NOTHROW;

    /// @brief    Throws a specified error directly
    ///
    /// @param    lastError    The error code to throw (usually a captured error
    /// code)
    static void __declspec(noreturn) Throw(DWORD lastError);

    /// @brief    Throw from last error.
    static void __declspec(noreturn) ThrowFromLastError()
    {
        Throw(::GetLastError());
    }
    ;

    /// @brief    Throw from a specified NT error
    ///
    /// @param    errorCode    The error code to throw (usually a captured error
    /// code)
    static void __declspec(noreturn) ThrowFromNtError(NTSTATUS errorCode);

    /// @brief    Gets the error code.
    ///
    /// @return    The error code.
    DWORD GetErrorCode() const
    {
        return errorCode_;
    }

    /// @brief    Gets the character message.
    ///
    /// @return    The character message.
    std::string GetCharMessage() const;

    /// @brief    Gets the message for the exception
    ///
    /// @return    Exception message
    virtual const char* what() const
    {
        static std::string buff;
        buff = GetCharMessage();
        return buff.c_str();
    }
    ;
};

/// @brief    Exception for signaling ERROR_SUCCESS.
struct ErrorSuccessException : public Win32Exception
{
    ErrorSuccessException() : Win32Exception(ERROR_SUCCESS)
    {
    }
};

/// @brief    Exception for signaling ERROR_FILE_NOT_FOUND.
struct ErrorFileNotFoundException : public Win32Exception
{
    ErrorFileNotFoundException() : Win32Exception(ERROR_FILE_NOT_FOUND)
    {
    }
};

/// @brief    Exception for signaling ERROR_ACCESS_DENIED.
struct ErrorAccessDeniedException : public Win32Exception
{
    ErrorAccessDeniedException() : Win32Exception(ERROR_ACCESS_DENIED)
    {
    }
};

/// @brief    Exception for signaling ERROR_ALREADY_EXISTS.
struct ErrorAlreadyExistsException : public Win32Exception
{
    ErrorAlreadyExistsException() : Win32Exception(ERROR_ALREADY_EXISTS)
    {
    }
};

/// @brief    Exception for signaling ERROR_PATH_NOT_FOUND.
struct ErrorPathNotFoundException : public Win32Exception
{
    ErrorPathNotFoundException() : Win32Exception(ERROR_PATH_NOT_FOUND)
    {
    }
};

/// @brief    Exception for signaling ERROR_INVALID_PARAMETER.
struct ErrorInvalidParameterException : public Win32Exception
{
    ErrorInvalidParameterException() : Win32Exception(ERROR_INVALID_PARAMETER)
    {
    }
};

/// @brief    Exception for signaling ERROR_MOD_NOT_FOUND.
struct ErrorModuleNotFoundException : public Win32Exception
{
    ErrorModuleNotFoundException() : Win32Exception(ERROR_MOD_NOT_FOUND)
    {
    }
};

/// @brief    Exception for signaling ERROR_PROC_NOT_FOUND.
struct ErrorProcedureNotFoundException : public Win32Exception
{
    ErrorProcedureNotFoundException() : Win32Exception(ERROR_PROC_NOT_FOUND)
    {
    }
};

class HresultException : public std::exception
{
    std::string narrow;
    HRESULT hResult;

    public:
    HresultException(HRESULT hRes, std::string n);
    HRESULT GetErrorCode() const;
    std::string const& GetErrorStringA() const;
    virtual char const* what();
};

/**
 * @brief    Throws an exception from a HRESULT. This may be a Win32Exception
 * if
 *             the error is a system error; otherwise it is _com_error.
 *             
 * @param   hRes    The hResult to check.
 */
void __declspec(noreturn) ThrowFromHResult(HRESULT hRes);

/**
 * @brief    Throws if the hResult indicated is a failure.
 *
 * @param   hRes    The hResult to check.
 */
void ThrowIfFailed(HRESULT hRes);

std::string GetWin32ErrorMessage(DWORD errorCode);
std::uint32_t GetWin32ErrorFromNtError(NTSTATUS errorCode);
std::string GetHresultErrorMessage(HRESULT errorCode);

}
}
