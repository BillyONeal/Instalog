#pragma once
#include <string>
#include <exception>
#include <windows.h>
namespace Instalog { namespace SystemFacades {

	/// @brief	A nice wrapper for throwing exceptions around Win32 errors
	class Win32Exception : public std::exception
	{
		DWORD errorCode_;
	public:
		/// @brief	Constructor
		///
		/// @param	errorCode	The error code
		Win32Exception(DWORD errorCode) : errorCode_(errorCode) {};

		/// @brief	Throws a specified error directly
		///
		/// @param	lastError	The error code to throw (usually a captured error code)
		static void Throw(DWORD lastError);

		/// @brief	Throw from last error.
		static void ThrowFromLastError() { Throw(::GetLastError()); };

		/// @brief	Throw from a specified NT error
		///
		/// @param	errorCode	The error code to throw (usually a captured error code)
		static void ThrowFromNtError(NTSTATUS errorCode);

		/// @brief	Gets the error code.
		///
		/// @return	The error code.
		DWORD GetErrorCode() const
		{
			return errorCode_;
		}

		/// @brief	Gets the wide message.
		///
		/// @return	The wide message.
		std::wstring GetWideMessage() const;

		/// @brief	Gets the character message.
		///
		/// @return	The character message.
		std::string GetCharMessage() const;

		/// @brief	Gets the message for the exception
		///
		/// @return	Exception message
		virtual const char* what() const
		{
			static std::string buff;
			buff = GetCharMessage();
			return buff.c_str();
		};
	};

	struct ErrorSuccessException : public Win32Exception
	{
		ErrorSuccessException() : Win32Exception(ERROR_SUCCESS) {}
	};
	struct ErrorFileNotFoundException : public Win32Exception
	{
		ErrorFileNotFoundException() : Win32Exception(ERROR_FILE_NOT_FOUND) {}
	};
	struct ErrorAccessDeniedException : public Win32Exception
	{
		ErrorAccessDeniedException() : Win32Exception(ERROR_ACCESS_DENIED) {}
	};
	struct ErrorAlreadyExistsException : public Win32Exception
	{
		ErrorAlreadyExistsException() : Win32Exception(ERROR_ALREADY_EXISTS) {}
	};
	struct ErrorPathNotFoundException : public Win32Exception
	{
		ErrorPathNotFoundException() : Win32Exception(ERROR_PATH_NOT_FOUND) {}
	};
	struct ErrorInvalidParameterException : public Win32Exception
	{
		ErrorInvalidParameterException() : Win32Exception(ERROR_INVALID_PARAMETER) {}
	};
	struct ErrorModuleNotFoundException : public Win32Exception
	{
		ErrorModuleNotFoundException() : Win32Exception(ERROR_MOD_NOT_FOUND) {}
	};
}}
