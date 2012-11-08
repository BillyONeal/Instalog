// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include "../LogCommon/EventLog.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using Instalog::SystemFacades::EventLogEntry;
using Instalog::SystemFacades::OldEventLog;
using Instalog::SystemFacades::XmlEventLog;

TEST_CLASS(EventLogTests)
{
    TEST_METHOD(OldEventLogReceivedEventLogEntries)
    {
        OldEventLog eventLog;

        std::vector<std::unique_ptr<EventLogEntry>> eventLogEntries(eventLog.ReadEvents());

        Assert::IsTrue(eventLogEntries.size() > 0);
    }

    TEST_METHOD(XmlEventLogReceivedEventLogEntries)
    {
        XmlEventLog eventLog;

        std::vector<std::unique_ptr<EventLogEntry>> eventLogEntries(eventLog.ReadEvents());

        Assert::IsTrue(eventLogEntries.size() > 0);
    }
};

