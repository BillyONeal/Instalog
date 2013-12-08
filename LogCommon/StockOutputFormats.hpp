// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>
#include <windows.h>
#include "File.hpp"
#include "Win32Glue.hpp"
#include "LogSink.hpp"

namespace Instalog
{

/// @brief    Writes the default date format
///
/// @param [out]    str    The string stream to write the date to
/// @param    time           The time.  Note that this is a FILETIME structure
/// (though it expects a plain std::int64_t to avoid including windows.h).  Use
/// FiletimeToInteger() to convert to a std::int64_t.
void WriteDefaultDateFormat(log_sink& str, std::uint64_t time);

/// @brief    Writes the default date format including milliseconds
///
/// @param [out]    str    The string stream to write the date to
/// @param    time           The time.  Note that this is a FILETIME structure
/// (though it expects a plain std::int64_t to avoid including windows.h).  Use
/// FiletimeToInteger() to convert to a std::int64_t.
void WriteMillisecondDateFormat(log_sink& str, std::uint64_t time);

/// @brief    Writes the file attributes
///
/// @param [out]    str    The string stream to write the attributes to
/// @param    attributes           The attributes.  Note that this is a DWORD
/// (though it expects a plain std::int32_t to avoid including windows.h)
void WriteFileAttributes(log_sink& str, std::uint32_t attributes);

/// @brief    Writes the default representation of a file
///
/// @param [out]    str            The string stream to write the attributes to
/// @param    targetFile            Target file.
void WriteDefaultFileOutput(log_sink& str, std::string targetFile);
void WriteFileListingFile(log_sink& str, std::string const& targetFile);
void WriteFileListingFromFindData(log_sink& str,
                                  SystemFacades::FindFilesRecord const& info);

LONG GetTimeZoneBias();

/// @brief    Writes the script header.
///
/// @param [out]    log    The log stream
/// @param [in]     startTime The starting time of report generation.
void WriteScriptHeader(log_sink& log, std::uint64_t startTime);

/// @brief    Writes the script footer.
///
/// @param [out]    log    The log stream.
/// @param [in]     startTime The time that report generation started.
void WriteScriptFooter(log_sink& log, std::uint64_t startTime);

/// @brief    Writes the memory information.
///
/// @param [out]    str    The log stream
void WriteMemoryInformation(log_sink& str);

/// @brief    Writes the operating system version.
///
/// @param [out]    log    The log stream.
void WriteOsVersion(log_sink& log);

/// @brief  Gets the current local time as a FILETIME cast to an std::uint64_t.
/// @return The current time.
std::uint64_t GetLocalTime();
}
