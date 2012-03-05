#pragma once

#include <string>
#include <Windows.h>

namespace Instalog { namespace SystemFacades {

	class RuntimeDynamicLinker 
	{
		HMODULE hModule;
	public:
		RuntimeDynamicLinker(std::wstring const& filename);
		~RuntimeDynamicLinker();
	};

}}