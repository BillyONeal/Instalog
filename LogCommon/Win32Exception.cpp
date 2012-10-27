// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <memory>
#include "Library.hpp"
#include "Com.hpp"
#include "StringUtilities.hpp"
#include "Win32Exception.hpp"

namespace Instalog { namespace SystemFacades {

	void Win32Exception::Throw(DWORD lastError)
	{
		switch(lastError)
		{
		case ERROR_SUCCESS: throw ErrorSuccessException(); break;
		case ERROR_FILE_NOT_FOUND: throw ErrorFileNotFoundException(); break;
		case ERROR_PATH_NOT_FOUND: throw ErrorPathNotFoundException(); break;
		case ERROR_ACCESS_DENIED: throw ErrorAccessDeniedException(); break;
		case ERROR_ALREADY_EXISTS: throw ErrorAlreadyExistsException(); break;
		case ERROR_INVALID_PARAMETER: throw ErrorInvalidParameterException(); break;
		case ERROR_MOD_NOT_FOUND: throw ErrorModuleNotFoundException(); break;
		case ERROR_PROC_NOT_FOUND: throw ErrorProcedureNotFoundException(); break;
		default: throw Win32Exception(lastError); break;
		}
	}

	struct LocalFreeHelper
	{
		void operator()(void * toFree)
		{
			::LocalFree(reinterpret_cast<HLOCAL>(toFree));
		};
	};

	std::wstring Win32Exception::GetWideMessage() const
	{
		std::unique_ptr<void, LocalFreeHelper> buff;
		LPWSTR buffPtr;
		DWORD bufferLength = ::FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			GetErrorCode(),
			0,
			reinterpret_cast<LPWSTR>(&buffPtr),
			0,
			NULL);
		buff.reset(buffPtr);
		return std::wstring(buffPtr, bufferLength);
	}

	std::string Win32Exception::GetCharMessage() const
	{
		std::unique_ptr<void, LocalFreeHelper> buff;
		LPSTR buffPtr;
		DWORD bufferLength = ::FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			GetErrorCode(),
			0,
			reinterpret_cast<LPSTR>(&buffPtr),
			0,
			NULL);
		buff.reset(buffPtr);
		return std::string(buffPtr, bufferLength);
	}

	void Win32Exception::ThrowFromNtError( NTSTATUS errorCode )
	{
		typedef ULONG (WINAPI *RtlNtStatusToDosErrorFunc)(
			__in  NTSTATUS Status
			);
		RtlNtStatusToDosErrorFunc conv = GetNtDll().GetProcAddress<RtlNtStatusToDosErrorFunc>("RtlNtStatusToDosError");
		Throw(conv(errorCode));
	}

	void ThrowIfFailed( HRESULT hRes )
	{
		if (FAILED(hRes))
		{
			ThrowFromHResult(hRes);
		}
	}

	void ThrowFromHResult( HRESULT hRes )
	{
		UniqueComPtr<IErrorInfo> iei;
		if (S_OK == ::GetErrorInfo(0, iei.PassAsOutParameter()) && (iei.Get() != nullptr))
		{
			// get the error description from the IErrorInfo 
			UniqueBstr bStr;
			iei->GetDescription(bStr.AsTarget());
            auto errorMessage = bStr.AsString();
			std::string narrowMessage(Instalog::ConvertUnicode(errorMessage));
			throw HresultException(hRes, errorMessage, narrowMessage);
		}
		else if (HRESULT_FACILITY(hRes) == FACILITY_ITF)
		{
			throw HresultException(hRes, L"Interface Specific", "Interface Specific");
		}
		else
		{
			Win32Exception::Throw(static_cast<DWORD>(hRes));
		}
	}


	HresultException::HresultException( HRESULT hRes, std::wstring w, std::string n )
		: hResult(hRes)
		, wide(std::move(w))
		, narrow(std::move(n))
	{ }

	HRESULT HresultException::GetErrorCode() const
	{
		return hResult;
	}

	std::string const& HresultException::GetErrorStringA() const
	{
		return narrow;
	}

	std::wstring const& HresultException::GetErrorStringW() const
	{
		return wide;
	}

	char const* HresultException::what()
	{
		return narrow.c_str();
	}

}} // namespace Instalog::SystemFacades
