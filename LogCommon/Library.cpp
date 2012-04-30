// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "Win32Exception.hpp"
#include "Library.hpp"

namespace Instalog { namespace SystemFacades {

	Library::Library( std::wstring const& filename, DWORD flags )
		: hModule(::LoadLibraryExW(filename.c_str(), NULL, flags))
	{
		if (hModule == NULL)
		{
			Win32Exception::ThrowFromLastError();
		}
	}

	Library::~Library()
	{
		::FreeLibrary(hModule);
	}

	RuntimeDynamicLinker& GetNtDll()
	{
		static RuntimeDynamicLinker ntdll(L"ntdll.dll");
		return ntdll;
	}

	RuntimeDynamicLinker::RuntimeDynamicLinker( std::wstring const& filename )
		: Library( filename, 0 )
	{

	}

    static bool IsVistaLater()
    {
        OSVERSIONINFOW info;
        info.dwOSVersionInfoSize = sizeof(info);
        ::GetVersionExW(&info);
        return info.dwMajorVersion >= 6;
    }

    static bool IsVistaLaterCache()
    {
        static bool isVistaLater = IsVistaLater();
        return isVistaLater;
    }

	FormattedMessageLoader::FormattedMessageLoader( std::wstring const& filename )
		: Library( filename, ( IsVistaLaterCache() ? LOAD_LIBRARY_AS_IMAGE_RESOURCE : 0 ) | LOAD_LIBRARY_AS_DATAFILE )
	{

	}

	std::wstring FormattedMessageLoader::GetFormattedMessage( DWORD const& messageId, std::vector<std::wstring> const& arguments )
	{
		std::vector<DWORD_PTR> argumentPtrs;
		argumentPtrs.reserve(arguments.size());	

		for (std::vector<DWORD_PTR>::size_type i = 0; i < arguments.size(); ++i)
		{
			argumentPtrs.push_back(reinterpret_cast<DWORD_PTR>(arguments[i].c_str()));
		}

		auto argPtr = reinterpret_cast<va_list*>(argumentPtrs.data());
		if (arguments.empty())
		{
			argPtr = nullptr;
		}

		wchar_t *messagePtr = nullptr;
		if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY,
			hModule,
			messageId,
			LANG_SYSTEM_DEFAULT,
			reinterpret_cast<LPWSTR>(&messagePtr),
			0,
			argPtr) == 0)
		{
			Win32Exception::ThrowFromLastError();
		}
		std::wstring answer (messagePtr);
		LocalFree(messagePtr);
		return std::move(answer);
	}

}}
