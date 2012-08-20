// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include "../LogCommon/resource.h"
#include "../LogCommon/Whitelist.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using Instalog::Whitelist;

TEST_CLASS(WhitelistTests)
{
public:
	TEST_METHOD(FindsProcess)
	{
		Whitelist w(IDR_RUNNINGPROCESSESWHITELIST);
		Assert::IsTrue(w.IsOnWhitelist(L"C:\\Windows\\System32\\Ntoskrnl.exe"));
	}

	TEST_METHOD(DoesntFindProcess)
	{
		Whitelist w(IDR_RUNNINGPROCESSESWHITELIST);
		Assert::IsFalse(w.IsOnWhitelist(L"Derp"));
	}

	TEST_METHOD(DoesPrefixes)
	{
		std::vector<std::pair<std::wstring, std::wstring>> replacements;
		replacements.emplace_back(std::pair<std::wstring, std::wstring>(L"c:\\windows", L"d:\\windows"));
		Whitelist w(IDR_RUNNINGPROCESSESWHITELIST, replacements);
		Assert::IsTrue(w.IsOnWhitelist(L"D:\\Windows\\System32\\Ntoskrnl.exe"));
	}

	TEST_METHOD(DoesPrefixesAnyway)
	{
		std::vector<std::pair<std::wstring, std::wstring>> replacements;
		replacements.emplace_back(std::pair<std::wstring, std::wstring>(L"c:\\windows", L"d:\\windows"));
		Whitelist w(IDR_RUNNINGPROCESSESWHITELIST, replacements);
		Assert::IsFalse(w.IsOnWhitelist(L"Derp"));
	}

	TEST_METHOD(DoesPrefixesAfterLowercase)
	{
		std::vector<std::pair<std::wstring, std::wstring>> replacements;
		replacements.emplace_back(std::pair<std::wstring, std::wstring>(L"C:\\windows", L"d:\\windows"));
		Whitelist w(IDR_RUNNINGPROCESSESWHITELIST, replacements);
		Assert::IsTrue(w.IsOnWhitelist(L"D:\\Windows\\System32\\Ntoskrnl.exe"));
	}
};
