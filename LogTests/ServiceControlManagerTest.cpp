#include "pch.hpp"
#include "gtest/gtest.h"
#include <LogCommon/ServiceControlManager.hpp>

using Instalog::SystemFacades::ServiceControlManager;
using Instalog::SystemFacades::Service;

TEST(ServiceControlManager, CreateServiceControlManager)
{
	EXPECT_NO_THROW(ServiceControlManager scm);
}

#include <iostream>
TEST(ServiceControlManager, GetServices)
{
	ServiceControlManager scm;
	std::vector<Service> services = scm.GetServices();
	for (auto it = services.begin(); it != services.end(); ++it)
	{
		std::wcout << it->getDisplayName() << std::endl;
	}
}