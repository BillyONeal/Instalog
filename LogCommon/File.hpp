#pragma once
#include <string>
#include <windows.h>

namespace Instalog { namespace SystemFacades {

	// Represents a plain Win32 file handle.
	class File
	{
		HANDLE hFile;
	public:
		File(
			std::wstring const&,
			DWORD = GENERIC_READ,
			DWORD = FILE_SHARE_READ | FILE_SHARE_WRITE,
			LPSECURITY_ATTRIBUTES = nullptr,
			DWORD = OPEN_EXISTING,
			DWORD = FILE_ATTRIBUTE_NORMAL
		);
		~File();
		static void Delete(std::wstring const&);
		static bool Exists(std::wstring const&);
	};

}}