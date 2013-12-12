// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <string>
#include <iterator>
#include <boost/algorithm/string/split.hpp>
#include "../LogCommon/Win32Exception.hpp"
#include "../LogCommon/File.hpp"
#include "../LogCommon/Path.hpp"
#include "../LogCommon/Wow64.hpp"
#include "TestSupport.hpp"
#include "../LogCommon/Utf8.hpp"

using namespace Instalog::Path;

TEST(PathAppendTest, NoSlashes)
{
    EXPECT_EQ("one\\two", Append("one", "two"));
}

TEST(PathAppendTest, PathHasSlashes)
{
    EXPECT_EQ("one\\two", Append("one\\", "two"));
}

TEST(PathAppendTest, MoreHasSlashes)
{
    EXPECT_EQ("one\\two", Append("one", "\\two"));
}

TEST(PathAppendTest, BothHaveSlashes)
{
    EXPECT_EQ("one\\two", Append("one\\", "\\two"));
}

TEST(PathAppendTest, PathHasManySlashes)
{
    EXPECT_EQ("one\\\\\\two", Append("one\\\\\\", "two"));
}

TEST(PathAppendTest, MoreHasManySlashes)
{
    EXPECT_EQ("one\\\\\\two", Append("one", "\\\\\\two"));
}

TEST(PathAppendTest, BothHaveManySlashes)
{
    EXPECT_EQ("one\\\\\\\\\\two", Append("one\\\\\\", "\\\\\\two"));
}

TEST(PathAppendTest, MoreIsEmpty)
{
    EXPECT_EQ("one", Append("one", ""));
}

TEST(PathAppendTest, PathIsEmpty)
{
    EXPECT_EQ("two", Append("", "two"));
}

TEST(PathAppendTest, BothAreEmpty)
{
    EXPECT_EQ("", Append("", ""));
}

TEST(PathAppendTest, AppendFailure)
{
    EXPECT_STREQ(
        "c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\bin\\amd64\\ExampleFileDoesNotExistFindMeFindMeFindMeFindMe.found",
        Append(
            "c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\bin\\amd64",
            "ExampleFileDoesNotExistFindMeFindMeFindMeFindMe.found").c_str());
}

TEST(PathPrettify, CorrectOutput)
{
    std::string path("C:\\ExAmPlE\\FooBar\\Target.EXE");
    Prettify(path.begin(), path.end());
    ASSERT_EQ("C:\\Example\\Foobar\\Target.exe", path);
}

TEST(PathPrettify, DriveCapitalized)
{
    std::string path("c:\\Example\\Foobar\\Target.exe");
    Prettify(path.begin(), path.end());
    ASSERT_EQ("C:\\Example\\Foobar\\Target.exe", path);
}

TEST(PathPrettify, SpacesOkay)
{
    std::string path("C:\\Example\\Foo bar\\Target.exe");
    Prettify(path.begin(), path.end());
    ASSERT_EQ("C:\\Example\\Foo bar\\Target.exe", path);
}

static void TestExpansion(std::string const& expected,
                          std::string source,
                          bool expectedReturn = true)
{
    EXPECT_EQ(expectedReturn, ExpandShortPath(source));
    EXPECT_EQ(expected, source);
}

TEST(PathExpanding, DirectoryExpansion)
{
    TestExpansion("C:\\Program Files\\", "C:\\Progra~1\\");
}

TEST(PathExpanding, NonExistantDirectoryExpansion)
{
    TestExpansion("C:\\zzzzz~1\\", "C:\\zzzzz~1\\", false);
}

TEST(PathExpanding, FileExpansion)
{
    HANDLE hFile = ::CreateFileW(L"Temporary Long Path File",
                                 GENERIC_WRITE,
                                 0,
                                 0,
                                 CREATE_ALWAYS,
                                 FILE_FLAG_DELETE_ON_CLOSE,
                                 0);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::wcout << ::GetLastError() << std::endl;
    }

    TestExpansion("Temporary Long Path File", "Tempor~1", true);

    ::CloseHandle(hFile);
}

TEST(PathExpanding, NonExistantFileExpansion)
{
    TestExpansion("zzzzzz~1", "zzzzzz~1", false);
}

static void TestResolve(std::string expected,
                        std::string source,
                        bool expectedReturn = true)
{
    Instalog::SystemFacades::NativeFilePathScope scope;
    EXPECT_EQ(expectedReturn, ResolveFromCommandLine(source));
    EXPECT_TRUE(boost::algorithm::iequals(expected, source))
        << "Expected file name\n" << expected << "\nbut got\n" << source;
}

TEST(PathResolution, EmptyGivesEmpty)
{
    TestResolve("", "", false);
}

TEST(PathResolution, DoesNotExistUnchanged)
{
    TestResolve(
        "C:\\Windows\\DOESNOTEXIST\\DOESNOTEXIST\\GAHIDONTKNOWWHATSGOINGTOHAPPEN\\Explorer.exe",
        "C:\\Windows\\DOESNOTEXIST\\DOESNOTEXIST\\GAHIDONTKNOWWHATSGOINGTOHAPPEN\\Explorer.exe",
        false);
}

