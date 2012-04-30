// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#pragma comment(lib, "dnsapi.lib")
#include <windns.h>
#include <vector>
#include "StringUtilities.hpp"
#include "Dns.hpp"

namespace Instalog { namespace SystemFacades {

	std::wstring HostnameFromIpAddress( std::wstring ipAddress, bool useSafeDnsAddresses /*= false*/ )
	{
		DNS_STATUS status;
		PDNS_RECORD pDnsRecord;
		//wchar_t pReversedIP[255];
		//wchar_t DnsServIp[255];

		if (useSafeDnsAddresses)
		{
			static const char *safeDnsServerAddresses[] = {
				"8.8.8.8",			// Google Public DNS Primary
				"8.8.4.4",			// Google Public DNS Secondary
				"208.67.222.222",	// OpenDNS Primary
				"208.67.220.220",	// OpenDNS Secondary
			};
			static const size_t numDnsServerAddresses = (sizeof(safeDnsServerAddresses) / sizeof(const char *));

			std::vector<char> serversList(sizeof(IP4_ARRAY) + (numDnsServerAddresses - 1) * sizeof(IP4_ADDRESS));
			reinterpret_cast<PIP4_ARRAY>(serversList.data())->AddrCount = numDnsServerAddresses;
			for (size_t i = 0; i < numDnsServerAddresses; ++i) 
			{
				DWORD addr = inet_addr(safeDnsServerAddresses[i]);
				if (addr == INADDR_NONE)
				{
					throw std::runtime_error("Invalid DNS server IP in array of safe addresses");
				}
				reinterpret_cast<PIP4_ARRAY>(serversList.data())->AddrArray[i] = addr;
			}

			status = DnsQuery(ipAddress.c_str(), DNS_TYPE_A, DNS_QUERY_BYPASS_CACHE, 
				reinterpret_cast<PIP4_ARRAY>(serversList.data())->AddrArray, &pDnsRecord, NULL);
		}
		else
		{
			status = DnsQuery(ipAddress.c_str(), DNS_TYPE_A, DNS_QUERY_BYPASS_CACHE, 
				NULL, &pDnsRecord, NULL);
		}

		if (status)
		{
			return L"";
		}
		else
		{
			IN_ADDR ipaddr;
			ipaddr.S_un.S_addr = (pDnsRecord->Data.A.IpAddress);
			DnsRecordListFree(pDnsRecord, DnsFreeRecordListDeep);

			char* hostnameNarrow = inet_ntoa(ipaddr);
			if (hostnameNarrow == NULL)
			{
				return L"";
			}
			else
			{
				std::wcout << hostnameNarrow << std::endl;
				return ConvertUnicode(hostnameNarrow);
			}
		}
	}

}}