// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#pragma comment(lib, "dnsapi.lib")
#include <windns.h>
#include <vector>
#include "Dns.hpp"

namespace Instalog { namespace SystemFacades {

	void HostnameFromIpAddress( std::wstring ipAddress, bool useSafeDnsAddresses /*= false*/ )
	{
		DNS_STATUS status;
		PDNS_RECORD pDnsRecord;
		PIP4_ARRAY pSrvList = NULL;
		WORD wType = DNS_TYPE_A;
		const wchar_t* pOwnerName = ipAddress.c_str();
		//wchar_t pReversedIP[255];
		//wchar_t DnsServIp[255];
		IN_ADDR ipaddr;

		if (useSafeDnsAddresses)
		{
			static const char *safeDnsServerAddresses[] = {
				"8.8.8.8",			// Google Public DNS Primary
				"8.8.4.4",			// Google Public DNS Secondary
				"208.67.222.222",	// OpenDNS Primary
				"208.67.220.220",	// OpenDNS Secondary
			};
			static const size_t numDnsServerAddresses = (sizeof(safeDnsServerAddresses) / sizeof(const char *));

			std::vector<char> srvList(sizeof(IP4_ARRAY) - sizeof(DWORD) + numDnsServerAddresses * sizeof(DWORD));
			reinterpret_cast<PIP4_ARRAY>(srvList.data())->AddrCount = numDnsServerAddresses;
			for (size_t i = 0; i < numDnsServerAddresses; ++i) 
			{
				DWORD addr = inet_addr(safeDnsServerAddresses[i]);
				if (addr == INADDR_NONE)
				{
					throw std::runtime_error("Invalid DNS server IP in array of safe addresses");
				}
				reinterpret_cast<PIP4_ARRAY>(srvList.data())->AddrArray[i] = addr;
			}
			pSrvList = reinterpret_cast<PIP4_ARRAY>(srvList.data());
		}
		
		status = DnsQuery(pOwnerName,                 //Pointer to OwnerName. 
						  wType,                      //Type of the record to be queried.
						  DNS_QUERY_BYPASS_CACHE,     //Bypasses the resolver cache on the lookup. 
						  pSrvList,                   //Contains DNS server IP address.
						  &pDnsRecord,                //Resource record that contains the response.
						  NULL);                      //Reserved for future use.

		if (status)
		{
			std::wcout << "Error\n";
		}
		else
		{
			ipaddr.S_un.S_addr = (pDnsRecord->Data.A.IpAddress);
			std::wcout << inet_ntoa(ipaddr) << std::endl;

			// Free memory allocated for DNS records. 
			DnsRecordListFree(pDnsRecord, DnsFreeRecordListDeep);
		}
	}

}}