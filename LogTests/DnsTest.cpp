// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include "LogCommon/Dns.hpp"

using namespace Instalog::SystemFacades;

TEST(Dns, DnsTest)
{
	HostnameFromIpAddress(L"google.com");
	HostnameFromIpAddress(L"google.com", true);
}
