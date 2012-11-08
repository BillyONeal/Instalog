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

    /// @brief    Gets the "safe" DNS servers
    ///
    /// @exception    std::runtime_error    Thrown when the addresses array contains an invalid server 
    ///                                 IP (shouldn't happen ever)
    ///
    /// @return    The safe servers list.
    static std::vector<char> GetSafeServersList()
    {
        static const char *safeDnsServerAddresses[] = {
            "8.8.8.8",            // Google Public DNS Primary
            "8.8.4.4",            // Google Public DNS Secondary
            "208.67.222.222",    // OpenDNS Primary
            "208.67.220.220",    // OpenDNS Secondary
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

        return serversList;
    }

    std::wstring IpAddressFromHostname( std::wstring const& hostname, bool useSafeDnsAddresses /*= false*/ )
    {
        DNS_STATUS status;
        PDNS_RECORD pDnsRecord;

        if (useSafeDnsAddresses)
        {
             std::vector<char> serversList = GetSafeServersList();

            status = DnsQuery(hostname.c_str(), DNS_TYPE_A, DNS_QUERY_BYPASS_CACHE, 
                reinterpret_cast<PIP4_ARRAY>(serversList.data()), &pDnsRecord, NULL);
        }
        else
        {
            status = DnsQuery(hostname.c_str(), DNS_TYPE_A, DNS_QUERY_BYPASS_CACHE, 
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
                return ConvertUnicode(hostnameNarrow);
            }
        }
    }

    std::wstring HostnameFromIpAddress( std::wstring ipAddress, bool useSafeDnsAddresses /*= false*/ )
    {
        std::wstring reversedIpAddress(ReverseIpAddress(ipAddress));
        reversedIpAddress.append(L".IN-ADDR.ARPA");

        DNS_STATUS status;
        PDNS_RECORD pDnsRecord;

        if (useSafeDnsAddresses)
        {
            std::vector<char> serversList = GetSafeServersList();

            status = DnsQuery(reversedIpAddress.c_str(), DNS_TYPE_PTR, DNS_QUERY_BYPASS_CACHE, 
                reinterpret_cast<PIP4_ARRAY>(serversList.data()), &pDnsRecord, NULL);
        }
        else
        {
            status = DnsQuery(reversedIpAddress.c_str(), DNS_TYPE_PTR, DNS_QUERY_BYPASS_CACHE, 
                NULL, &pDnsRecord, NULL);
        }

        if (status)
        {
            return L"";
        }
        else
        {
            return pDnsRecord->Data.PTR.pNameHost;
        }
    }

    std::wstring ReverseIpAddress( std::wstring ipAddress )
    {
        if (ipAddress.size() == 0)
        {
            return L"";
        }

        std::wstring reversed;
        reversed.reserve(ipAddress.size());

        auto regionStart = ipAddress.end() - 1;
        auto regionEnd = ipAddress.end();
        for (; regionStart != ipAddress.begin(); --regionStart)
        {
            if (*regionStart == L'.')
            {
                reversed.append(regionStart + 1, regionEnd);
                reversed.append(L".");
                regionEnd = regionStart;
            }
        }
        reversed.append(regionStart, regionEnd);

        return reversed;
    }

}}