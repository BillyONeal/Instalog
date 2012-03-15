#include "pch.hpp"
#include "gtest/gtest.h"
#include "LogCommon/resource.h"
#include "LogCommon/Whitelist.hpp"

using namespace testing;
using Instalog::Whitelist;

TEST(Whitelist, FindsProcess)
{
	Whitelist w(IDR_RUNNINGPROCESSESWHITELIST);
	EXPECT_TRUE(w.IsOnWhitelist(L"C:\\Windows\\System32\\Ntoskrnl.exe"));
}

TEST(Whitelist, DoesntFindProcess)
{
	Whitelist w(IDR_RUNNINGPROCESSESWHITELIST);
	EXPECT_FALSE(w.IsOnWhitelist(L"Derp"));
}
