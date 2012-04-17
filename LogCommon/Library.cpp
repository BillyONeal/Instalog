// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
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

	FormattedMessageLoader::FormattedMessageLoader( std::wstring const& filename )
		: Library( filename, LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE )
	{

	}

	std::wstring FormattedMessageLoader::GetFormattedMessage( DWORD const& messageId, std::vector<std::wstring> const& arguments )
	{
		if (arguments.size() > 0)
		{
			std::vector<DWORD_PTR> argumentPtrs;
			argumentPtrs.reserve(arguments.size());	

			for (std::vector<DWORD_PTR>::size_type i = 0; i < arguments.size(); ++i)
			{
				argumentPtrs.push_back(reinterpret_cast<DWORD_PTR>(arguments[i].c_str()));
			}

			wchar_t *messagePtr = NULL;
			if (!FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY,
					            hModule,
								messageId,
								LANG_SYSTEM_DEFAULT,
								reinterpret_cast<LPWSTR>(&messagePtr),
								0,
								reinterpret_cast<va_list*>(argumentPtrs.data())))
			{
				Win32Exception::ThrowFromLastError();
			}

			return std::wstring(messagePtr); // TODO: Bill, is this a memory leak?
		}
		else
		{
			wchar_t *messagePtr = NULL;
			if (!FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY,
				hModule,
				messageId,
				LANG_SYSTEM_DEFAULT,
				reinterpret_cast<LPWSTR>(&messagePtr),
				0,
				NULL))
			{
				Win32Exception::ThrowFromLastError();
			}

			return std::wstring(messagePtr); // TODO: Bill, is this a memory leak?	
		}
	}

}}
