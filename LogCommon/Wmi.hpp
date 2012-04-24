// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once

#include <atlbase.h>
# pragma comment(lib, "wbemuuid.lib")
#include <wbemidl.h>
#include <string>

namespace Instalog { namespace SystemFacades {

	/// @brief	Gets a CComPtr to do WMI queries with
	///
	/// @return	The wbem services.
	CComPtr<IWbemServices> GetWbemServices();

	/// @brief	Converts a WMI date string to a FILETIME struct as UTC time
	///
	/// @param	datestring	The WMI date string.
	///
	/// @return	The date / time as a FILETIME struct in UTC time
	FILETIME WmiDateStringToFiletime(std::wstring const& datestring);

}}