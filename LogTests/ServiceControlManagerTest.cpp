#include "pch.hpp"
#include "gtest/gtest.h"
#include <LogCommon/ServiceControlManager.hpp>

using Instalog::SystemFacades::ServiceControlManager;

TEST(ServiceControlManager, CreateServiceControlManager)
{
	EXPECT_NO_THROW(ServiceControlManager scm);
}

TEST(ServiceControlManager, GetServices)
{
	ServiceControlManager scm;
	scm.GetServices();
}