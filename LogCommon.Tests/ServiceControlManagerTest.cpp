// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include "../LogCommon/ServiceControlManager.hpp"

using Instalog::SystemFacades::ServiceControlManager;
using Instalog::SystemFacades::Service;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_CLASS(ServiceControlManagerTests)
{
    TEST_METHOD(CreateServiceControlManager)
    {
        ServiceControlManager scm; //Expect no throw
    }

    static std::vector<Service> const& GetCachedServices()
    {
        static ServiceControlManager scm;
        static std::vector<Service> services(scm.GetServices());
        return services;
    }

    TEST_METHOD(GetServices)
    {
        std::vector<Service> const& services = GetCachedServices();
        Assert::IsFalse(services.empty());
    }

    TEST_METHOD(FieldsSetCorrectly)
    {
        std::vector<Service> const& services = GetCachedServices();
    
        for (auto it = services.begin(); it != services.end(); ++it)
        {
            Assert::IsFalse(it->GetServiceName().empty());
            Assert::IsFalse(it->GetDisplayName().empty());
            Assert::IsFalse(it->GetState().empty());
            Assert::IsTrue(it->GetServiceName().empty() == it->GetDisplayName().empty());
            if (it->GetSvchostGroup().empty())
            {
                Assert::IsFalse(it->IsSvchostService());
            }
            else
            {
                Assert::IsTrue(it->IsSvchostService());
            }
        }
    }

    TEST_METHOD(TcpipService)
    {
        std::vector<Service> const& services = GetCachedServices();

        auto tcpip = std::find_if(services.begin(), services.end(), [] (Service const& service) -> bool { 
            return service.GetServiceName() == L"Tcpip";
        });

        Assert::IsFalse(tcpip == services.end());
        Assert::AreEqual<std::wstring>(L"TCP/IP Protocol Driver", tcpip->GetDisplayName());
        Assert::AreEqual<std::wstring>(L"R", tcpip->GetState()); // This will fail if Tcpip is not running
        Assert::IsTrue(SERVICE_BOOT_START == tcpip->GetStart()); // This will fail if Tcpip is not configured to auto-start
        Assert::AreEqual<std::wstring>(L"C:\\Windows\\System32\\Drivers\\Tcpip.sys", tcpip->GetFilepath());
        Assert::IsTrue(tcpip->GetSvchostGroup().empty());
        Assert::IsTrue(tcpip->GetSvchostDll().empty());
        Assert::IsFalse(tcpip->IsSvchostService());
    }

    TEST_METHOD(RpcSsSvchostService)
    {
        std::vector<Service> const& services = GetCachedServices();

        auto rcpss = std::find_if(services.begin(), services.end(), [] (Service const& service) -> bool { 
            return boost::algorithm::iequals(service.GetServiceName(), L"RpcSs");
        });

        Assert::IsFalse(rcpss == services.end());
        Assert::AreEqual<std::wstring>(L"Remote Procedure Call (RPC)", rcpss->GetDisplayName());
        Assert::AreEqual<std::wstring>(L"R", rcpss->GetState()); // This will fail if RpcSs is not running
        Assert::AreEqual<DWORD>(SERVICE_AUTO_START, rcpss->GetStart()); // This will fail if RpcSs is not configured to auto-start
        Assert::AreEqual<std::wstring>(L"C:\\Windows\\System32\\Svchost.exe", rcpss->GetFilepath());
        Assert::AreEqual<std::wstring>(L"rpcss", rcpss->GetSvchostGroup());
        Assert::AreEqual<std::wstring>(L"C:\\Windows\\System32\\Rpcss.dll", rcpss->GetSvchostDll());
        Assert::IsTrue(rcpss->IsSvchostService());
    }

    TEST_METHOD(BeepEmptyImagepathService)
    {
        std::vector<Service> const& services = GetCachedServices();

        auto beep = std::find_if(services.begin(), services.end(), [] (Service const& service) -> bool { 
            return service.GetServiceName() == L"Beep";
        });

        Assert::IsFalse(beep == services.end());
        Assert::AreEqual<std::wstring>(L"Beep", beep->GetDisplayName());
        Assert::AreEqual<std::wstring>(L"R", beep->GetState()); // This will fail if beep is not running
        Assert::AreEqual<DWORD>(1, beep->GetStart()); // This will fail if beep is not configured to auto-start
        Assert::AreEqual<std::wstring>(L"C:\\Windows\\System32\\Drivers\\Beep.sys", beep->GetFilepath());
        Assert::IsTrue(beep->GetSvchostGroup().empty());
        Assert::IsFalse(beep->IsSvchostService());
    }
};
