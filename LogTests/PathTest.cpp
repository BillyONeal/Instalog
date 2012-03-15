#include "pch.hpp"
#include <string>
#include <LogCommon/Path.hpp>

using namespace Instalog::Path;

TEST(PathAppendTest, NoSlashes)
{
	EXPECT_EQ(L"one\\two", Append(L"one", L"two"));
}

TEST(PathAppendTest, PathHasSlashes)
{
	EXPECT_EQ(L"one\\two", Append(L"one\\", L"two"));
}

TEST(PathAppendTest, MoreHasSlashes)
{
	EXPECT_EQ(L"one\\two", Append(L"one", L"\\two"));
}

TEST(PathAppendTest, BothHaveSlashes)
{
	EXPECT_EQ(L"one\\two", Append(L"one\\", L"\\two"));
}

TEST(PathAppendTest, PathHasManySlashes)
{
	EXPECT_EQ(L"one\\\\\\two", Append(L"one\\\\\\", L"two"));
}

TEST(PathAppendTest, MoreHasManySlashes)
{
	EXPECT_EQ(L"one\\\\\\two", Append(L"one", L"\\\\\\two"));
}

TEST(PathAppendTest, BothHaveManySlashes)
{
	EXPECT_EQ(L"one\\\\\\\\\\two", Append(L"one\\\\\\", L"\\\\\\two"));
}

TEST(PathAppendTest, MoreIsEmpty)
{
	EXPECT_EQ(L"one", Append(L"one", L""));
}

TEST(PathAppendTest, PathIsEmpty)
{
	EXPECT_EQ(L"two", Append(L"", L"two"));
}

TEST(PathAppendTest, BothAreEmpty)
{
	EXPECT_EQ(L"", Append(L"", L""));
}

TEST(PathPrettify, CorrectOutput)
{
	std::wstring path(L"C:\\ExAmPlE\\FooBar\\Target.EXE");
	Prettify(path.begin(), path.end());
	ASSERT_EQ(L"C:\\Example\\Foobar\\Target.exe", path);
}

TEST(PathPrettify, DriveCapitalized)
{
	std::wstring path(L"c:\\Example\\Foobar\\Target.exe");
	Prettify(path.begin(), path.end());
	ASSERT_EQ(L"C:\\Example\\Foobar\\Target.exe", path);
}

TEST(PathPrettify, SpacesOkay)
{
	std::wstring path(L"C:\\Example\\Foo bar\\Target.exe");
	Prettify(path.begin(), path.end());
	ASSERT_EQ(L"C:\\Example\\Foo bar\\Target.exe", path);
}

static void TestResolve(std::wstring const& expected, std::wstring source)
{
	ResolveFromCommandLine(source);
	ASSERT_EQ(expected, source);
}

TEST(PathResolution, DoesNotExistUnchanged)
{
	TestResolve(L"C:\\Windows\\DOESNOTEXIST\\DOESNOTEXIST\\GAHIDONTKNOWWHATSGOINGTOHAPPEN\\Explorer.exe", L"C:\\Windows\\DOESNOTEXIST\\DOESNOTEXIST\\GAHIDONTKNOWWHATSGOINGTOHAPPEN\\Explorer.exe");
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
	TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"system32\\Ntoskrnl.exe");
}

TEST(PathResolution, System32SlashedReplaced)
{
	TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"\\system32\\Ntoskrnl.exe");
}

TEST(PathResolution, WindDirReplaced)
{
	TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"\\system32\\Ntoskrnl.exe");
}
