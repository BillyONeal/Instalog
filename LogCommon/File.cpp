#include "pch.hpp"
#include "File.hpp"
#include "Win32Exception.hpp"

namespace Instalog { namespace SystemFacades {

	File::File( 
		std::wstring const& filename, 
		DWORD desiredAccess, 
		DWORD shareMode, 
		LPSECURITY_ATTRIBUTES securityAttributes, 
		DWORD createdDisposition, 
		DWORD flags
	)
		: hFile(::CreateFileW(filename.c_str(), desiredAccess, shareMode, securityAttributes, createdDisposition, flags, nullptr))
	{
		if (hFile == INVALID_HANDLE_VALUE)
		{
			Win32Exception::ThrowFromLastError();
		}
	}

	File::~File()
	{
		::CloseHandle(hFile);
	}

	std::vector<char> File::ReadBytes( int bytesToRead )
	{
		std::vector<char> bytes(bytesToRead);
		DWORD bytesRead = 0;

		if (::ReadFile(hFile, bytes.data(), bytesToRead, &bytesRead, NULL) == false)
		{
			Win32Exception::ThrowFromLastError();
		}
		
		if (bytesRead < bytesToRead)
		{
			bytes.resize(bytesRead);
		}

		return bytes;
	}

	void File::Delete( std::wstring const& filename)
	{
		if (::DeleteFileW(filename.c_str()) == 0) 
		{
			Win32Exception::ThrowFromLastError();
		}
	}

	bool File::Exists( std::wstring const& filename)
	{
		DWORD attributes = ::GetFileAttributesW(filename.c_str());

		return attributes != INVALID_FILE_ATTRIBUTES;
	}

	bool File::IsDirectory( std::wstring const& filename)
	{
		DWORD attributes = ::GetFileAttributesW(filename.c_str());

		return attributes == FILE_ATTRIBUTE_DIRECTORY;
	}

	bool File::IsExecutable( std::wstring const& filename )
	{
		if (Exists(filename) == false)
			return false;
		if (IsDirectory(filename))
			return false;

		File executable = File(filename);
		std::vector<char> bytes = executable.ReadBytes(2);
		return bytes[0] == 'M' && bytes[1] == 'Z';
	}

}}
