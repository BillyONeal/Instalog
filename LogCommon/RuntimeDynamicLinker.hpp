#pragma once
#include <string>
#include <Windows.h>
#include "Win32Exception.hpp"

namespace Instalog { namespace SystemFacades {

	/// @brief	An easy runtime dynamic linker
	/// 
	/// @details	Opens and closes libraries
	class RuntimeDynamicLinker 
	{
		HMODULE hModule;
	public:
		/// @brief	Constructor.
		///
		/// @param	filename	Filename of the library.
		RuntimeDynamicLinker(std::wstring const& filename);

		/// @brief	Destructor.  Frees the library
		~RuntimeDynamicLinker();

		/// @brief	Gets a function pointer to the specified function
		///
		/// @param	functionName	Name of the function.
		///
		/// @return	Function pointer
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

	/// @brief	Gets the Windows NT dll
	///
	/// @return	The Windows NT dll
	RuntimeDynamicLinker& GetNtDll();
}}
