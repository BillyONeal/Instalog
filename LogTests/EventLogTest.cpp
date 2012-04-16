// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include "LogCommon/EventLog.hpp"

using Instalog::SystemFacades::OldEventLog;
using Instalog::SystemFacades::OldEventLogEntry;
using Instalog::SystemFacades::XmlEventLog;
using Instalog::SystemFacades::XmlEventLogEntry;

TEST(OldEventLog, ReceivedEventLogEntries)
{
	OldEventLog eventLog;

	std::vector<OldEventLogEntry> eventLogEntries = eventLog.ReadEvents();

	ASSERT_TRUE(eventLogEntries.size() > 0);
}

TEST(XmlEventLog, ReceivedEventLogEntries)
{
	XmlEventLog eventLog;
	std::vector<XmlEventLogEntry> eventLogEntries(eventLog.ReadEvents());

	ASSERT_TRUE(eventLogEntries.size() > 0);
}