#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include "Win32Exception.hpp"

namespace Instalog { namespace SystemFacades {

	class Library
	{
	protected:
		/// @summary	The module handle.
		HMODULE hModule;

		/// @brief	Constructor, opens a handle to the library with the given flags
		///
		/// @param	filename	Path of the library.
		/// @param	flags   	The flags.
		Library(std::wstring const& filename, DWORD flags);

		/// @brief	Destructor, frees the library.
		~Library();
	};

	/// @brief	An easy runtime dynamic linker
	class RuntimeDynamicLinker : public Library
	{
	public:
		/// @brief	Constructor.
		///
		/// @param	filename	Filename of the library.
		RuntimeDynamicLinker(std::wstring const& filename);

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

	class FormattedMessageLoader : public Library
	{
	public:
		/// @brief	Constructor.
		///
		/// @param	filename	Filename of the library containing the messages.
		FormattedMessageLoader(std::wstring const& filename);

		std::wstring GetFormattedMessage(DWORD const& messageId, std::vector<std::wstring> const& arguments = std::vector<std::wstring>() );
	};
}}
