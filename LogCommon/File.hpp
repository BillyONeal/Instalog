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
		unsigned __int64 GetSize() const;
		DWORD GetAttributes() const;
		std::vector<char> ReadBytes(unsigned int bytesToRead) const;
		bool WriteBytes(std::vector<char> const& bytes); // TODO: const?
		std::wstring GetCompany() const;
		BY_HANDLE_FILE_INFORMATION GetExtendedAttributes() const;
		static unsigned __int64 GetSize(std::wstring const& filename);
		static DWORD GetAttributes(std::wstring const& filename);
		static WIN32_FILE_ATTRIBUTE_DATA GetExtendedAttributes(std::wstring const& filename);
		static void Delete(std::wstring const& filename);
		static bool Exists(std::wstring const& filename);
		static bool IsDirectory(std::wstring const& filename);
		static bool IsExecutable(std::wstring const& filename);
	};

}}
