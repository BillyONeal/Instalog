// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include "../LogCommon/Dns.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Instalog::SystemFacades;

TEST_CLASS(DnsTest)
{
public:
	TEST_METHOD(GetIpAddressFromHostnameDefaultServers)
	{
		Assert::IsFalse(IpAddressFromHostname(L"google.com").empty());
	}

	TEST_METHOD(GetIpAddressFromHostnameSafeServers)
	{
		Assert::IsFalse(IpAddressFromHostname(L"google.com", true).empty());
	}

	TEST_METHOD(DnsIpServersEmptyIpReverse)
	{
		Assert::AreEqual<std::wstring>(L"", ReverseIpAddress(L""));
	}

	TEST_METHOD(DnsIpServersFullIpReverse)
	{
		Assert::AreEqual<std::wstring>(L"901.678.345.012", ReverseIpAddress(L"012.345.678.901"));
	}

	TEST_METHOD(DnsIpReverseShortIpReverse)
	{
		Assert::AreEqual<std::wstring>(L"4.3.2.1", ReverseIpAddress(L"1.2.3.4"));
	}

	TEST_METHOD(DnsIpReverseMixedIpReverse)
	{
		Assert::AreEqual<std::wstring>(L"789.56.234.1", ReverseIpAddress(L"1.234.56.789"));
	}

	TEST_METHOD(DnsIpReverseRealIpReverse)
	{
		Assert::AreEqual<std::wstring>(L"138.204.14.72", ReverseIpAddress(L"72.14.204.138"));
	}

	TEST_METHOD(DnsHostnameFromAddressDefaultServers)
	{
		Assert::IsTrue(boost::ends_with(HostnameFromIpAddress(IpAddressFromHostname(L"google.com")), L".1e100.net"));
	}

	TEST_METHOD(DnsHostnameFromAddressSafeServers)
	{
		std::wcout << IpAddressFromHostname(L"google.com", true) << std::endl;
		std::wcout << HostnameFromIpAddress(IpAddressFromHostname(L"google.com", true), true) << std::endl;
		Assert::IsTrue(boost::ends_with(HostnameFromIpAddress(IpAddressFromHostname(L"google.com", true), true), L".1e100.net"));
	}
};
