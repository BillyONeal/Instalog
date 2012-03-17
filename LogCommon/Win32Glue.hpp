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