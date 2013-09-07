// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <string>
#include <iterator>
#include <boost/algorithm/string/split.hpp>
#include "../LogCommon/Win32Exception.hpp"
#include "../LogCommon/File.hpp"
#include "../LogCommon/Path.hpp"

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

TEST(PathAppendTest, AppendFailure)
{
    EXPECT_STREQ(
        L"c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\bin\\amd64\\ExampleFileDoesNotExistFindMeFindMeFindMeFindMe.found",
        Append(
            L"c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\bin\\amd64",
            L"ExampleFileDoesNotExistFindMeFindMeFindMeFindMe.found").c_str());
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

static void TestExpansion(std::wstring const& expected,
                          std::wstring source,
                          bool expectedReturn = true)
{
    EXPECT_EQ(expectedReturn, ExpandShortPath(source));
    EXPECT_EQ(expected, source);
}

TEST(PathExpanding, DirectoryExpansion)
{
    TestExpansion(L"C:\\Program Files\\", L"C:\\Progra~1\\");
}

TEST(PathExpanding, NonExistantDirectoryExpansion)
{
    TestExpansion(L"C:\\zzzzz~1\\", L"C:\\zzzzz~1\\", false);
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

    TestExpansion(L"Temporary Long Path File", L"Tempor~1", true);

    ::CloseHandle(hFile);
}

TEST(PathExpanding, NonExistantFileExpansion)
{
    TestExpansion(L"zzzzzz~1", L"zzzzzz~1", false);
}

static void TestResolve(std::wstring const& expected,
                        std::wstring source,
                        bool expectedReturn = true)
{
    EXPECT_EQ(expectedReturn, ResolveFromCommandLine(source));
    EXPECT_EQ(expected, source);
}

TEST(PathResolution, EmptyGivesEmpty)
{
    TestResolve(L"", L"", false);
}

TEST(PathResolution, DoesNotExistUnchanged)
{
    TestResolve(
        L"C:\\Windows\\DOESNOTEXIST\\DOESNOTEXIST\\GAHIDONTKNOWWHATSGOINGTOHAPPEN\\Explorer.exe",
        L"C:\\Windows\\DOESNOTEXIST\\DOESNOTEXIST\\GAHIDONTKNOWWHATSGOINGTOHAPPEN\\Explorer.exe",
        false);
}

TEST(PathResolution, CanonicalPathUnchanged)
{
    TestResolve(L"C:\\Windows\\Explorer.exe", L"C:\\Windows\\Explorer.exe");
}

TEST(PathResolution, NativePathsCanonicalized)
{
    TestResolve(L"C:\\Windows\\Explorer.exe",
                L"\\??\\C:\\Windows\\Explorer.exe");
}

TEST(PathResolution, NtPathsCanonicalized)
{
    TestResolve(L"C:\\Windows\\Explorer.exe",
                L"\\\\?\\C:\\Windows\\Explorer.exe");
}

TEST(PathResolution, GlobalrootRemoved)
{
    TestResolve(L"C:\\Windows\\Explorer.exe",
                L"globalroot\\C:\\Windows\\Explorer.exe");
}

TEST(PathResolution, SlashGlobalrootRemoved)
{
    TestResolve(L"C:\\Windows\\Explorer.exe",
                L"\\globalroot\\C:\\Windows\\Explorer.exe");
}

TEST(PathResolution, System32Replaced)
{
    TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe",
                L"system32\\Ntoskrnl.exe");
}

TEST(PathResolution, System32SlashedReplaced)
{
    TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe",
                L"\\system32\\Ntoskrnl.exe");
}

TEST(PathResolution, WindDirReplaced)
{
    TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe",
                L"\\systemroot\\system32\\Ntoskrnl.exe");
}

TEST(PathResolution, DefaultKernelFoundOnPath)
{
    TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"ntoskrnl");
}

TEST(PathResolution, AddsExtension)
{
    TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe",
                L"C:\\Windows\\System32\\Ntoskrnl");
}

