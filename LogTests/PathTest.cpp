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

static void TestResolve(std::wstring source, std::wstring const& expected)
{
	ResolveFromCommandLine(source);
	ASSERT_EQ(expected, source);
}

TEST(PathResolution, CanonicalPathUnchanged)
{
	TestResolve(L"C:\\Windows\\Explorer.exe", L"C:\\Windows\\Explorer.exe");
}

TEST(PathResolution, NativePathsCanonicalized)
{
	TestResolve(L"C:\\Windows\\Explorer.exe", L"\\??\\C:\\Windows\\Explorer.exe");
}

TEST(PathResolution, NtPathsCanonicalized)
{
	TestResolve(L"C:\\Windows\\Explorer.exe", L"\\\\?\\C:\\Windows\\Explorer.exe");
}

TEST(PathResolution, GlobalrootRemoved)
{
	TestResolve(L"C:\\Windows\\Explorer.exe", L"globalroot\\C:\\Windows\\Explorer.exe");
}

TEST(PathResolution, SlashGlobalrootRemoved)
{
	TestResolve(L"C:\\Windows\\Explorer.exe", L"\\globalroot\\C:\\Windows\\Explorer.exe");
}

TEST(PathResolution, System32Replaced)
{
	TestResolve(L"C:\\Windows\\System32\\Explorer.exe", L"system32\\Explorer.exe");
}

TEST(PathResolution, System32SlashedReplaced)
{
	TestResolve(L"C:\\Windows\\System32\\Explorer.exe", L"\\system32\\Explorer.exe");
}

TEST(PathResolution, WindDirReplaced)
{
	TestResolve(L"C:\\Windows\\System32\\Explorer.exe", L"\\system32\\Explorer.exe");
}
