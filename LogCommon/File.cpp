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
			WindowsApiException::ThrowFromLastError();
		}
	}

	File::~File()
	{
		::CloseHandle(hFile);
	}

	// TODO: Throw exception :D
	void File::Delete( std::wstring const& filename)
	{
		::DeleteFileW(filename.c_str());
	}

	bool File::Exists( std::wstring const& filename)
	{
		DWORD attributes = ::GetFileAttributesW(filename.c_str());

		return attributes != INVALID_FILE_ATTRIBUTES;
	}

}}
