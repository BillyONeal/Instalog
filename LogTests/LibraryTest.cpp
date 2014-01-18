// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include <string>
#include <vector>
#include "../LogCommon/File.hpp"
#include "../LogCommon/Library.hpp"
#include "../LogCommon/Win32Exception.hpp"

using namespace Instalog::SystemFacades;

TEST(RuntimeDynamicLinker, NonexistentDllThrows)
{
    ASSERT_THROW(RuntimeDynamicLinker ntdll("IDoNotExist.dll"),
                 ErrorModuleNotFoundException);
}

TEST(RuntimeDynamicLinker, CanLoadFunction)
{
    typedef NTSTATUS(NTAPI * NtCloseT)(HANDLE);
    RuntimeDynamicLinker ntdll("ntdll.dll");
    NtCloseT ntClose = ntdll.GetProcAddress<NtCloseT>("NtClose");
    HANDLE h = ::CreateFileW(L"DeleteMe.txt",
                             GENERIC_WRITE,
                             0,
                             0,
                             CREATE_NEW,
                             FILE_FLAG_DELETE_ON_CLOSE,
                             0);
    ntClose(h);
    ASSERT_FALSE(File::Exists("DeleteMe.txt"));
}

TEST(FormattedMessageLoader, NonexistentDllThrows)
{
    ASSERT_THROW(FormattedMessageLoader ntdll("IDoNotExist.dll"),
                 ErrorFileNotFoundException);
}

TEST(FormattedMessageLoader, DhcpClientArguments)
{
    DWORD messageId = 50037;
    std::vector<std::wstring> arguments;
    arguments.emplace_back(L"1");

    FormattedMessageLoader dhcpcore("C:\\Windows\\System32\\dhcpcore.dll");
    std::string message = dhcpcore.GetFormattedMessage(messageId, arguments);
    ASSERT_EQ("DHCPv4 client service is stopped. ShutDown Flag value is 1\r\n",
              message);
}

TEST(FormattedMessageLoader, DhcpClientNoArguments)
{
    DWORD messageId = 50036;
    FormattedMessageLoader dhcpcore("C:\\Windows\\System32\\dhcpcore.dll");
    std::string message = dhcpcore.GetFormattedMessage(messageId);
    EXPECT_EQ("DHCPv4 client service is started\r\n", message);

    message = dhcpcore.GetFormattedMessage(messageId);
    EXPECT_EQ("DHCPv4 client service is started\r\n", message);
}