TEST(PathResolution, CanonicalPathUnchanged)
{
    TestResolve("C:\\Windows\\Explorer.exe", "C:\\Windows\\Explorer.exe");
}

TEST(PathResolution, NativePathsCanonicalized)
{
    TestResolve("C:\\Windows\\Explorer.exe",
                "\\??\\C:\\Windows\\Explorer.exe");
}

TEST(PathResolution, NtPathsCanonicalized)
{
    TestResolve("C:\\Windows\\Explorer.exe",
                "\\\\?\\C:\\Windows\\Explorer.exe");
}

TEST(PathResolution, GlobalrootRemoved)
{
    TestResolve("C:\\Windows\\Explorer.exe",
                "globalroot\\C:\\Windows\\Explorer.exe");
}

TEST(PathResolution, SlashGlobalrootRemoved)
{
    TestResolve("C:\\Windows\\Explorer.exe",
                "\\globalroot\\C:\\Windows\\Explorer.exe");
}

TEST(PathResolution, System32Replaced)
{
    TestResolve("C:\\Windows\\System32\\Ntoskrnl.exe",
                "system32\\Ntoskrnl.exe");
}

TEST(PathResolution, System32SlashedReplaced)
{
    TestResolve("C:\\Windows\\System32\\Ntoskrnl.exe",
                "\\system32\\Ntoskrnl.exe");
}

TEST(PathResolution, WindDirReplaced)
{
    TestResolve("C:\\Windows\\System32\\Ntoskrnl.exe",
                "\\systemroot\\system32\\Ntoskrnl.exe");
}

TEST(PathResolution, DefaultKernelFoundOnPath)
{
    TestResolve("C:\\Windows\\System32\\Ntoskrnl.exe", "ntoskrnl");
}

TEST(PathResolution, AddsExtension)
{
    TestResolve("C:\\Windows\\System32\\Ntoskrnl.exe",
                "C:\\Windows\\System32\\Ntoskrnl");
}

TEST(PathResolution, CombinePhaseOneAndTwo)
{
    TestResolve("C:\\Windows\\System32\\Ntoskrnl.exe",
                "\\globalroot\\System32\\Ntoskrnl");
}

TEST(PathResolution, RundllVundo)
{
    TestResolve("C:\\Windows\\System32\\Ntoskrnl.exe",
                "rundll32 ntoskrnl,ShellExecute");
}

TEST(PathResolution, RundllVundoSpaces)
{
    TestResolve(
        "C:\\Windows\\System32\\Ntoskrnl.exe",
        "rundll32                                                 ntoskrnl,ShellExecute");
}

TEST(PathResolution, QuotedPath)
{
    TestResolve(
        "C:\\Program files\\Windows nt\\Accessories\\Wordpad.exe",
        "\"C:\\Program Files\\Windows nt\\Accessories\\Wordpad.exe\"  Arguments Arguments Arguments");
}

TEST(PathResolution, QuotedPathRundll)
{
    std::string testPath(GetTestFilePath("ExampleTestingFile.exe"));
    std::wstring testPathWide(utf8::ToUtf16(testPath));
    HANDLE hFile = ::CreateFileW(testPathWide.c_str(),
                                 GENERIC_WRITE,
                                 0,
                                 0,
                                 CREATE_ALWAYS,
                                 FILE_FLAG_DELETE_ON_CLOSE,
                                 0);
    TestResolve(
        testPath,
        "\"C:\\Windows\\System32\\Rundll32.exe\" \"" + testPath + ",Argument arg arg\"");
    ::CloseHandle(hFile);
}

struct PathResolutionPathOrderFixture : public testing::Test
{
    std::vector<std::string> pathItems;
    std::string const fileName;
    PathResolutionPathOrderFixture()
        : fileName("ExampleFileDoesNotExistFindMeFindMeFindMeFindMe.found")
    {
    }
    virtual void SetUp()
    {
        using namespace std::placeholders;
        std::wstring pathBuffer;
        DWORD pathLen = ::GetEnvironmentVariableW(L"PATH", nullptr, 0);
        pathBuffer.resize(pathLen);
        ::GetEnvironmentVariableW(L"PATH", &pathBuffer[0], pathLen);
        pathBuffer.pop_back(); // remove null
        std::vector<std::wstring> pathWideItems;
        boost::algorithm::split(pathWideItems,
                                pathBuffer,
                                std::bind1st(std::equal_to<wchar_t>(), L';'));
        ASSERT_LE(3ul, pathWideItems.size());
        std::transform(pathWideItems.begin(),
                       pathWideItems.end(),
                       std::back_inserter(pathItems),
                       [&](std::wstring & x) { return Append(utf8::ToUtf8(x), fileName); });
        std::for_each(pathItems.begin(), pathItems.end(), [](std::string & a) {
            Prettify(a.begin(), a.end());
        });
    }
};

