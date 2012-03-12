#pragma once
#include <string>
#include <Windows.h>
#include "Win32Exception.hpp"

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
			FuncT answer = reinterpret_cast<FuncT>(::GetProcAddress(hModule, functionName));
			if (!answer)
			{
				Win32Exception::ThrowFromLastError();
			}
			return answer;
		}
	};

	RuntimeDynamicLinker& GetNtDll();
}}
