// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once

#include <string>

namespace Instalog
{
namespace SystemFacades
{

/// @brief    Queries DNS for the IP address for a given hostname.
///
/// @param    hostname               The hostname
/// @param    useSafeDnsAddresses    (optional) If false, this will use the
/// system's default DNS servers.  If true, this will manually use known
/// "safe" DNS servers (Google's and if that fails, then OpenDNS)
///
/// @return    The IPv4 address in the form xxx.xxx.xxx.xxx or empty string if
/// an error occured
std::string IpAddressFromHostname(std::string const& hostname,
                                  bool useSafeDnsAddresses = false);

/// @brief    Queries DNS for the hostnape for a given IP address
///
/// @param    ipAddress               The IP address.
/// @param    useSafeDnsAddresses    (optional) If false, this will use the
/// system's default DNS servers.  If true, this will manually use known
/// "safe" DNS servers (Google's and if that fails, then OpenDNS)
///
/// @return    The hostname or empty string if an error occured
std::string HostnameFromIpAddress(std::string const& ipAddress,
                                  bool useSafeDnsAddresses = false);

/// @brief    Reverses an IP address.  Doesn't do any validation, just reverses
/// around periods.
///
/// @param    ipAddress    The address to reverse
///
/// @return    The reversed IP address
std::string ReverseIpAddress(std::string const& ipAddress);
}
}