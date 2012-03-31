// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <windows.h>

namespace Instalog {

	/// @brief	Converts Windows FILETIME structure to an int
	///
	/// @param	ft	The filetime.
	///
	/// @return	unsigned __int64 representation of ft
	inline unsigned __int64 FiletimeToInteger(FILETIME const& ft)
	{
		unsigned __int64 result = ft.dwHighDateTime;
		result <<= 32;
		result |= ft.dwLowDateTime;
		return result;
	}

}