TEST_F(PathResolutionPathOrderFixture, NoCreateFails)
{
    TestResolve(fileName, fileName, false);
}

TEST_F(PathResolutionPathOrderFixture, SeesLastPathItem)
{
    HANDLE hFile = ::CreateFileW(utf8::ToUtf16(pathItems.back()).c_str(),
                                 GENERIC_WRITE,
                                 0,
                                 0,
                                 CREATE_ALWAYS,
                                 FILE_FLAG_DELETE_ON_CLOSE,
                                 0);
    TestResolve(pathItems.back(), fileName);
    ::CloseHandle(hFile);
}

TEST_F(PathResolutionPathOrderFixture, RespectsPathOrder)
{
    // Create 2 temporary files at the first parts of %PATH% which succeed.
    HANDLE files[2];
    std::size_t idx = 0;
    std::size_t firstFileIdx = 0;
    std::size_t foundFiles = 0;
    for (; idx < pathItems.size() && foundFiles < _countof(files); ++idx)
    {
        HANDLE hCurrent = ::CreateFileW(utf8::ToUtf16(pathItems[idx]).c_str(),
                                        GENERIC_WRITE,
                                        0,
                                        0,
                                        CREATE_ALWAYS,
                                        FILE_FLAG_DELETE_ON_CLOSE,
                                        0);

        if (hCurrent == INVALID_HANDLE_VALUE)
        {
            continue;
        }

        if (foundFiles == 0)
        {
            firstFileIdx = idx;
        }

        files[foundFiles++] = hCurrent;
    }

    ASSERT_EQ(2, foundFiles);

    // Verify that the first file in %PATH% is resolved, not the second.
    TestResolve(pathItems[firstFileIdx], fileName);

    // Teardown / close temporary bits
    for (HANDLE hFile : files)
    {
        ::CloseHandle(hFile);
    }
}

struct PathResolutionPathExtOrderFixture : public testing::Test
{
    std::vector<std::string> pathItems;
    std::string const fileName;
    PathResolutionPathExtOrderFixture()
        : fileName("ExampleFileDoesNotExistFindMeFindMeFindMeFindMeExt")
    {
    }
    virtual void SetUp()
    {
        std::wstring pathBuffer;
        DWORD pathLen = ::GetEnvironmentVariableW(L"PATHEXT", nullptr, 0);
        pathBuffer.resize(pathLen);
        ::GetEnvironmentVariable(L"PATHEXT", &pathBuffer[0], pathLen);
        pathBuffer.pop_back(); // remove null
        std::vector<std::wstring> pathItemsWide;
        boost::algorithm::split(pathItemsWide,
                                pathBuffer,
                                std::bind1st(std::equal_to<wchar_t>(), L';'));
        ASSERT_LE(3u, pathItemsWide.size());
        std::transform(pathItemsWide.cbegin(),
                       pathItemsWide.cend(),
                       std::back_inserter(pathItems),
                       [](std::wstring const& s)
        { return utf8::ToUtf8(s); });
        std::for_each(pathItems.begin(),
                      pathItems.end(),
                      [this](std::string & a) { a.insert(0, fileName); });
        std::transform(pathItems.begin(),
                       pathItems.end(),
                       pathItems.begin(),
                       [](std::string &
                          x) { return Append("C:\\Windows\\System32", x); });
        std::for_each(pathItems.begin(), pathItems.end(), [](std::string & a) {
            Prettify(a.begin(), a.end());
        });
    }
};

TEST_F(PathResolutionPathExtOrderFixture, NoCreateFails)
{
    TestResolve(fileName, fileName, false);
}

TEST_F(PathResolutionPathExtOrderFixture, SeesLastPathItem)
{
    HANDLE hFile = ::CreateFileW(utf8::ToUtf16(pathItems.back()).c_str(),
                                 GENERIC_WRITE,
                                 0,
                                 0,
                                 CREATE_ALWAYS,
                                 FILE_FLAG_DELETE_ON_CLOSE,
                                 0);
    TestResolve(pathItems.back(), fileName);
    ::CloseHandle(hFile);
}

TEST_F(PathResolutionPathExtOrderFixture, RespectsPathExtOrder)
{
    HANDLE hFile = ::CreateFileW(utf8::ToUtf16(pathItems[1]).c_str(),
                                 GENERIC_WRITE,
                                 0,
                                 0,
                                 CREATE_ALWAYS,
                                 FILE_FLAG_DELETE_ON_CLOSE,
                                 0);
    HANDLE hFile2 = ::CreateFileW(utf8::ToUtf16(pathItems[2]).c_str(),
                                  GENERIC_WRITE,
                                  0,
                                  0,
                                  CREATE_ALWAYS,
                                  FILE_FLAG_DELETE_ON_CLOSE,
                                  0);
    TestResolve(pathItems[1], fileName);
    ::CloseHandle(hFile);
    ::CloseHandle(hFile2);
}
