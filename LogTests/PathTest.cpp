#include "pch.hpp"
#include <string>
#include <boost/algorithm/string/split.hpp>
#include "LogCommon/Win32Exception.hpp"
#include "LogCommon/File.hpp"
#include "LogCommon/Path.hpp"

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

static void TestResolve(std::wstring const& expected, std::wstring source, bool expectedReturn = true)
{
	EXPECT_EQ(expectedReturn, ResolveFromCommandLine(source));
	EXPECT_EQ(expected, source);
}

TEST(PathResolution, DoesNotExistUnchanged)
{
	TestResolve(L"C:\\Windows\\DOESNOTEXIST\\DOESNOTEXIST\\GAHIDONTKNOWWHATSGOINGTOHAPPEN\\Explorer.exe", L"C:\\Windows\\DOESNOTEXIST\\DOESNOTEXIST\\GAHIDONTKNOWWHATSGOINGTOHAPPEN\\Explorer.exe", false);
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

TEST(PathResolution, DefaultKernelFoundOnPath)
{
	TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"ntoskrnl");
}

TEST(PathResolution, AddsExtension)
{
	TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"C:\\Windows\\System32\\Ntoskrnl");
}

TEST(PathResolution, CombinePhaseOneAndTwo)
{
	TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"\\globalroot\\System32\\Ntoskrnl");
}

TEST(PathResolution, RundllVundo)
{
	TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"rundll32 ntoskrnl,ShellExecute");
}

struct PathResolutionFs : public testing::Test
{
	std::vector<std::wstring> pathItems;
	std::wstring const fileName;
	PathResolutionFs()
		: fileName(L"ExampleFileDoesNotExistFindMeFindMeFindMeFindMe.found")
	{ }
	virtual void SetUp()
	{
		using namespace std::placeholders;
		std::wstring pathBuffer;
		DWORD pathLen = ::GetEnvironmentVariableW(L"PATH", nullptr, 0);
		pathBuffer.resize(pathLen);
		::GetEnvironmentVariable(L"PATH", &pathBuffer[0], pathLen);
		boost::algorithm::split(pathItems, pathBuffer, std::bind(std::equal_to<wchar_t>(), _1, L';'));
		ASSERT_LE(3, pathItems.size());
		std::transform(pathItems.begin(), pathItems.end(), pathItems.begin(),
			std::bind(Append, _1, fileName));
	}
};

TEST_F(PathResolutionFs, DISABLED_NoCreateFails)
{
	TestResolve(fileName, fileName, false);
}

TEST_F(PathResolutionFs, DISABLED_SeesLastPathItem)
{
	HANDLE hFile = ::CreateFileW(pathItems.back().c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
	TestResolve(pathItems.back(), fileName);
	::CloseHandle(hFile);
}

TEST_F(PathResolutionFs, DISABLED_RespectsPathOrder)
{
	HANDLE hFile = ::CreateFileW(pathItems[1].c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
	HANDLE hFile2 = ::CreateFileW(pathItems[2].c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
	TestResolve(pathItems[1], fileName);
	::CloseHandle(hFile);
	::CloseHandle(hFile2);
}
