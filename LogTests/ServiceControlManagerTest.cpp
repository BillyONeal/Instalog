// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include "../LogCommon/ServiceControlManager.hpp"

using Instalog::SystemFacades::ServiceControlManager;
using Instalog::SystemFacades::Service;

TEST(ServiceControlManager, CreateServiceControlManager)
{
    EXPECT_NO_THROW(ServiceControlManager scm);
}

static std::vector<Service> const& GetCachedServices()
{
    static ServiceControlManager scm;
    static std::vector<Service> services(scm.GetServices());
    return services;
}

TEST(ServiceControlManager, GetServices)
{
    std::vector<Service> const& services = GetCachedServices();
    EXPECT_FALSE(services.empty());
}

TEST(ServiceControlManager, FieldsSetCorrectly)
{
    std::vector<Service> const& services = GetCachedServices();
    
    for (auto it = services.begin(); it != services.end(); ++it)
    {
        EXPECT_FALSE(it->GetServiceName().empty());
        EXPECT_FALSE(it->GetDisplayName().empty());
        EXPECT_FALSE(it->GetState().empty());
        EXPECT_TRUE(it->GetServiceName().empty() == it->GetDisplayName().empty());
        if (it->GetSvchostGroup().empty())
        {
            EXPECT_FALSE(it->IsSvchostService());
        }
        else
        {
            EXPECT_TRUE(it->IsSvchostService());
        }
    }
}

TEST(ServiceControlManager, TcpipService)
{
    std::vector<Service> const& services = GetCachedServices();

    auto tcpip = std::find_if(services.begin(), services.end(), [] (Service const& service) -> bool { 
        return service.GetServiceName() == L"Tcpip";
    });

    ASSERT_NE(tcpip, services.end());
    EXPECT_EQ(L"TCP/IP Protocol Driver", tcpip->GetDisplayName());
    EXPECT_EQ(L"R", tcpip->GetState()) << L"This will fail if Tcpip is not running";
    EXPECT_EQ(SERVICE_BOOT_START, tcpip->GetStart()) << L"This will fail if Tcpip is not configured to auto-start";
    EXPECT_EQ(L"C:\\Windows\\System32\\Drivers\\Tcpip.sys", tcpip->GetFilepath());
    EXPECT_TRUE(tcpip->GetSvchostGroup().empty());
    EXPECT_TRUE(tcpip->GetSvchostDll().empty());
    EXPECT_FALSE(tcpip->IsSvchostService());
}

TEST(ServiceControlManager, RpcSsSvchostService)
{
    std::vector<Service> const& services = GetCachedServices();

    auto rcpss = std::find_if(services.begin(), services.end(), [] (Service const& service) -> bool { 
        return service.GetServiceName() == L"RpcSs";
        //Triggers compiler bug
        //return boost::algorithm::iequals(service.getServiceName(), L"RpcSs");
    });

    ASSERT_NE(rcpss, services.end());
    EXPECT_EQ(L"Remote Procedure Call (RPC)", rcpss->GetDisplayName());
    EXPECT_EQ(L"R", rcpss->GetState()) << L"This will fail if RpcSs is not running";
    EXPECT_EQ(SERVICE_AUTO_START, rcpss->GetStart()) << L"This will fail if RpcSs is not configured to auto-start";
    EXPECT_EQ(L"C:\\Windows\\System32\\Svchost.exe", rcpss->GetFilepath());
    EXPECT_EQ(L"rpcss", rcpss->GetSvchostGroup());
    EXPECT_EQ(L"C:\\Windows\\System32\\Rpcss.dll", rcpss->GetSvchostDll());
    EXPECT_TRUE(rcpss->IsSvchostService());
}

TEST(ServiceControlManager, BeepEmptyImagepathService)
{
    std::vector<Service> const& services = GetCachedServices();

    auto beep = std::find_if(services.begin(), services.end(), [] (Service const& service) -> bool { 
        return service.GetServiceName() == L"Beep";
    });

    ASSERT_NE(beep, services.end());
    EXPECT_EQ(L"Beep", beep->GetDisplayName());
    EXPECT_EQ(L"R", beep->GetState()) << L"This will fail if beep is not running";
    EXPECT_EQ(1, beep->GetStart()) << L"This will fail if beep is not configured to auto-start";
    EXPECT_EQ(L"C:\\Windows\\System32\\Drivers\\Beep.sys", beep->GetFilepath());
    EXPECT_TRUE(beep->GetSvchostGroup().empty());
    EXPECT_FALSE(beep->IsSvchostService());
}
