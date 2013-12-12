// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include "../LogCommon/resource.h"
#include "../LogCommon/Whitelist.hpp"

using namespace testing;
using Instalog::Whitelist;

TEST(Whitelist, FindsProcess)
{
    Whitelist w(IDR_RUNNINGPROCESSESWHITELIST);
    EXPECT_TRUE(w.IsOnWhitelist("C:\\Windows\\System32\\Ntoskrnl.exe"));
}

TEST(Whitelist, DoesntFindProcess)
{
    Whitelist w(IDR_RUNNINGPROCESSESWHITELIST);
    EXPECT_FALSE(w.IsOnWhitelist("Derp"));
}

TEST(Whitelist, DoesPrefixes)
{
    std::vector<std::pair<std::string, std::string>> replacements;
    replacements.emplace_back("c:\\windows", "d:\\windows");
    Whitelist w(IDR_RUNNINGPROCESSESWHITELIST, replacements);
    EXPECT_TRUE(w.IsOnWhitelist("D:\\Windows\\System32\\Ntoskrnl.exe"));
}

TEST(Whitelist, DoesPrefixesAnyway)
{
    std::vector<std::pair<std::string, std::string>> replacements;
    replacements.emplace_back("c:\\windows", "d:\\windows");
    Whitelist w(IDR_RUNNINGPROCESSESWHITELIST, replacements);
    EXPECT_FALSE(w.IsOnWhitelist("Derp"));
}

TEST(Whitelist, DoesPrefixesAfterLowercase)
{
    std::vector<std::pair<std::string, std::string>> replacements;
    replacements.emplace_back("C:\\windows", "d:\\windows");
    Whitelist w(IDR_RUNNINGPROCESSESWHITELIST, replacements);
    EXPECT_TRUE(w.IsOnWhitelist("D:\\Windows\\System32\\Ntoskrnl.exe"));
}
