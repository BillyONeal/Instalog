#pragma once
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <windows.h>

namespace Instalog { namespace SystemFacades {

	/// @brief Represents a plain Win32 file handle.
	class File : boost::noncopyable
	{
		HANDLE hFile;
	public:
		File();
		///
		/// @param	filename		  	Filename of the file.
		/// @param	desiredAccess	  	(optional) the desired access.
		/// @param	shareMode		  	(optional) the share mode.
		/// @param	securityAttributes	(optional) the security attributes.
		/// @param	createdDisposition	(optional) the created disposition.
		/// @param	flags			  	(optional) the flags.
		///
		/// @throw	Win32Exception on failure
		File(
			std::wstring const& filename,
			DWORD desiredAccess = GENERIC_READ,
			DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE,
			LPSECURITY_ATTRIBUTES securityAttributes = nullptr,
			DWORD createdDisposition = OPEN_EXISTING,
			DWORD flags = FILE_ATTRIBUTE_NORMAL
		);
		File(File && other);
		File& operator=(File other);
		~File();

		/// @brief	Gets the size of the file in bytes
		///
		/// @return	The size.
		///
		/// @throw	Win32Exception on failure
		unsigned __int64 GetSize() const;

		/// @brief	Gets the attributes of the file as a DWORD
		///
		/// @return	The attributes.
		///
		/// @throw	Win32Exception on error
		DWORD GetAttributes() const;

		/// @brief	Gets the extended attributes of the file
		///
		/// @return	The extended attributes.
		///
		/// @throw	Win32Exception on error
		BY_HANDLE_FILE_INFORMATION GetExtendedAttributes() const;

		/// @brief	Reads the the specified amount of bytes into a vector 
		///
		/// @param	bytesToRead	The number of bytes to read.
		///
		/// @return	The bytes as a vector of chars.
		/// 
		/// @throw	Win32Exception on error
		std::vector<char> ReadBytes(unsigned int bytesToRead) const;

		/// @brief	Writes the specified bytes to the file
		///
		/// @param	bytes	The bytes.
		///
		/// @return	true if all of the bytes were written
		/// 
		/// @throw	Win32Exception on error
		bool WriteBytes(std::vector<char> const& bytes); 

		/// @brief	Gets the size of the specified file in bytes without opening a file handle
		///
		/// @return	The size.
		///
		/// @throw	Win32Exception on error
		static unsigned __int64 GetSize(std::wstring const& filename);

		/// @brief	Gets the attributes of the specified file as a DWORD without opening a file handle
		///
		/// @return	The attributes.
		///
		/// @throw	Win32Exception on error
		static DWORD GetAttributes(std::wstring const& filename);

		/// @brief	Gets the extended attributes of the specified file without opening a file handle
		///
		/// @return	The extended attributes.
		///
		/// @throw	Win32Exception on error
		static WIN32_FILE_ATTRIBUTE_DATA GetExtendedAttributes(std::wstring const& filename);

		/// @brief	Deletes the given file.
		///
		/// @param	filename	Filename of the file.
		/// 
		/// @throw	Win32Exception on error
		static void Delete(std::wstring const& filename);

		/// @brief	Determines if the given file exists
		///
		/// @param	filename	Filename of the file.
		///
		/// @return	true if the file exists
		static bool Exists(std::wstring const& filename);

		/// @brief	Query if a path is a directory.
		///
		/// @param	filename	Path of the file.
		///
		/// @return	true if the path is directory, false if not.
		static bool IsDirectory(std::wstring const& filename);

		/// @brief  Query if a path is a file. (That is, exists, and is not a directory)
		///
		/// @param  filename    Path of the file to check.
		///
		/// @remarks Equivalent to Exists() && !IsDirectory()
		///
		/// @return true if the path exists and is a file, false otherwise.
		static bool IsExclusiveFile(std::wstring const& fileName);

		/// @brief	Query if filename is an executable.
		///
		/// @param	filename	Filename of the file.
		///
		/// @return	true if it is an executable, false if not.
		static bool IsExecutable(std::wstring const& filename);

		/// @brief	Gets the company of the file
		///
		/// @param	target	Target file
		///
		/// @return	The company.
		/// 
		/// @throw	Win32Exception on error
		static std::wstring GetCompany(std::wstring const& target);
	};

}}
