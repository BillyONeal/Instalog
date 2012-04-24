// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "File.hpp"
#include "Win32Exception.hpp"

#pragma comment(lib, "Version.lib")

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

	File::File( File && other )
	{
		hFile = other.hFile;
		other.hFile = INVALID_HANDLE_VALUE;
	}

	File::File()
		: hFile(INVALID_HANDLE_VALUE)
	{
	}

	File::~File()
	{
		if (hFile == INVALID_HANDLE_VALUE)
		{
			return;
		}
		::CloseHandle(hFile);
	}

	unsigned __int64 File::GetSize() const
	{
		BY_HANDLE_FILE_INFORMATION info = GetExtendedAttributes();

		unsigned __int64 highSize = info.nFileSizeHigh;
		highSize <<= 32;
		return highSize + info.nFileSizeLow;
	}

	DWORD File::GetAttributes() const
	{
		return GetExtendedAttributes().dwFileAttributes;
	}

	BY_HANDLE_FILE_INFORMATION File::GetExtendedAttributes() const
	{
		BY_HANDLE_FILE_INFORMATION info;

		if (GetFileInformationByHandle(hFile, &info) == false)
		{
			Win32Exception::ThrowFromLastError();
		}

		return info;
	}

	std::vector<char> File::ReadBytes( unsigned int bytesToRead ) const
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

	bool File::WriteBytes( std::vector<char> const& bytes )
	{
		DWORD bytesWritten;

		if (WriteFile(hFile, bytes.data(), static_cast<DWORD>(bytes.size()), &bytesWritten, NULL) == false)
		{
			Win32Exception::ThrowFromLastError();
		}

		if (bytesWritten == bytes.size())
			return true;
		else
			return false;
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

	std::wstring File::GetCompany(std::wstring const& filename)
	{
		DWORD infoSize = ::GetFileVersionInfoSizeW(filename.c_str(), 0);
		if (infoSize == 0)
		{
			Win32Exception::ThrowFromLastError();
		}
		std::vector<char> buff(infoSize);
		if (::GetFileVersionInfoW(filename.c_str(), 0, static_cast<DWORD>(buff.size()), buff.data()) == 0)
		{
			Win32Exception::ThrowFromLastError();
		}
		wchar_t const targetPath[] = L"\\StringFileInfo\\040904B0\\CompanyName";
		void * companyData;
		UINT len;
		if (::VerQueryValueW(buff.data(), targetPath, &companyData, &len) == 0)
		{
			Win32Exception::ThrowFromLastError();
		}
		return std::wstring(static_cast<wchar_t *>(companyData), len-1);
	}

	unsigned __int64 File::GetSize( std::wstring const& filename )
	{
		WIN32_FILE_ATTRIBUTE_DATA fad = File::GetExtendedAttributes(filename);
		unsigned __int64 size = fad.nFileSizeHigh;
		size <<= 32;
		size |= fad.nFileSizeLow;
		return size;
	}

	DWORD File::GetAttributes( std::wstring const& filename )
	{
		DWORD answer = ::GetFileAttributesW(filename.c_str());
		if (answer == INVALID_FILE_ATTRIBUTES)
		{
			Win32Exception::ThrowFromLastError();
		}
		return answer;
	}

	WIN32_FILE_ATTRIBUTE_DATA File::GetExtendedAttributes( std::wstring const& filename )
	{
		WIN32_FILE_ATTRIBUTE_DATA fad;
		if (::GetFileAttributesExW(filename.c_str(), GetFileExInfoStandard, &fad) == 0)
		{
			Win32Exception::ThrowFromLastError();
		}
		return fad;
	}

	File& File::operator=( File other )
	{
		File copied(std::move(other));
		std::swap(this->hFile, copied.hFile);
		return *this;
	}

	bool File::IsExclusiveFile( std::wstring const& fileName )
	{
		DWORD attribs = ::GetFileAttributesW(fileName.c_str());
		if (attribs == INVALID_FILE_ATTRIBUTES)
		{
			return false;
		}
		else
		{
			return (attribs & FILE_ATTRIBUTE_DIRECTORY) == 0;
		}
	}

}}
