// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <string>

namespace Instalog { namespace Path {

	/// @brief	Appends two paths together considering the middle backslash
	/// 		
	/// @details	This isn't a very advanced method.  It only considers the last character of path and the first character of more. 
	///
	/// @param	[in,out] path	Left of the path.  This variable will be modified to equal the final result.
	/// @param	[in] more	The right of the path to be appended
	std::wstring Append(std::wstring path, std::wstring const& more);

	/// @brief	"Prettifies" paths
	/// 
	/// @details	Lowercases everything in a path besides the drive letter and characters immediately following backslashes
	///
	/// @param	first	An iterator to the beginning of the path to be pretified
	/// @param	last 	An iterator one past the end of the path to be pretified
	void Prettify(std::wstring::iterator first, std::wstring::iterator last);

	/// @brief	Resolve a path from the command line 
	/// 
	/// @details This is intended to work in the same way as Windows does it
	///
	/// @param	[in,out]	path	Full pathname of the file.
	///
	/// @return	true if the path exists and is not a directory, false otherwise
	bool ResolveFromCommandLine(std::wstring &path);

	/// @brief	Gets the Windows path.
	/// 
	/// @details	Does not include the trailing backslash (e.g. "C:\Windows") unless Windows is installed in the drive root
	///
	/// @return	The Windows path.
	std::wstring GetWindowsPath();

    /**
     * Expands environment strings.
     *
     * @param input The input string to expand environment variables inside.
     *
     * @return The string with environment strings expanded.
     */
    std::wstring ExpandEnvStrings(std::wstring const& input);
}}
