// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <string>
#include <memory>
#include <iterator>
#include <algorithm>
#include <type_traits>
#include <boost/config.hpp>

namespace Instalog
{
namespace Path
{

/// @brief    Appends two paths together considering the middle backslash
///
/// @details    This isn't a very advanced method.  It only considers the last
/// character of path and the first character of more.
///
/// @param    [in,out] path    Left of the path.  This variable will be modified
/// to equal the final result.
/// @param    [in] more    The right of the path to be appended
std::string Append(std::string path, std::string const& more);

/// @brief    "Prettifies" paths
///
/// @details    Lowercases everything in a path besides the drive letter and
/// characters immediately following backslashes
///
/// @param    first    An iterator to the beginning of the path to be pretified
/// @param    last     An iterator one past the end of the path to be pretified
void Prettify(std::string::iterator first, std::string::iterator last);

/// @brief    Expands a short windows path to the corresponding long version
///
/// @param    path    Full long path to the file
bool ExpandShortPath(std::string& path);

/// @brief    Resolve a path from the command line
///
/// @details This is intended to work in the same way as Windows does it
///
/// @param    [in,out]    path    Full pathname of the file.
///
/// @return    true if the path exists and is not a directory, false otherwise
bool ResolveFromCommandLine(std::string& path);

/// @brief    Gets the Windows path.
///
/// @details    Does not include the trailing backslash (e.g. "C:\Windows")
/// unless Windows is installed in the drive root
///
/// @return    The Windows path.
std::string GetWindowsPath();

/**
 * Expands environment strings.
 *
 * @param input The input string to expand environment variables inside.
 *
 * @return The string with environment strings expanded.
 */
std::string ExpandEnvStrings(std::string const& input);

}
}
