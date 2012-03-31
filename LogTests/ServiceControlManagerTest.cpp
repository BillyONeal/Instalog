// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include <LogCommon/ServiceControlManager.hpp>

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
		EXPECT_FALSE(it->getServiceName().empty());
		EXPECT_FALSE(it->getDisplayName().empty());
		EXPECT_FALSE(it->getState().empty());
		EXPECT_TRUE(it->getServiceName().empty() == it->getDisplayName().empty());
		if (it->getSvchostGroup().empty())
		{
			EXPECT_FALSE(it->isSvchostService());
		}
		else
		{
			EXPECT_TRUE(it->isSvchostService());
		}
	}
}

TEST(ServiceControlManager, TcpipService)
{
	std::vector<Service> const& services = GetCachedServices();

	auto tcpip = std::find_if(services.begin(), services.end(), [] (Service const& service) -> bool { 
		return service.getServiceName() == L"Tcpip";
	});

	ASSERT_NE(tcpip, services.end());
	EXPECT_EQ(L"TCP/IP Protocol Driver", tcpip->getDisplayName());
	EXPECT_EQ(L"R", tcpip->getState()) << L"This will fail if Tcpip is not running";
	EXPECT_EQ(SERVICE_BOOT_START, tcpip->getStart()) << L"This will fail if Tcpip is not configured to auto-start";
	EXPECT_EQ(L"C:\\Windows\\System32\\Drivers\\Tcpip.sys", tcpip->getFilepath());
	EXPECT_TRUE(tcpip->getSvchostGroup().empty());
	EXPECT_TRUE(tcpip->getSvchostDll().empty());
	EXPECT_FALSE(tcpip->isSvchostService());
}

TEST(ServiceControlManager, RpcSsSvchostService)
{
	std::vector<Service> const& services = GetCachedServices();

	auto rcpss = std::find_if(services.begin(), services.end(), [] (Service const& service) -> bool { 
		return service.getServiceName() == L"RpcSs";
		//Triggers compiler bug
		//return boost::algorithm::iequals(service.getServiceName(), L"RpcSs");
	});

	ASSERT_NE(rcpss, services.end());
	EXPECT_EQ(L"Remote Procedure Call (RPC)", rcpss->getDisplayName());
	EXPECT_EQ(L"R", rcpss->getState()) << L"This will fail if RpcSs is not running";
	EXPECT_EQ(SERVICE_AUTO_START, rcpss->getStart()) << L"This will fail if RpcSs is not configured to auto-start";
	EXPECT_EQ(L"C:\\Windows\\System32\\Svchost.exe", rcpss->getFilepath());
	EXPECT_EQ(L"rpcss", rcpss->getSvchostGroup());
	EXPECT_EQ(L"C:\\Windows\\System32\\Rpcss.dll", rcpss->getSvchostDll());
	EXPECT_TRUE(rcpss->isSvchostService());
}

TEST(ServiceControlManager, BeepEmptyImagepathService)
{
	std::vector<Service> const& services = GetCachedServices();

	auto beep = std::find_if(services.begin(), services.end(), [] (Service const& service) -> bool { 
		return service.getServiceName() == L"Beep";
	});

	ASSERT_NE(beep, services.end());
	EXPECT_EQ(L"Beep", beep->getDisplayName());
	EXPECT_EQ(L"R", beep->getState()) << L"This will fail if beep is not running";
	EXPECT_EQ(1, beep->getStart()) << L"This will fail if beep is not configured to auto-start";
	EXPECT_EQ(L"C:\\Windows\\System32\\Drivers\\Beep.sys", beep->getFilepath());
	EXPECT_TRUE(beep->getSvchostGroup().empty());
	EXPECT_FALSE(beep->isSvchostService());
}
