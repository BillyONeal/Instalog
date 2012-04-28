// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <ostream>
#include <string>
#include <vector>
#include <windows.h>
#include "Win32Glue.hpp"

namespace Instalog {

	/// @brief	Writes the default date format
	///
	/// @param [out]	str	The string stream to write the date to
	/// @param	time	   	The time.  Note that this is a FILETIME structure (though it expects a plain __int64 to avoid including windows.h).  Use FiletimeToInteger() to convert to a __int64.
	void WriteDefaultDateFormat(std::wostream &str, unsigned __int64 time);

	/// @brief	Writes the default date format including milliseconds
	///
	/// @param [out]	str	The string stream to write the date to
	/// @param	time	   	The time.  Note that this is a FILETIME structure (though it expects a plain __int64 to avoid including windows.h).  Use FiletimeToInteger() to convert to a __int64.
	void WriteMillisecondDateFormat(std::wostream &str, unsigned __int64 time);

	/// @brief	Writes the current date format including milliseconds
	///
	/// @param [out]	str	The string stream to write the date to
	void WriteCurrentMillisecondDate(std::wostream &str);

	/// @brief	Writes the file attributes
	///
	/// @param [out]	str	The string stream to write the attributes to
	/// @param	attributes	   	The attributes.  Note that this is a DWORD (though it expects a plain __int32 to avoid including windows.h)
	void WriteFileAttributes(std::wostream &str, unsigned __int32 attributes);

	/// @brief	Writes the default representation of a file 
	///
	/// @param [out]	str			The string stream to write the attributes to
	/// @param	targetFile			Target file.
	void WriteDefaultFileOutput(std::wostream &str, std::wstring targetFile);
	void WriteFileListingFile(std::wostream &str, std::wstring const& targetFile);
	void WriteFileListingFromFindData( std::wostream &str, WIN32_FIND_DATAW const& info );

	/// @brief	Writes the script header.
	///
	/// @param [out]	log	The log stream
	void WriteScriptHeader(std::wostream &log);

	/// @brief	Writes the script footer.
	///
	/// @param [out]	log	The log stream.
	void WriteScriptFooter(std::wostream &log);

	/// @brief	Writes the memory information.
	///
	/// @param [out]	str	The log stream
	void WriteMemoryInformation(std::wostream &str);

	/// @brief	Writes the operating system version.
	///
	/// @param [out]	log	The log stream.
	void WriteOsVersion(std::wostream &log);

}
