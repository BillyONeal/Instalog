// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include "LogCommon/Dns.hpp"

using namespace Instalog::SystemFacades;

TEST(DnsAddressFromHostname, DefaultServers)
{
    ASSERT_FALSE(IpAddressFromHostname(L"google.com").empty());
}

TEST(DnsAddressFromHostname, SafeServers)
{
    ASSERT_FALSE(IpAddressFromHostname(L"google.com", true).empty());
}

/* This needs reworked; see issue 13
TEST_METHOD(GetIpAddressFromHostnameSafeServers)
{
    Assert::IsFalse(IpAddressFromHostname(L"google.com", true).empty());
}
*/

TEST(DnsIpReverse, EmptyIpReverse)
{
    ASSERT_EQ(L"", ReverseIpAddress(L""));
}

TEST(DnsIpReverse, FullIpReverse)
{
    ASSERT_EQ(L"901.678.345.012", ReverseIpAddress(L"012.345.678.901"));
}

TEST(DnsIpReverse, ShortIpReverse)
{
    ASSERT_EQ(L"4.3.2.1", ReverseIpAddress(L"1.2.3.4"));
}

TEST(DnsIpReverse, MixedIpReverse)
{
    ASSERT_EQ(L"789.56.234.1", ReverseIpAddress(L"1.234.56.789"));
}

TEST(DnsIpReverse, RealIpReverse)
{
    ASSERT_EQ(L"138.204.14.72", ReverseIpAddress(L"72.14.204.138"));
}

TEST(DnsHostnameFromAddress, DefaultServers)
{
    ASSERT_TRUE(boost::ends_with(HostnameFromIpAddress(IpAddressFromHostname(L"google.com")), L".1e100.net"));
}

/* This needs reworked; see issue 13
TEST_METHOD(DnsHostnameFromAddressSafeServers)
{
    std::wcout << IpAddressFromHostname(L"google.com", true) << std::endl;
    std::wcout << HostnameFromIpAddress(IpAddressFromHostname(L"google.com", true), true) << std::endl;
    Assert::IsTrue(boost::ends_with(HostnameFromIpAddress(IpAddressFromHostname(L"google.com", true), true), L".1e100.net"));
}
*/
