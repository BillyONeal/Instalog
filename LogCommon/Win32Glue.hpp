#pragma once
#include <windows.h>

inline unsigned __int64 FiletimeToInteger(FILETIME const& ft)
{
	unsigned __int64 result = ft.dwHighDateTime;
	result <<= 32;
	result |= ft.dwLowDateTime;
	return result;
}
