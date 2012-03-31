#include "pch.hpp"
#include "gtest/gtest.h"
#include "LogCommon/EventLog.hpp"

using Instalog::SystemFacades::EventLog;

TEST(EventLog, Test)
{
	EventLog eventLog;

	eventLog.ReadEvents();
}