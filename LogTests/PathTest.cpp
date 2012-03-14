#include "pch.hpp"
#include <string>
#include <LogCommon/Path.hpp>

using namespace Instalog::Path;

TEST(Path, NoSlashes)
{
	EXPECT_EQ(L"one\\two", Append(L"one", L"two"));
}

TEST(Path, PathHasSlashes)
{
	EXPECT_EQ(L"one\\two", Append(L"one\\", L"two"));
}

TEST(Path, MoreHasSlashes)
{
	EXPECT_EQ(L"one\\two", Append(L"one", L"\\two"));
}

TEST(Path, BothHaveSlashes)
{
	EXPECT_EQ(L"one\\two", Append(L"one\\", L"\\two"));
}

TEST(Path, PathHasManySlashes)
{
	EXPECT_EQ(L"one\\\\\\two", Append(L"one\\\\\\", L"two"));
}

TEST(Path, MoreHasManySlashes)
{
	EXPECT_EQ(L"one\\\\\\two", Append(L"one", L"\\\\\\two"));
}

TEST(Path, BothHaveManySlashes)
{
	EXPECT_EQ(L"one\\\\\\\\\\two", Append(L"one\\\\\\", L"\\\\\\two"));
}

TEST(Path, MoreIsEmpty)
{
	EXPECT_EQ(L"one", Append(L"one", L""));
}

TEST(Path, PathIsEmpty)
{
	EXPECT_EQ(L"two", Append(L"", L"two"));
}

TEST(Path, BothAreEmpty)
{
	EXPECT_EQ(L"", Append(L"", L""));
}