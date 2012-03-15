#pragma once
#include <string>
#include <vector>
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
		std::vector<char> ReadBytes(int bytesToRead);
		static void Delete(std::wstring const& filename);
		static bool Exists(std::wstring const& filename);
		static bool IsDirectory(std::wstring const& filename);
		static bool IsExecutable(std::wstring const& filename);
	};

}}
