// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include <string>
#include <vector>
#include "../LogCommon/File.hpp"
#include "../LogCommon/Library.hpp"
#include "../LogCommon/Win32Exception.hpp"

using namespace Instalog::SystemFacades;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_CLASS(RuntimeDynamicLinkerTests)
{
public:
    TEST_METHOD(NonexistentDllThrows)
    {
        Assert::ExpectException<ErrorModuleNotFoundException>([] { RuntimeDynamicLinker ntdll(L"IDoNotExist.dll"); });
    }

    TEST_METHOD(CanLoadFunction)
    {
        typedef NTSTATUS (NTAPI *NtCloseT)(HANDLE);
        RuntimeDynamicLinker ntdll(L"ntdll.dll");
        NtCloseT ntClose = ntdll.GetProcAddress<NtCloseT>("NtClose");
        HANDLE h = ::CreateFileW(L"DeleteMe.txt", GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, 0);
        ntClose(h);
        Assert::IsFalse(File::Exists(L"DeleteMe.txt"));
    }
};

TEST_CLASS(FormattedMessageLoaderTests)
{
public:
    TEST_METHOD(NonexistentDllThrows)
    {
        Assert::ExpectException<ErrorFileNotFoundException>([] { FormattedMessageLoader ntdll(L"IDoNotExist.dll"); });
    }

    TEST_METHOD(DhcpClientArguments)
    {
        DWORD messageId = 50037;
        std::vector<std::wstring> arguments;
        arguments.push_back(L"1");

        FormattedMessageLoader dhcpcore(L"C:\\Windows\\System32\\dhcpcore.dll");
        std::wstring message = dhcpcore.GetFormattedMessage(messageId, arguments);
        Assert::AreEqual<std::wstring>(L"DHCPv4 client service is stopped. ShutDown Flag value is 1\r\n", message);
    }

    TEST_METHOD(DhcpClientNoArguments)
    {
        DWORD messageId = 50036;
        std::vector<std::wstring> arguments;

        FormattedMessageLoader dhcpcore(L"C:\\Windows\\System32\\dhcpcore.dll");
        std::wstring message = dhcpcore.GetFormattedMessage(messageId, arguments);
        Assert::AreEqual<std::wstring>(L"DHCPv4 client service is started\r\n", message);

        message = dhcpcore.GetFormattedMessage(messageId);
        Assert::AreEqual<std::wstring>(L"DHCPv4 client service is started\r\n", message);
    }
};
