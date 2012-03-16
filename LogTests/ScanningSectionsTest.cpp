#include "pch.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include "gtest/gtest.h"
#include "LogCommon/ScanningSections.hpp"

using namespace Instalog;

struct RunningProcessesTest : public testing::Test
{
	RunningProcesses rp;
	std::wostringstream ss;
	ScriptSection section;
	std::vector<std::wstring> options;
	void Go()
	{
		rp.Execute(ss, section, options);
	}
};

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
	ASSERT_PRED2((boost::algorithm::contains<std::wstring, std::wstring>), ss.str(), svcHost);
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

struct ServicesDriversTest : public testing::Test
{
	ServicesDrivers sd;
	std::wostringstream ss;
	ScriptSection section;
	std::vector<std::wstring> options;
	void Go()
	{
		sd.Execute(ss, section, options);
	}
};

TEST_F(ServicesDriversTest, Tcpip)
{
	Go();
	ASSERT_TRUE(boost::algorithm::contains(ss.str(), L"R0 Tcpip;TCP/IP Protocol Driver;C:\\Windows\\System32\\Drivers\\Tcpip.sys\n")) << L"This will fail if Tcpip is not running or does not autostart";
}

TEST_F(ServicesDriversTest, DISABLED_RpcSsSvchost) // TODO: This fails until svchost registry stuff is implemented
{
	Go();
	ASSERT_TRUE(boost::algorithm::contains(ss.str(), L"R2 RpcSs;Remote Procedure Call (RPC);rpcss->C:\\Windows\\System32\\Rpcss.dll\n")) << L"This will fail if RpcSs is not running or does not autostart";;
}