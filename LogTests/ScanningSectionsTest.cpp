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
