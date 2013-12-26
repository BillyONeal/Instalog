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
using Instalog::path;

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
    std::wstring dir1W;
    std::wstring dir2W;
    std::wstring pathBuffer;
    std::vector<std::string> pathItems;
    std::vector<std::string> pathExtItems;
    std::string const fileName;
    PathResolutionPathOrderFixture()
        : fileName("ExampleFileDoesNotExistFindMeFindMeFindMeFindMe.found")
    {
    }

    virtual void SetUp()
    {
        using namespace std::placeholders;
        DWORD pathLen = ::GetEnvironmentVariableW(L"PATH", nullptr, 0);
        pathBuffer.resize(pathLen);
        ::GetEnvironmentVariableW(L"PATH", &pathBuffer[0], pathLen);
        pathBuffer.pop_back(); // remove null

        std::string dir1 = Instalog::Path::Append(GetTestBinaryDir(), "Directory1");
        std::string dir2 = Instalog::Path::Append(GetTestBinaryDir(), "Directory2");
        dir1W = utf8::ToUtf16(dir1);
        dir2W = utf8::ToUtf16(dir2);
        std::wstring setPath = dir1W + L";" + dir2W;
        ::SetEnvironmentVariableW(L"PATH", setPath.c_str());
        pathItems.emplace_back(Instalog::Path::Append(dir1, fileName));
        pathItems.emplace_back(Instalog::Path::Append(dir2, fileName));
        ::CreateDirectoryW(dir1W.c_str(), nullptr);
        ::CreateDirectoryW(dir2W.c_str(), nullptr);

        std::wstring pathExt;
        pathLen = ::GetEnvironmentVariableW(L"PATHEXT", nullptr, 0);
        pathExt.resize(pathLen);
        ::GetEnvironmentVariableW(L"PATHEXT", &pathExt[0], pathLen);
        pathExt.pop_back(); // remove null
        std::vector<std::wstring> pathExtWide;
        boost::algorithm::split(pathExtWide,
            pathExt,
            std::bind1st(std::equal_to<wchar_t>(), L';'));
        ASSERT_LE(3u, pathExtWide.size());
        pathExtItems.emplace_back(Instalog::Path::Append(dir1, "ExampleFileDoesNotExistFindMeFindMeFindMeFindMe" + utf8::ToUtf8(pathExtWide[0])));
        pathExtItems.emplace_back(Instalog::Path::Append(dir1, "ExampleFileDoesNotExistFindMeFindMeFindMeFindMe" + utf8::ToUtf8(pathExtWide[1])));
    }

    virtual void TearDown()
    {
        ::RemoveDirectoryW(dir1W.c_str());
        ::RemoveDirectoryW(dir2W.c_str());
        ::SetEnvironmentVariableW(L"PATH", pathBuffer.c_str());
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

TEST_F(PathResolutionPathOrderFixture, SeesLastPathExtItem)
{
    HANDLE hFile = ::CreateFileW(utf8::ToUtf16(pathExtItems.back()).c_str(),
                                 GENERIC_WRITE,
                                 0,
                                 0,
                                 CREATE_ALWAYS,
                                 FILE_FLAG_DELETE_ON_CLOSE,
                                 0);
    TestResolve(pathExtItems.back(), "ExampleFileDoesNotExistFindMeFindMeFindMeFindMe");
    ::CloseHandle(hFile);
}

TEST_F(PathResolutionPathOrderFixture, RespectsPathExtOrder)
{
    HANDLE hFile = ::CreateFileW(utf8::ToUtf16(pathExtItems[0]).c_str(),
                                 GENERIC_WRITE,
                                 0,
                                 0,
                                 CREATE_ALWAYS,
                                 FILE_FLAG_DELETE_ON_CLOSE,
                                 0);
    HANDLE hFile2 = ::CreateFileW(utf8::ToUtf16(pathExtItems[1]).c_str(),
                                  GENERIC_WRITE,
                                  0,
                                  0,
                                  CREATE_ALWAYS,
                                  FILE_FLAG_DELETE_ON_CLOSE,
                                  0);
    TestResolve(pathExtItems[0], "ExampleFileDoesNotExistFindMeFindMeFindMeFindMe");
    ::CloseHandle(hFile);
    ::CloseHandle(hFile2);
}

// These tests use U+00A9 COPYRIGHT SIGN as an example character not requiring UTF-16 surrogates
#define UNICODE_CHARACTER   "\xC2\xA9"
#define UNICODE_CHARACTERW L"\x00A9"

// These tests use U+1F4A9 PILE OF POO as an example character requiring UTF-16 surrogates
#define SURROGATE_CHARACTER   "\xF0\x9F\x92\xA9"
#define SURROGATE_CHARACTERW L"\xD83D\xDCA9"

static char const examplePath[] = "Example i " SURROGATE_CHARACTER " path " UNICODE_CHARACTER;
static wchar_t const exampleWidePath[] = L"Example i " SURROGATE_CHARACTERW L" path " UNICODE_CHARACTERW;
static char const* examplePathUpper = "EXAMPLE I " SURROGATE_CHARACTER " PATH " UNICODE_CHARACTER;
static wchar_t const* exampleWidePathUpper = L"EXAMPLE I " SURROGATE_CHARACTERW L" PATH " UNICODE_CHARACTERW;

static void test_path_class_unicode_path(path const& p)
{
    // -1 for the null
    EXPECT_EQ(_countof(exampleWidePath) - 1, p.size());
    EXPECT_EQ(_countof(exampleWidePath) - 1, p.capacity());
    EXPECT_EQ(32767, p.max_size());

    EXPECT_STREQ(exampleWidePath, p.get());
    EXPECT_EQ(static_cast<std::string>(examplePath), p.to_string());
    EXPECT_EQ(static_cast<std::wstring>(exampleWidePath), p.to_wstring());

    EXPECT_STREQ(exampleWidePathUpper, p.get_upper());
    EXPECT_EQ(static_cast<std::string>(examplePathUpper), p.to_upper_string());
    EXPECT_EQ(static_cast<std::wstring>(exampleWidePathUpper), p.to_upper_wstring());
}

TEST(PathClass, ConstructUnicodeFromCharStar)
{
    path p(examplePath);
    test_path_class_unicode_path(p);
}

TEST(PathClass, ConstructUnicodeFromWcharTStar)
{
    path p(exampleWidePath);
    test_path_class_unicode_path(p);
}

TEST(PathClass, ConstructUnicodeFromString)
{
    path p(static_cast<std::string>(examplePath));
    test_path_class_unicode_path(p);
}

TEST(PathClass, ConstructUnicodeFromWstring)
{
    path p(static_cast<std::wstring>(exampleWidePath));
    test_path_class_unicode_path(p);
}

static void test_empty_path(path const& p)
{
    EXPECT_EQ(0u, p.size());
    EXPECT_EQ(0u, p.capacity());
    EXPECT_NE(nullptr, p.get());
    EXPECT_NE(nullptr, p.get_upper());
}

TEST(PathClass, DefaultConstructionIsEmpty)
{
    path p;
    test_empty_path(p);
}

TEST(PathClass, NullptrConstructionIsEmpty)
{
    path p(nullptr);
    test_empty_path(p);
}

static void test_equal_paths(path const& expected, path const& actual)
{
    EXPECT_EQ(expected.size(), actual.size());
    EXPECT_STREQ(expected.get(), actual.get());
    EXPECT_STREQ(expected.get_upper(), actual.get_upper());
    EXPECT_EQ(expected, actual);
}

static void test_path_is_example(path const& actual)
{
    EXPECT_EQ(7, actual.size());
    EXPECT_STREQ(L"Example", actual.get());
    EXPECT_STREQ(L"EXAMPLE", actual.get_upper());
}

TEST(PathClass, CopyConstructor)
{
    path start("Example");
    path copy(start);
    test_equal_paths(start, copy);
}

TEST(PathClass, MoveConstructor)
{
    path start("Example");
    path moved(std::move(start));
    test_empty_path(start);
    test_path_is_example(moved);
}

TEST(PathClass, CopyAssignment)
{
    path start("Example");
    path copy;
    copy = start;
    test_equal_paths(start, copy);
}

TEST(PathClass, CopyAssignmentToLonger)
{
    path start("Example");
    path copy("This string is really really long.");
    auto const cap = copy.capacity();
    copy = start;
    test_equal_paths(start, copy);
    EXPECT_EQ(cap, copy.capacity());
}

TEST(PathClass, CopyAssignmentToSelf)
{
    path start("Example");
    start = start;
    test_path_is_example(start);
}

TEST(PathClass, MoveAssignment)
{
    path start("Example");
    path moved;
    moved = std::move(start);
    test_empty_path(start);
    test_path_is_example(moved);
}

TEST(PathClass, MoveAssignmentToSelf)
{
    path start("Example");
    start = std::move(start);
    test_path_is_example(start);
}

TEST(PathClass, SwapMember)
{
    path left("Right path");
    path right("Left path");
    left.swap(right);
    EXPECT_STREQ(L"Left path", left.get());
    EXPECT_EQ(9, left.size());
    EXPECT_EQ(9, left.capacity());
    EXPECT_STREQ(L"Right path", right.get());
    EXPECT_EQ(10, right.size());
    EXPECT_EQ(10, right.capacity());
}

TEST(PathClass, SwapNoneMember)
{
    path left("Right path");
    path right("Left path");
    swap(left, right);
    EXPECT_STREQ(L"Left path", left.get());
    EXPECT_EQ(9, left.size());
    EXPECT_EQ(9, left.capacity());
    EXPECT_STREQ(L"Right path", right.get());
    EXPECT_EQ(10, right.size());
    EXPECT_EQ(10, right.capacity());
}

TEST(PathClass, Equals)
{
    path left("Example");
    path right("ExAmPlE");
    EXPECT_TRUE(left == right);
}

TEST(PathClass, NotEquals)
{
    path left("Example");
    path right("ExAmPlE");
    EXPECT_FALSE(left != right);
}

TEST(PathClass, Less)
{
    path empty;
    path apple("Apple");
    path bear("Bear");
    path cat("Cat");
    EXPECT_TRUE(empty < apple);
    EXPECT_TRUE(empty < bear);
    EXPECT_TRUE(empty < cat);
    EXPECT_TRUE(apple < bear);
    EXPECT_TRUE(apple < cat);
    EXPECT_TRUE(bear < cat);

    EXPECT_FALSE(cat < empty);
    EXPECT_FALSE(cat < apple);
    EXPECT_FALSE(cat < bear);
    EXPECT_FALSE(bear < empty);
    EXPECT_FALSE(bear < apple);
    EXPECT_FALSE(apple < empty);

    EXPECT_FALSE(empty < empty);
    EXPECT_FALSE(cat < cat);
}

TEST(PathClass, Greater)
{
    path empty;
    path apple("Apple");
    path bear("Bear");
    path cat("Cat");

    EXPECT_TRUE(cat > empty);
    EXPECT_TRUE(cat > apple);
    EXPECT_TRUE(cat > bear);
    EXPECT_TRUE(bear > empty);
    EXPECT_TRUE(bear > apple);
    EXPECT_TRUE(apple > empty);

    EXPECT_FALSE(empty > apple);
    EXPECT_FALSE(empty > bear);
    EXPECT_FALSE(empty > cat);
    EXPECT_FALSE(apple > bear);
    EXPECT_FALSE(apple > cat);
    EXPECT_FALSE(bear > cat);

    EXPECT_FALSE(empty > empty);
    EXPECT_FALSE(cat > cat);
}

TEST(PathClass, LessEqual)
{
    path empty;
    path apple("Apple");
    path bear("Bear");
    path cat("Cat");
    EXPECT_TRUE(empty <= apple);
    EXPECT_TRUE(empty <= bear);
    EXPECT_TRUE(empty <= cat);
    EXPECT_TRUE(apple <= bear);
    EXPECT_TRUE(apple <= cat);
    EXPECT_TRUE(bear <= cat);

    EXPECT_FALSE(cat <= empty);
    EXPECT_FALSE(cat <= apple);
    EXPECT_FALSE(cat <= bear);
    EXPECT_FALSE(bear <= empty);
    EXPECT_FALSE(bear <= apple);
    EXPECT_FALSE(apple <= empty);

    EXPECT_TRUE(empty <= empty);
    EXPECT_TRUE(cat <= cat);
}

TEST(PathClass, GreaterEqual)
{
    path empty;
    path apple("Apple");
    path bear("Bear");
    path cat("Cat");

    EXPECT_TRUE(cat >= empty);
    EXPECT_TRUE(cat >= apple);
    EXPECT_TRUE(cat >= bear);
    EXPECT_TRUE(bear >= empty);
    EXPECT_TRUE(bear >= apple);
    EXPECT_TRUE(apple >= empty);

    EXPECT_FALSE(empty >= apple);
    EXPECT_FALSE(empty >= bear);
    EXPECT_FALSE(empty >= cat);
    EXPECT_FALSE(apple >= bear);
    EXPECT_FALSE(apple >= cat);
    EXPECT_FALSE(bear >= cat);

    EXPECT_TRUE(empty >= empty);
    EXPECT_TRUE(cat >= cat);
}

TEST(PathClass, PathClear)
{
    path filled(L"data here");
    EXPECT_EQ(9, filled.capacity());
    filled.clear();
    EXPECT_EQ(9, filled.capacity());
    EXPECT_TRUE(filled.empty());
    EXPECT_STREQ(L"", filled.get());
}

TEST(PathClass, InsertGrow)
{
    path filled(L"start end");
    filled.insert(6, L"middle ");
    EXPECT_STREQ(L"start middle end", filled.get());
    EXPECT_STREQ(L"start middle end", filled.to_wstring().c_str());
    EXPECT_STREQ(L"START MIDDLE END", filled.get_upper());
    EXPECT_STREQ(L"START MIDDLE END", filled.to_upper_wstring().c_str());
}

TEST(PathClass, InsertNoGrow)
{
    path filled(L"this is a path with data which makes it really long");
    path smallData("start end");
    filled = smallData;
    filled.insert(6, L"example");
    filled.insert(6, L"middle ");
    EXPECT_STREQ(L"start middle exampleend", filled.get());
    EXPECT_STREQ(L"start middle exampleend", filled.to_wstring().c_str());
    EXPECT_STREQ(L"START MIDDLE EXAMPLEEND", filled.get_upper());
    EXPECT_STREQ(L"START MIDDLE EXAMPLEEND", filled.to_upper_wstring().c_str());
}

TEST(PathClass, InsertGrowStr)
{
    path filled(L"start end");
    filled.insert(6, static_cast<std::wstring>(L"middle "));
    EXPECT_STREQ(L"start middle end", filled.get());
    EXPECT_STREQ(L"start middle end", filled.to_wstring().c_str());
    EXPECT_STREQ(L"START MIDDLE END", filled.get_upper());
    EXPECT_STREQ(L"START MIDDLE END", filled.to_upper_wstring().c_str());
}

TEST(PathClass, InsertNoGrowStr)
{
    path filled(L"this is a path with data which makes it really long");
    path smallData("start end");
    filled = smallData;
    filled.insert(6, static_cast<std::wstring>(L"example"));
    filled.insert(6, static_cast<std::wstring>(L"middle "));
    EXPECT_STREQ(L"start middle exampleend", filled.get());
    EXPECT_STREQ(L"start middle exampleend", filled.to_wstring().c_str());
    EXPECT_STREQ(L"START MIDDLE EXAMPLEEND", filled.get_upper());
    EXPECT_STREQ(L"START MIDDLE EXAMPLEEND", filled.to_upper_wstring().c_str());
}

TEST(PathClass, InsertGrowLen)
{
    path filled(L"start end");
    filled.insert(6, L"middle ", 3);
    EXPECT_STREQ(L"start midend", filled.get());
    EXPECT_STREQ(L"start midend", filled.to_wstring().c_str());
    EXPECT_STREQ(L"START MIDEND", filled.get_upper());
    EXPECT_STREQ(L"START MIDEND", filled.to_upper_wstring().c_str());
}

TEST(PathClass, InsertNoGrowLen)
{
    path filled(L"this is a path with data which makes it really long");
    path smallData("start end");
    filled = smallData;
    filled.insert(6, L"example", 3);
    filled.insert(6, L"middle ", 3);
    EXPECT_STREQ(L"start midexaend", filled.get());
    EXPECT_STREQ(L"start midexaend", filled.to_wstring().c_str());
    EXPECT_STREQ(L"START MIDEXAEND", filled.get_upper());
    EXPECT_STREQ(L"START MIDEXAEND", filled.to_upper_wstring().c_str());
}

TEST(PathClass, OverlappingRegions)
{
    path filled(L"start end");
    filled.insert(5, filled.get() + 5);
    EXPECT_STREQ(L"start end end", filled.get());
    EXPECT_STREQ(L"start end end", filled.to_wstring().c_str());
    EXPECT_STREQ(L"START END END", filled.get_upper());
    EXPECT_STREQ(L"START END END", filled.to_upper_wstring().c_str());
}

TEST(PathClass, WstringInsert)
{
    path p;
    std::wstring example(L"example");
    p.insert(0, example);
    EXPECT_STREQ(L"example", p.get());
}

TEST(PathClass, LengthBufferInsert)
{
    path p;
    p.insert(0, L"this is new content", 4);
    EXPECT_STREQ(L"this", p.get());
}
