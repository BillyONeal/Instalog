// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once

#include <string>

namespace Instalog { namespace SystemFacades {

	/// @brief	Queries DNS for the hostname for a given IP address.
	///
	/// @param	ipAddress		   	The ip address in IPv4 form (xxx.xxx.xxx.xxx)
	/// @param	useSafeDnsAddresses	(optional) If false, this will use the system's default DNS
	/// 							servers.  If true, this will manually use known "safe" DNS 
	/// 							servers (Google's and if that fails, then OpenDNS)
	///
	/// @return	The string representation of the hostname, empty string on error
	std::wstring HostnameFromIpAddress(std::wstring ipAddress, bool useSafeDnsAddresses = false);

}}