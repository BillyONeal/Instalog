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
	static std::vector<Service> services;
	if (services.empty())
	{
		ServiceControlManager scm;
		services = scm.GetServices();
	}
	return services;
}

TEST(ServiceControlManager, GetServices)
{
	std::vector<Service> services = GetCachedServices();
	EXPECT_FALSE(services.empty());
}

TEST(ServiceControlManager, DISABLED_FieldsSetCorrectly) // TODO: This fails until svchost registry stuff is implemented
{
	std::vector<Service> services = GetCachedServices();
	
	for (auto it = services.begin(); it != services.end(); ++it)
	{
		EXPECT_FALSE(it->getServiceName().empty());
		EXPECT_FALSE(it->getDisplayName().empty());
		EXPECT_FALSE(it->getState().empty());
		EXPECT_FALSE(it->getFilepath().empty());
		EXPECT_TRUE(it->getServiceName().empty() == it->getDisplayName().empty());
		if (it->getServiceName().empty() == false || it->getDisplayName().empty() == false)
		{
			EXPECT_TRUE(it->isSvchostService());
		}
		else
		{
			EXPECT_FALSE(it->isSvchostService());
		}
	}
}

TEST(ServiceControlManager, TcpipService)
{
	std::vector<Service> services = GetCachedServices();

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

TEST(ServiceControlManager, DISABLED_RpcSsSvchostService) // TODO: This fails until svchost registry stuff is implemented
{
	std::vector<Service> services = GetCachedServices();

	auto rcpss = std::find_if(services.begin(), services.end(), [] (Service const& service) -> bool { 
		//return service.getServiceName() == L"RpcSs";
		return boost::algorithm::iequals(service.getServiceName(), L"RpcSs");
	});

	ASSERT_NE(rcpss, services.end());
	EXPECT_EQ(L"Remote Procedure Call (RPC)", rcpss->getDisplayName());
	EXPECT_EQ(L"R", rcpss->getState()) << L"This will fail if rcpss is not running";
	EXPECT_EQ(SERVICE_AUTO_START, rcpss->getStart()) << L"This will fail if rcpss is not configured to auto-start";
	EXPECT_EQ(L"C:\\Windows\\System32\\Svchost.exe", rcpss->getFilepath());
	EXPECT_EQ(L"rpcss", rcpss->getSvchostGroup());
	EXPECT_EQ(L"C:\\Windows\\System32\\Rpcss.dll", rcpss->getSvchostDll());
	EXPECT_TRUE(rcpss->isSvchostService());
}