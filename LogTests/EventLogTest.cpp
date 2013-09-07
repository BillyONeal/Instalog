// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include "../LogCommon/EventLog.hpp"

using Instalog::SystemFacades::EventLogEntry;
using Instalog::SystemFacades::OldEventLog;
using Instalog::SystemFacades::XmlEventLog;

TEST(OldEventLog, ReceivedEventLogEntries)
{
    OldEventLog eventLog;

    std::vector<std::unique_ptr<EventLogEntry>> eventLogEntries =
        eventLog.ReadEvents();

    ASSERT_TRUE(eventLogEntries.size() > 0);
}

TEST(XmlEventLog, ReceivedEventLogEntries)
{
    XmlEventLog eventLog;

    std::vector<std::unique_ptr<EventLogEntry>> eventLogEntries(
        eventLog.ReadEvents());

    ASSERT_TRUE(eventLogEntries.size() > 0);
}