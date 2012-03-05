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

}}
