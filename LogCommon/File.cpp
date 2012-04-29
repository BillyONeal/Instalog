// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "File.hpp"
#include "Win32Exception.hpp"
#include <boost/algorithm/string/predicate.hpp>

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

		return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && (attributes != INVALID_FILE_ATTRIBUTES);
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
		if (len == 0)
		{
			return std::wstring();
		}
		else
		{
			return std::wstring(static_cast<wchar_t *>(companyData), len-1);
		}
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

	FindFiles::FindFiles( std::wstring const& pattern, bool recursive /*= false*/, bool includeRelativeSubPath /*= true*/, bool skipDotDirectories /*= true*/ )
		: recursive(recursive)
		, skipDotDirectories(skipDotDirectories)
		, rootPath(pattern)
		, includeRelativeSubPath(includeRelativeSubPath)
		, valid(true)
	{
		std::wstring::iterator chop = rootPath.end() - 1;
		for (; chop > rootPath.begin(); --chop)
		{
			if (*chop == L'\\')
			{
				break;
			}
		}
		for (; chop > rootPath.begin(); --chop)
		{
			if (*chop != L'\\')
			{
				break;
			}
		}
		rootPath.erase(chop + 1, rootPath.end());
		rootPath.append(L"\\");

		HANDLE handle = ::FindFirstFile(pattern.c_str(), &data);

		if (handle == INVALID_HANDLE_VALUE)
		{
			DWORD lastError = ::GetLastError();
			
			if (lastError == ERROR_FILE_NOT_FOUND || lastError == ERROR_PATH_NOT_FOUND)
			{
				valid = false;
				return;
			}
			else
			{
				Win32Exception::ThrowFromLastError();
			}
		}

		handles.push(handle);

		if (skipDotDirectories && data.cFileName[0] == L'.')
		{
			Next();
		}
	}

	FindFiles::~FindFiles()
	{
		while (handles.empty() == false)
		{
			::FindClose(handles.top());
			handles.pop();
		}
	}

	void FindFiles::Next()
	{
		bool alreadyIncludedSubPath = false;

		// Get the next file, skip . and .. if requested
		do 
		{
			if (::FindNextFile(handles.top(), &data) == false)
			{
				DWORD errorStatus = ::GetLastError();
				if (errorStatus == ERROR_NO_MORE_FILES)
				{
					::FindClose(handles.top());
					handles.pop();
					if (subPaths.empty() == false)
					{
						subPaths.pop();
					}				
					if (handles.empty())
					{
						valid = false;
						return;
					}
					else
					{
						return Next();
					}
				}
				else
				{
					Win32Exception::Throw(errorStatus);
				}
			}
		} while (skipDotDirectories && (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0));

		if (recursive)
		{
			if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(data.cFileName, L".") != 0 && wcscmp(data.cFileName, L"..") != 0)
			{
				if (subPaths.empty())
				{
					subPaths.push(std::wstring(data.cFileName).append(L"\\"));		
				}
				else
				{
					subPaths.push(std::wstring(subPaths.top()).append(data.cFileName).append(L"\\"));				
				}

				HANDLE handle = ::FindFirstFile(std::wstring(rootPath).append(subPaths.top()).append(L"*").c_str(), &data);
				if (handle == INVALID_HANDLE_VALUE)
				{
					Win32Exception::ThrowFromLastError();
				}

				handles.push(handle);

				if (skipDotDirectories)
				{
					FindFiles::Next();
					alreadyIncludedSubPath = true;
				}
			}
		}

		if (includeRelativeSubPath && alreadyIncludedSubPath == false && subPaths.empty() == false)
		{
			wcscpy_s(data.cFileName, MAX_PATH, std::wstring(subPaths.top()).append(data.cFileName).c_str());
		}
	}

	bool FindFiles::IsValid()
	{
		return valid;
	}

}}
