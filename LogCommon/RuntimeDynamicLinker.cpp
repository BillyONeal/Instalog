// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "Win32Exception.hpp"
#include "RuntimeDynamicLinker.hpp"

namespace Instalog { namespace SystemFacades {

	RuntimeDynamicLinker::RuntimeDynamicLinker( std::wstring const& filename )
		: hModule(::LoadLibraryW(filename.c_str()))
	{
		if (hModule == NULL)
		{
			Win32Exception::ThrowFromLastError();
		}
	}

	RuntimeDynamicLinker::~RuntimeDynamicLinker()
	{
		::FreeLibrary(hModule);
	}

	RuntimeDynamicLinker& GetNtDll()
	{
		static RuntimeDynamicLinker ntdll(L"ntdll.dll");
		return ntdll;
	}

}}
