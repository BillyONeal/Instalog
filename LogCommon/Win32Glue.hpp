// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <windows.h>
#include "Win32Exception.hpp"

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

	/// @brief	Seconds since 1970 from FILETIME struct
	///
	/// @param	time	The time as a FILETIME
	///
	/// @return	Seconds since 1970
	DWORD SecondsSince1970(FILETIME const& time);

	/// @brief	Seconds since 1970 from SYSTEMTIME struct
	///
	/// @param	time	The time as a SYSTEMTIME
	///
	/// @return	Seconds since 1970
	DWORD SecondsSince1970(SYSTEMTIME const& time);

	/// @brief	Creates a FILETIME struct from seconds since 1970.
	///
	/// @param	seconds	The seconds since 1970.
	///
	/// @return	The time as a FILETIME struct
	FILETIME FiletimeFromSecondsSince1970(DWORD seconds);

	/// @brief	Creates a SYSTEMTIME struct from seconds since 1970.
	///
	/// @param	seconds	The seconds since 1970.
	///
	/// @return	The time as a SYSTEMTIME struct
	SYSTEMTIME SystemtimeFromSecondsSince1970(DWORD seconds);

}