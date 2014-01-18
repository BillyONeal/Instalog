// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include "../LogCommon/Dns.hpp"

using namespace Instalog::SystemFacades;

TEST(DnsAddressFromHostname, DefaultServers)
{
    ASSERT_FALSE(IpAddressFromHostname("google.com").empty());
}

TEST(DnsAddressFromHostname, SafeServers)
{
    ASSERT_FALSE(IpAddressFromHostname("google.com", true).empty());
}

/* This needs reworked; see issue 13
TEST_METHOD(GetIpAddressFromHostnameSafeServers)
{
    Assert::IsFalse(IpAddressFromHostname("google.com", true).empty());
}
*/

TEST(DnsIpReverse, EmptyIpReverse)
{
    ASSERT_EQ("", ReverseIpAddress(""));
}

TEST(DnsIpReverse, FullIpReverse)
{
    ASSERT_EQ("901.678.345.012", ReverseIpAddress("012.345.678.901"));
}

TEST(DnsIpReverse, ShortIpReverse)
{
    ASSERT_EQ("4.3.2.1", ReverseIpAddress("1.2.3.4"));
}

TEST(DnsIpReverse, MixedIpReverse)
{
    ASSERT_EQ("789.56.234.1", ReverseIpAddress("1.234.56.789"));
}

TEST(DnsIpReverse, RealIpReverse)
{
    ASSERT_EQ("138.204.14.72", ReverseIpAddress("72.14.204.138"));
}

TEST(DnsHostnameFromAddress, DefaultServers)
{
    auto const ip = IpAddressFromHostname("google.com");
    auto const host = HostnameFromIpAddress(ip);
    ASSERT_TRUE(boost::ends_with(host, ".1e100.net")) << "IP was " << ip << " and host is " << host;
}

/* This needs reworked; see issue 13
TEST_METHOD(DnsHostnameFromAddressSafeServers)
{
    std::wcout << IpAddressFromHostname("google.com", true) << std::endl;
    std::wcout << HostnameFromIpAddress(IpAddressFromHostname("google.com",
true), true) << std::endl;
    Assert::IsTrue(boost::ends_with(HostnameFromIpAddress(IpAddressFromHostname("google.com",
true), true), ".1e100.net"));
}
*/
