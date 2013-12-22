// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include "gtest/gtest.h"
#include "../LogCommon/Win32Glue.hpp"
#include "../LogCommon/ScanningSections.hpp"

using namespace Instalog;

struct BaseSectionsTest : public testing::Test
{
    string_sink ss;
    std::vector<std::string> options;
    ISectionDefinition const* def;
    std::string arg;
    void Go()
    {
        def->Execute(ss, ScriptSection(def, arg), options);
    }
};

struct RunningProcessesTest : public BaseSectionsTest
{
    RunningProcesses rp;
    RunningProcessesTest()
    {
        def = &rp;
    }
};

TEST_F(RunningProcessesTest, CommandIsrunningprocesses)
{
    ASSERT_EQ("runningprocesses", rp.GetScriptCommand());
}

TEST_F(RunningProcessesTest, NameIsRunningProcesses)
{
    ASSERT_EQ("Running Processes", rp.GetName());
}

TEST_F(RunningProcessesTest, PriorityIsScanning)
{
    ASSERT_EQ(SCANNING, rp.GetPriority());
}

inline bool test_icontains(std::string const& haystack,
                           std::string const& needle)
{
    return boost::algorithm::icontains(haystack, needle);
}

TEST_F(RunningProcessesTest, SvchostHasFullLine)
{
    if (Instalog::IsWow64Process())
    {
        // We can't open the svchost processes on wrong bitness.
        return;
    }

    std::string svcHostBad = "C:\\Windows\\system32\\svchost.exe";
    std::string svcHost = "C:\\Windows\\system32\\svchost.exe -k netsvcs";
    Go();
    ASSERT_TRUE(test_icontains(ss.get(), svcHost) || !test_icontains(ss.get(), svcHostBad));
}

TEST_F(RunningProcessesTest, TestsDoNotHaveFullLine)
{
    std::string tests = "Logtests.exe\r\n";
    Go();
    ASSERT_PRED2(&test_icontains, ss.get(), tests);
}

TEST_F(RunningProcessesTest, NtoskrnlNotPresent)
{
    Go();
    ASSERT_FALSE(boost::algorithm::contains(
        ss.get(), "C:\\Windows\\System32\\Ntoskrnl.exe"));
}

struct ServicesDriversTest : public BaseSectionsTest
{
    ServicesDrivers sd;
    ServicesDriversTest()
    {
        def = &sd;
    }
};

TEST_F(ServicesDriversTest, ScriptCommandIsCorrect)
{
    ASSERT_EQ("servicesdrivers", sd.GetScriptCommand());
}

TEST_F(ServicesDriversTest, NameIsCorrect)
{
    ASSERT_EQ("Services/Drivers", sd.GetName());
}

TEST_F(ServicesDriversTest, RpcSsSvchost)
{
    Go();
    ASSERT_TRUE(boost::algorithm::icontains(
        ss.get(),
        "R2 RpcSs;Remote Procedure Call (RPC);rpcss->C:\\Windows\\System32\\Rpcss.dll"))
        << "This will fail if RpcSs is not configured to auto-start or is not running";
}

struct EventViewerTest : public BaseSectionsTest
{
    EventViewer ev;
    EventViewerTest()
    {
        def = &ev;
    }
};

TEST_F(EventViewerTest, ScriptCommandIsCorrect)
{
    ASSERT_EQ("eventviewer", ev.GetScriptCommand());
}

TEST_F(EventViewerTest, NameIsCorrect)
{
    ASSERT_EQ("Event Viewer", ev.GetName());
}

TEST_F(EventViewerTest, ActuallyGotOutput)
{
    Go();
    ASSERT_FALSE(ss.get().empty());
}