TEST(PathResolution, CombinePhaseOneAndTwo)
{
    TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe",
                L"\\globalroot\\System32\\Ntoskrnl");
}

TEST(PathResolution, RundllVundo)
{
    TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe",
                L"rundll32 ntoskrnl,ShellExecute");
}

TEST(PathResolution, RundllVundoSpaces)
{
    TestResolve(
        L"C:\\Windows\\System32\\Ntoskrnl.exe",
        L"rundll32                                                 ntoskrnl,ShellExecute");
}

TEST(PathResolution, QuotedPath)
{
    TestResolve(
        L"C:\\Program files\\Windows nt\\Accessories\\Wordpad.exe",
        L"\"C:\\Program Files\\Windows nt\\Accessories\\Wordpad.exe\"  Arguments Arguments Arguments");
}

TEST(PathResolution, QuotedPathRundll)
{
    HANDLE hFile = ::CreateFileW(L"C:\\ExampleTestingFile.exe",
                                 GENERIC_WRITE,
                                 0,
                                 0,
                                 CREATE_ALWAYS,
                                 FILE_FLAG_DELETE_ON_CLOSE,
                                 0);
    TestResolve(
        L"C:\\Exampletestingfile.exe",
        L"\"C:\\Windows\\System32\\Rundll32.exe\" \"C:\\ExampleTestingFile.exe,Argument arg arg\"");
    ::CloseHandle(hFile);
}

