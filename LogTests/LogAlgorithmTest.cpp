// Copyright © Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.
#include "pch.hpp"
#include "../LogCommon/LogAlgorithm.hpp"
#include <vector>

using namespace Instalog;

TEST(LogAlgorithm, ContainsTrue)
{
    std::vector<int> example{42, 1729, 87};
    ASSERT_TRUE(contains(example.cbegin(), example.cend(), 1729));
}

TEST(LogAlgorithm, ContainsFalse)
{
    std::vector<int> example{22, 33, 44};
    ASSERT_FALSE(contains(example.cbegin(), example.cend(), 1729));
}
