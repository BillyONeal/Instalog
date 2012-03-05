#pragma once
#include <string>
#include <exception>
#include <windows.h>
namespace Instalog { namespace SystemFacades {

	class Win32Exception : public std::exception
	{
		DWORD errorCode_;
	public:
		Win32Exception(DWORD errorCode) : errorCode_(errorCode) {};
		static void Throw(DWORD lastError);
		static void ThrowFromLastError() { Throw(::GetLastError()); };
		static void ThrowFromNtError(NTSTATUS errorCode);
		DWORD GetErrorCode() const
		{
			return errorCode_;
		}
		std::wstring GetWideMessage() const;
		std::string GetCharMessage() const;
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
