// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "../LogCommon/ServiceControlManager.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include "gtest/gtest.h"

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
        EXPECT_TRUE(it->GetServiceName().empty() ==
                    it->GetDisplayName().empty());
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

    auto tcpip = std::find_if(
        services.begin(), services.end(), [](Service const & service)->bool {
            return service.GetServiceName() == "Tcpip";
        });

    ASSERT_NE(tcpip, services.end());
    EXPECT_STRCASEEQ("TCP/IP Protocol Driver", tcpip->GetDisplayName().c_str());
    EXPECT_EQ("R", tcpip->GetState())
        << "This will fail if Tcpip is not running";
    EXPECT_EQ(SERVICE_BOOT_START, tcpip->GetStart())
        << "This will fail if Tcpip is not configured to auto-start";
    EXPECT_STRCASEEQ("C:\\Windows\\System32\\Drivers\\Tcpip.sys",
              tcpip->GetFilepath().c_str());
    EXPECT_TRUE(tcpip->GetSvchostGroup().empty());
    EXPECT_FALSE(tcpip->GetSvchostDll().is_valid());
    EXPECT_FALSE(tcpip->IsSvchostService());
}

TEST(ServiceControlManager, RpcSsSvchostService)
{
    std::vector<Service> const& services = GetCachedServices();

    auto rcpss = std::find_if(
        services.begin(), services.end(), [](Service const & service)->bool {
            return boost::algorithm::iequals(service.GetServiceName(), "RpcSs");
        });

    ASSERT_NE(rcpss, services.end());
    EXPECT_EQ("Remote Procedure Call (RPC)", rcpss->GetDisplayName());
    EXPECT_EQ("R", rcpss->GetState())
        << "This will fail if RpcSs is not running";
    EXPECT_EQ(SERVICE_AUTO_START, rcpss->GetStart())
        << "This will fail if RpcSs is not configured to auto-start";
    EXPECT_STRCASEEQ("C:\\Windows\\System32\\Svchost.exe", rcpss->GetFilepath().c_str());
    EXPECT_EQ("rpcss", rcpss->GetSvchostGroup());
    EXPECT_STRCASEEQ("C:\\Windows\\System32\\Rpcss.dll",
              rcpss->GetSvchostDll().get().c_str());
    EXPECT_TRUE(rcpss->IsSvchostService());
}

TEST(ServiceControlManager, BeepEmptyImagepathService)
{
    std::vector<Service> const& services = GetCachedServices();

    auto beep = std::find_if(
        services.begin(), services.end(), [](Service const & service)->bool {
            return service.GetServiceName() == "Beep";
        });

    ASSERT_NE(beep, services.end());
    EXPECT_EQ("Beep", beep->GetDisplayName());
    EXPECT_EQ("R", beep->GetState())
        << "This will fail if beep is not running";
    EXPECT_EQ(1, beep->GetStart())
        << "This will fail if beep is not configured to auto-start";
    EXPECT_STRCASEEQ("C:\\Windows\\System32\\Drivers\\Beep.sys", beep->GetFilepath().c_str());
    EXPECT_TRUE(beep->GetSvchostGroup().empty());
    EXPECT_FALSE(beep->IsSvchostService());
}