struct PathResolutionPathOrderFixture : public testing::Test
{
    std::vector<std::wstring> pathItems;
    std::wstring const fileName;
    PathResolutionPathOrderFixture()
        : fileName(L"ExampleFileDoesNotExistFindMeFindMeFindMeFindMe.found")
    {
    }
    virtual void SetUp()
    {
        using namespace std::placeholders;
        std::wstring pathBuffer;
        DWORD pathLen = ::GetEnvironmentVariableW(L"PATH", nullptr, 0);
        pathBuffer.resize(pathLen);
        ::GetEnvironmentVariable(L"PATH", &pathBuffer[0], pathLen);
        pathBuffer.pop_back(); // remove null
        boost::algorithm::split(pathItems,
                                pathBuffer,
                                std::bind1st(std::equal_to<wchar_t>(), L';'));
        ASSERT_LE(3ul, pathItems.size());
        std::transform(pathItems.begin(),
                       pathItems.end(),
                       pathItems.begin(),
                       [&](std::wstring & x) { return Append(x, fileName); });
        std::for_each(pathItems.begin(), pathItems.end(), [](std::wstring & a) {
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
    HANDLE hFile = ::CreateFileW(pathItems.back().c_str(),
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
    HANDLE hFile = ::CreateFileW(pathItems[1].c_str(),
                                 GENERIC_WRITE,
                                 0,
                                 0,
                                 CREATE_ALWAYS,
                                 FILE_FLAG_DELETE_ON_CLOSE,
                                 0);
    HANDLE hFile2 = ::CreateFileW(pathItems[2].c_str(),
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

struct PathResolutionPathExtOrderFixture : public testing::Test
{
    std::vector<std::wstring> pathItems;
    std::wstring const fileName;
    PathResolutionPathExtOrderFixture()
        : fileName(L"ExampleFileDoesNotExistFindMeFindMeFindMeFindMeExt")
    {
    }
    virtual void SetUp()
    {
        std::wstring pathBuffer;
        DWORD pathLen = ::GetEnvironmentVariableW(L"PATHEXT", nullptr, 0);
        pathBuffer.resize(pathLen);
        ::GetEnvironmentVariable(L"PATHEXT", &pathBuffer[0], pathLen);
        pathBuffer.pop_back(); // remove null
        boost::algorithm::split(pathItems,
                                pathBuffer,
                                std::bind1st(std::equal_to<wchar_t>(), L';'));
        ASSERT_LE(3u, pathItems.size());
        std::for_each(pathItems.begin(),
                      pathItems.end(),
                      [this](std::wstring & a) { a.insert(0, fileName); });
        std::transform(pathItems.begin(),
                       pathItems.end(),
                       pathItems.begin(),
                       [](std::wstring &
                          x) { return Append(L"C:\\Windows\\System32", x); });
        std::for_each(pathItems.begin(), pathItems.end(), [](std::wstring & a) {
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
    HANDLE hFile = ::CreateFileW(pathItems.back().c_str(),
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
    HANDLE hFile = ::CreateFileW(pathItems[1].c_str(),
                                 GENERIC_WRITE,
                                 0,
                                 0,
                                 CREATE_ALWAYS,
                                 FILE_FLAG_DELETE_ON_CLOSE,
                                 0);
    HANDLE hFile2 = ::CreateFileW(pathItems[2].c_str(),
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

struct PathClassTests : testing::Test
{
    path examplePath;
    virtual void SetUp()
    {
        examplePath = L"C:\\I am an example path.exe";
    }
};

TEST_F(PathClassTests, UppercaseSanity)
{
    std::wstring result(L"C:\\I AM AN EXAMPLE PATH.EXE");
    ASSERT_TRUE(
        std::equal(examplePath.ubegin(), examplePath.uend(), result.begin()));
}

TEST_F(PathClassTests, CanInsert)
{
    auto insertionLength = 233u;
    // Force reallocation
    ASSERT_GT(insertionLength, examplePath.capacity());
    std::wstring buffer;
    buffer.insert(buffer.begin(), insertionLength, L'a');
    examplePath.insert(examplePath.begin() + 3, buffer.begin(), buffer.end());
    buffer.insert(0, L"C:\\");
    buffer.append(L"I am an example path.exe");
    ASSERT_STREQ(buffer.c_str(), examplePath.c_str());
    boost::algorithm::to_upper(buffer);
    ASSERT_STREQ(buffer.c_str(), examplePath.uc_str());
}

TEST_F(PathClassTests, CanInsertNoRealloc)
{
    // Force reallocation
    examplePath.reserve(260);
    std::wstring buffer(L"aaaa");
    examplePath.insert(examplePath.begin() + 3, buffer.begin(), buffer.end());
    buffer.insert(0, L"C:\\");
    buffer.append(L"I am an example path.exe");
    ASSERT_STREQ(buffer.c_str(), examplePath.c_str());
    boost::algorithm::to_upper(buffer);
    ASSERT_STREQ(buffer.c_str(), examplePath.uc_str());
}

TEST_F(PathClassTests, CanInsertBorderCaseNoReallocate)
{
    auto insertionLength = examplePath.capacity() - examplePath.size();
    auto oldCapacity = examplePath.capacity();
    std::wstring buffer;
    buffer.insert(buffer.begin(), insertionLength, L'a');
    examplePath.insert(examplePath.begin() + 3, buffer.begin(), buffer.end());
    buffer.insert(0, L"C:\\");
    buffer.append(L"I am an example path.exe");
    ASSERT_STREQ(buffer.c_str(), examplePath.c_str());
    boost::algorithm::to_upper(buffer);
    ASSERT_STREQ(buffer.c_str(), examplePath.uc_str());
    ASSERT_EQ(examplePath.size(), examplePath.capacity());
    ASSERT_EQ(oldCapacity, examplePath.capacity());
}

TEST_F(PathClassTests, CanInsertNothing)
{
    // Insert zero length
    char* nullPtr = nullptr;
    examplePath.insert(examplePath.begin() + 3, nullPtr, nullPtr);
    std::wstring buffer(L"C:\\I am an example path.exe");
    ASSERT_STREQ(buffer.c_str(), examplePath.c_str());
    boost::algorithm::to_upper(buffer);
    ASSERT_STREQ(buffer.c_str(), examplePath.uc_str());
}
