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

TEST(Whitelist, DoesPrefixes)
{
	std::vector<std::pair<std::wstring, std::wstring>> replacements;
	replacements.emplace_back(std::pair<std::wstring, std::wstring>(L"c:\\windows", L"d:\\windows"));
	Whitelist w(IDR_RUNNINGPROCESSESWHITELIST, replacements);
	EXPECT_TRUE(w.IsOnWhitelist(L"D:\\Windows\\System32\\Ntoskrnl.exe"));
}

TEST(Whitelist, DoesPrefixesAnyway)
{
	std::vector<std::pair<std::wstring, std::wstring>> replacements;
	replacements.emplace_back(std::pair<std::wstring, std::wstring>(L"c:\\windows", L"d:\\windows"));
	Whitelist w(IDR_RUNNINGPROCESSESWHITELIST, replacements);
	EXPECT_FALSE(w.IsOnWhitelist(L"Derp"));
}

TEST(Whitelist, DoesPrefixesAfterLowercase)
{
	std::vector<std::pair<std::wstring, std::wstring>> replacements;
	replacements.emplace_back(std::pair<std::wstring, std::wstring>(L"C:\\windows", L"d:\\windows"));
	Whitelist w(IDR_RUNNINGPROCESSESWHITELIST, replacements);
	EXPECT_FALSE(w.IsOnWhitelist(L"D:\\Windows\\System32\\Ntoskrnl.exe"));
}
