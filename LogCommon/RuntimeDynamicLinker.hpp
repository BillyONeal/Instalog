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

		template <typename FuncT>
		FuncT GetProcAddress(char const* functionName)
		{
			return reinterpret_cast<FuncT>(::GetProcAddress(hModule, functionName));
		}
	};

}}
