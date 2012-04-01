// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include "LogCommon/Win32Glue.hpp"

using namespace Instalog;

TEST(Win32Glue, SecondsSince1970)
{
	SYSTEMTIME jan1970plusDay = { 1970, 1, 4,2,0,0,0,0};
	ASSERT_EQ(86400, SecondsSince1970(jan1970plusDay));
}