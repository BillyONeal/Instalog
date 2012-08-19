// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include "gtest/gtest.h"
#include "LogCommon/ScanningSections.hpp"

using namespace Instalog;

struct BaseSectionsTest : public testing::Test
{
	std::wostringstream ss;
	std::vector<std::wstring> options;
    ISectionDefinition const* def;
    std::wstring arg;
	void Go()
	{
		def->Execute(ss, ScriptSection(def, arg), options);
	}
};

struct RunningProcessesTest : public BaseSectionsTest
{
	RunningProcesses rp;
    RunningProcessesTest() { def = &rp; }
};

TEST_F(RunningProcessesTest, CommandIsrunningprocesses)
{
	ASSERT_EQ(L"runningprocesses", rp.GetScriptCommand());
}

TEST_F(RunningProcessesTest, NameIsRunningProcesses)
{
	ASSERT_EQ(L"Running Processes", rp.GetName());
}

TEST_F(RunningProcessesTest, PriorityIsScanning)
{
	ASSERT_EQ(SCANNING, rp.GetPriority());
}

TEST_F(RunningProcessesTest, SvchostHasFullLine)
{
	std::wstring svcHost = L"C:\\Windows\\system32\\svchost.exe -k netsvcs";
	Go();
	bool svcHostHasFullLine = boost::algorithm::icontains<std::wstring, std::wstring>(ss.str(), svcHost);
	ASSERT_TRUE(svcHostHasFullLine);
}

TEST_F(RunningProcessesTest, ExplorerDoesNotHaveFullLine)
{
	std::wstring explorer = L"C:\\Windows\\Explorer.exe\n";
	Go();
	ASSERT_PRED2((boost::algorithm::contains<std::wstring, std::wstring>), ss.str(), explorer);
}

TEST_F(RunningProcessesTest, NtoskrnlNotPresent)
{
	Go();
	ASSERT_FALSE(boost::algorithm::contains(ss.str(), L"C:\\Windows\\System32\\Ntoskrnl.exe"));
}

struct ServicesDriversTest : public BaseSectionsTest
{
    ServicesDrivers sd;
    ServicesDriversTest() { def = &sd; }
};

TEST_F(ServicesDriversTest, ScriptCommandIsCorrect)
{
	ASSERT_EQ(L"servicesdrivers", sd.GetScriptCommand());
}

TEST_F(ServicesDriversTest, NameIsCorrect)
{
	ASSERT_EQ(L"Services/Drivers", sd.GetName());
}

TEST_F(ServicesDriversTest, ActuallyGotOutput)
{
	Go();
	ASSERT_FALSE(ss.str().empty());
}

TEST_F(ServicesDriversTest, TcpipWhitelisted)
{
	Go();
	ASSERT_FALSE(boost::algorithm::contains(ss.str(),
		L"R0 Tcpip;TCP/IP Protocol Driver;C:\\Windows\\System32\\Drivers\\Tcpip.sys"));
}

TEST_F(ServicesDriversTest, RpcSsSvchost) 
{
	Go();
	ASSERT_TRUE(boost::algorithm::contains(ss.str(), L"R2 RpcSs;Remote Procedure Call (RPC);rpcss->C:\\Windows\\System32\\Rpcss.dll")) << L"This will fail if RpcSs is not configured to auto-start or is not running";
}

struct EventViewerTest : public BaseSectionsTest
{
    EventViewer ev;
    EventViewerTest() { def = &ev; }
};

TEST_F(EventViewerTest, ScriptCommandIsCorrect)
{
	ASSERT_EQ(L"eventviewer", ev.GetScriptCommand());
}

TEST_F(EventViewerTest, NameIsCorrect)
{
	ASSERT_EQ(L"Event Viewer", ev.GetName());
}

TEST_F(EventViewerTest, ActuallyGotOutput)
{
	Go();
	ASSERT_FALSE(ss.str().empty());
}