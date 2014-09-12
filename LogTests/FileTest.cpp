// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include <windows.h>
#include <sddl.h>
#include <aclapi.h>
#include "gtest/gtest.h"
#include "../LogCommon/File.hpp"
#include "../LogCommon/Path.hpp"
#include "../LogCommon/Win32Exception.hpp"
#include "../LogCommon/Win32Glue.hpp"
#include "../LogCommon/Utf8.hpp"

using Instalog::SystemFacades::File;
using Instalog::SystemFacades::FindFiles;
using Instalog::SystemFacades::ErrorFileNotFoundException;
using Instalog::SystemFacades::ErrorPathNotFoundException;
using Instalog::SystemFacades::ErrorAccessDeniedException;

static std::string GenerateTestDirectory()
{
    std::wstring resultWide;
    resultWide.resize(MAX_PATH);
    resultWide.resize(::GetTempPathW(MAX_PATH, &resultWide[0]));
    std::string result = utf8::ToUtf8(resultWide);
    result = Instalog::Path::Append(std::move(result), "InstalogTesting");
    resultWide = utf8::ToUtf16(result);
    ::CreateDirectoryW(resultWide.c_str(), 0);
    result.push_back('\\');
    return result;
}

static std::string GetTestPath(std::string const& suffix)
{
    static auto startingDirectory = ::GenerateTestDirectory();
    auto result = startingDirectory;
    result.append(suffix);
    return result;
}

TEST(File, CanOpenDefault)
{
    ASSERT_THROW(File unitUnderTest(GetTestPath("CanOpenDefault.txt")),
                 ErrorFileNotFoundException);
}

TEST(File, CanOpenWithOtherOptions)
{
    File unitUnderTest(GetTestPath("CanOpenWithOtherOptions.txt"),
                       GENERIC_READ,
                       0,
                       0,
                       CREATE_NEW,
                       FILE_FLAG_DELETE_ON_CLOSE);
}

TEST(File, CloseActuallyCalled)
{
    {
        File unitUnderTest(GetTestPath("CloseActuallyCalled.txt"),
                           GENERIC_READ,
                           0,
                           0,
                           CREATE_NEW,
                           FILE_FLAG_DELETE_ON_CLOSE);
    }
    ASSERT_FALSE(File::Exists(GetTestPath("CloseActuallyCalled.txt")));
}

TEST(File, GetSizeHandle)
{
    std::vector<char> bytesToWrite;
    bytesToWrite.push_back('t');
    bytesToWrite.push_back('e');
    bytesToWrite.push_back('s');
    bytesToWrite.push_back('t');

    // Write to the file
    {
        File fileToWriteTo(GetTestPath("GetSizeHandle.txt"),
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           0,
                           CREATE_ALWAYS);
        fileToWriteTo.WriteBytes(bytesToWrite);
    }

    // Read from the file
    {
        File fileToReadFrom(GetTestPath("GetSizeHandle.txt"),
                            GENERIC_READ,
                            0,
                            0,
                            OPEN_EXISTING,
                            FILE_FLAG_DELETE_ON_CLOSE);
        EXPECT_EQ(4, fileToReadFrom.GetSize());
    }
}

TEST(File, GetAttributesHandle)
{
    // Write to the file
    {
        File fileToWriteTo(GetTestPath("GetAttributesHandle.txt"),
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           0,
                           CREATE_ALWAYS);
    }

    // Read from the file
    {
        File fileToReadFrom(GetTestPath("GetAttributesHandle.txt"),
                            GENERIC_READ,
                            0,
                            0,
                            OPEN_EXISTING,
                            FILE_FLAG_DELETE_ON_CLOSE);
        EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE, fileToReadFrom.GetAttributes());
    }
}

TEST(File, GetExtendedAttributes)
{
    // We just take an example file and make the assumption that if we are
    // consistent that's right.

    File explorer("C:\\Windows\\Explorer.exe",
                  FILE_READ_EA | FILE_READ_ATTRIBUTES,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  nullptr,
                  OPEN_EXISTING);
    BY_HANDLE_FILE_INFORMATION a = explorer.GetExtendedAttributes();

    WIN32_FILE_ATTRIBUTE_DATA b =
        File::GetExtendedAttributes("C:\\Windows\\Explorer.exe");

    EXPECT_EQ(a.dwFileAttributes, b.dwFileAttributes);
    EXPECT_EQ(a.ftCreationTime.dwLowDateTime, b.ftCreationTime.dwLowDateTime);
    EXPECT_EQ(a.ftCreationTime.dwHighDateTime, b.ftCreationTime.dwHighDateTime);
    EXPECT_EQ(a.ftLastAccessTime.dwLowDateTime,
              b.ftLastAccessTime.dwLowDateTime);
    EXPECT_EQ(a.ftLastAccessTime.dwHighDateTime,
              b.ftLastAccessTime.dwHighDateTime);
    EXPECT_EQ(a.ftLastWriteTime.dwLowDateTime, b.ftLastWriteTime.dwLowDateTime);
    EXPECT_EQ(a.ftLastWriteTime.dwHighDateTime,
              b.ftLastWriteTime.dwHighDateTime);
    EXPECT_EQ(a.nFileSizeHigh, b.nFileSizeHigh);
    EXPECT_EQ(a.nFileSizeLow, b.nFileSizeLow);
}

TEST(File, GetSizeNoHandle)
{
    std::vector<char> bytesToWrite;
    bytesToWrite.push_back('t');
    bytesToWrite.push_back('e');
    bytesToWrite.push_back('s');
    bytesToWrite.push_back('t');

    // Write to the file
    {
        File fileToWriteTo(GetTestPath("GetSizeNoHandle.txt"),
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           0,
                           CREATE_ALWAYS);
        fileToWriteTo.WriteBytes(bytesToWrite);
    }

    EXPECT_EQ(4, File::GetSize(GetTestPath("GetSizeNoHandle.txt")));

    File::Delete(GetTestPath("GetSizeNoHandle.txt"));
}

TEST(File, GetAttributesNoHandle)
{
    // Write to the file
    {
        File fileToWriteTo(GetTestPath("GetAttributesNoHandle.txt"),
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           0,
                           CREATE_ALWAYS);
    }

    EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE,
              File::GetAttributes(GetTestPath("GetAttributesNoHandle.txt")));

    File::Delete(GetTestPath("GetAttributesNoHandle.txt"));
}

TEST(File, CanReadBytes)
{
    File explorer("C:\\Windows\\Explorer.exe",
                  FILE_READ_DATA,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  nullptr,
                  OPEN_EXISTING);
    std::vector<char> bytes = explorer.ReadBytes(2);
    EXPECT_EQ('M', bytes[0]);
    EXPECT_EQ('Z', bytes[1]);
}

TEST(File, CanWriteBytes)
{
    std::vector<char> bytesToWrite;
    bytesToWrite.push_back('t');
    bytesToWrite.push_back('e');
    bytesToWrite.push_back('s');
    bytesToWrite.push_back('t');

    // Write to the file
    {
        File fileToWriteTo(GetTestPath("CanWriteBytes.txt"),
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           0,
                           CREATE_ALWAYS);
        EXPECT_TRUE(fileToWriteTo.WriteBytes(bytesToWrite));
    }

    // Read from the file
    {
        File fileToReadFrom(GetTestPath("CanWriteBytes.txt"),
                            GENERIC_READ,
                            0,
                            0,
                            OPEN_EXISTING,
                            FILE_FLAG_DELETE_ON_CLOSE);
        std::vector<char> bytesRead = fileToReadFrom.ReadBytes(4);
        EXPECT_EQ(bytesToWrite, bytesRead);
    }
}

TEST(File, CantWriteBytesToReadOnlyFile)
{
    File fileToWriteTo(GetTestPath("CantWriteBytesToReadOnlyFile.txt"),
                       GENERIC_READ,
                       0,
                       0,
                       CREATE_ALWAYS,
                       FILE_FLAG_DELETE_ON_CLOSE);

    std::vector<char> bytesToWrite;
    bytesToWrite.push_back('t');
    bytesToWrite.push_back('e');
    bytesToWrite.push_back('s');
    bytesToWrite.push_back('t');

    EXPECT_THROW(fileToWriteTo.WriteBytes(bytesToWrite),
                 ErrorAccessDeniedException);
}

TEST(File, CanDelete)
{
    {
        File unitUnderTest(GetTestPath("DeleteMe.txt"),
                           GENERIC_READ,
                           0,
                           0,
                           CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL);
    }
    File::Delete(GetTestPath("DeleteMe.txt"));
    ASSERT_FALSE(File::Exists(GetTestPath("DeleteMe.txt")));
}

TEST(File, DeleteChecksError)
{
    ASSERT_THROW(File::Delete(GetTestPath("IDoNotExist.txt")),
                 ErrorFileNotFoundException);
}

TEST(File, FileExists)
{
    ASSERT_TRUE(File::Exists("C:\\Windows\\Explorer.exe"));
}

TEST(File, DirectoryExists)
{
    ASSERT_TRUE(File::Exists("C:\\Windows"));
}

TEST(File, NotExists)
{
    ASSERT_FALSE(File::Exists(GetTestPath("I Do Not Exist")));
}

TEST(File, IsDirectoryExists)
{
    ASSERT_TRUE(File::IsDirectory("C:\\Windows"));
}

TEST(File, IsDirectoryNotExists)
{
    ASSERT_FALSE(File::IsDirectory(GetTestPath("I Do Not Exist")));
}

TEST(File, IsDirectoryFile)
{
    ASSERT_FALSE(File::IsDirectory("C:\\Windows\\Explorer.exe"));
}

TEST(File, IsExecutable)
{
    ASSERT_TRUE(File::IsExecutable("C:\\Windows\\Explorer.exe"));
}

TEST(File, IsExecutablNonExecutable)
{
    ASSERT_FALSE(
        File::IsExecutable("C:\\Windows\\System32\\drivers\\etc\\hosts"));
}

TEST(File, IsExecutableDirectory)
{
    ASSERT_FALSE(File::IsExecutable("C:\\Windows"));
}

TEST(File, GetsCompanyInformation)
{
    EXPECT_EQ("Microsoft Corporation",
              File::GetCompany("C:\\Windows\\Explorer.exe"));
}

TEST(File, ExtendedAttributesStaticFailsNonexistent)
{
    EXPECT_THROW(File::GetExtendedAttributes("C:\\Nonexistent\\Nonexistent"),
                 ErrorPathNotFoundException);
}

TEST(File, AttributesStaticFailsNonexistent)
{
    EXPECT_THROW(File::GetAttributes("C:\\Nonexistent\\Nonexistent"),
                 ErrorPathNotFoundException);
}

TEST(File, GetSizeStaticFailsNonexistent)
{
    EXPECT_THROW(File::GetSize("C:\\Nonexistent\\Nonexistent"),
                 ErrorPathNotFoundException);
}

TEST(File, FileIsDefaultConstructable)
{
    File f;
}

TEST(File, FileIsMoveConstructable)
{
    File f;
    File fTest(std::move(f));
}

TEST(File, FileIsMoveAssignable)
{
    File f;
    File fTest;
    fTest = std::move(f);
}

TEST(File, ExclusiveMatch)
{
    ASSERT_PRED1(File::IsExclusiveFile, "C:\\Windows\\Explorer.exe");
}

TEST(File, ExclusiveNoMatch)
{
    ASSERT_FALSE(
        File::IsExclusiveFile("C:\\Nonexistent\\Nonexistent\\Nonexistent"));
}

TEST(File, ExclusiveNoMatchDir)
{
    ASSERT_FALSE(File::IsExclusiveFile("C:\\Windows"));
}

using Instalog::Path::Append;

struct FindFileFixture : public ::testing::Test
{
    /*
    +---FindFilesTests
    |   +---Basic
    |   |   +---One
    |   |   |   \---Four
    |   |   |       \---Five
    |   |   +---Three
    |   |   \---Two
    |   |       \---Six
    |   \---Errors
    |       \---AccessDenied
    |           \---ShouldNotSee
    */

    std::string rootPath;
    std::string basicRootPath;
    std::string errorRootPath;

    std::string one;
    std::string two;
    std::string three;
    std::string four;
    std::string five;
    std::string six;

    std::string accessDenied;
    std::string shouldNotSee;
    std::string afterAccess;
    std::string afterAccess2;

    virtual void SetUp() override
    {
        // Figure out what the paths should be.
        rootPath = GetTestPath("FindFileTests");
        basicRootPath = Append(rootPath, "Basic");
        errorRootPath = Append(rootPath, "Errors");

        one = Append(basicRootPath, "One");
        two = Append(basicRootPath, "Two");
        three = Append(basicRootPath, "Three");
        four = Append(one, "Four");
        five = Append(four, "Five");
        six = Append(two, "Six");

        accessDenied = Append(errorRootPath, "AccessDenied");
        shouldNotSee = Append(accessDenied, "ShouldNotSee");
        afterAccess = Append(errorRootPath, "After");
        afterAccess2 = Append(afterAccess, "After2");

        // Create the security descriptor for the Access Denied directory
        // Grant WRITE_DAC to everyone, and nothing else
        // (Notably, no FILE_LIST_DIRECTORY)
        SECURITY_DESCRIPTOR sd;
        ::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        unsigned char aclBuffer[1024];
        unsigned char sidBuffer[SECURITY_MAX_SID_SIZE];
        PACL acl = reinterpret_cast<PACL>(&aclBuffer[0]);
        PSID sid = reinterpret_cast<PSID>(&sidBuffer[0]);
        DWORD sidLength = SECURITY_MAX_SID_SIZE;
        ::CreateWellKnownSid(WinWorldSid, nullptr, sid, &sidLength);
        ::InitializeAcl(acl, 1024, ACL_REVISION);
        ::AddAccessAllowedAceEx(acl,
                                ACL_REVISION,
                                OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
                                WRITE_DAC | FILE_ADD_SUBDIRECTORY,
                                sid);

        ::SetSecurityDescriptorDacl(&sd, TRUE, acl, FALSE);
        ::SetSecurityDescriptorControl(
            &sd, SE_DACL_PROTECTED, SE_DACL_PROTECTED);

        SECURITY_ATTRIBUTES sa = {};
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = &sd;

        ::CreateDirectoryW(utf8::ToUtf16(rootPath).c_str(), nullptr);
        ::CreateDirectoryW(utf8::ToUtf16(basicRootPath).c_str(), nullptr);
        ::CreateDirectoryW(utf8::ToUtf16(errorRootPath).c_str(), nullptr);

        ::CreateDirectoryW(utf8::ToUtf16(one).c_str(), nullptr);
        ::CreateDirectoryW(utf8::ToUtf16(two).c_str(), nullptr);
        ::CreateDirectoryW(utf8::ToUtf16(three).c_str(), nullptr);
        ::CreateDirectoryW(utf8::ToUtf16(four).c_str(), nullptr);
        ::CreateDirectoryW(utf8::ToUtf16(five).c_str(), nullptr);
        ::CreateDirectoryW(utf8::ToUtf16(six).c_str(), nullptr);

        ::CreateDirectoryW(utf8::ToUtf16(accessDenied).c_str(), &sa);
        ::CreateDirectoryW(utf8::ToUtf16(shouldNotSee).c_str(), nullptr);
        ::CreateDirectoryW(utf8::ToUtf16(afterAccess).c_str(), nullptr);
        ::CreateDirectoryW(utf8::ToUtf16(afterAccess2).c_str(), &sa);
    }

    virtual void TearDown() override
    {
        SECURITY_DESCRIPTOR sd;
        ::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        unsigned char aclBuffer[1024];
        unsigned char sidBuffer[SECURITY_MAX_SID_SIZE];
        PACL acl = reinterpret_cast<PACL>(&aclBuffer[0]);
        PSID sid = reinterpret_cast<PSID>(&sidBuffer[0]);
        DWORD sidLength = SECURITY_MAX_SID_SIZE;
        ::CreateWellKnownSid(WinWorldSid, nullptr, sid, &sidLength);
        ::InitializeAcl(acl, 1024, ACL_REVISION);
        ::AddAccessAllowedAce(acl, ACL_REVISION, FILE_ALL_ACCESS, sid);
        ::SetSecurityDescriptorDacl(&sd, TRUE, acl, FALSE);
        ::SetSecurityDescriptorControl(
            &sd, SE_DACL_PROTECTED, SE_DACL_PROTECTED);

        std::wstring accessDeniedWide(utf8::ToUtf16(accessDenied));
        ::SetNamedSecurityInfoW(&accessDeniedWide[0],
                                SE_FILE_OBJECT,
                                DACL_SECURITY_INFORMATION,
                                nullptr,
                                nullptr,
                                acl,
                                nullptr);

        std::wstring shouldNotSeeWide(utf8::ToUtf16(shouldNotSee));
        ::SetNamedSecurityInfoW(&shouldNotSeeWide[0],
                                SE_FILE_OBJECT,
                                DACL_SECURITY_INFORMATION,
                                nullptr,
                                nullptr,
                                acl,
                                nullptr);

        std::wstring afterAccess2Wide(utf8::ToUtf16(afterAccess2));
        ::SetNamedSecurityInfoW(&afterAccess2Wide[0],
                                SE_FILE_OBJECT,
                                DACL_SECURITY_INFORMATION,
                                nullptr,
                                nullptr,
                                acl,
                                nullptr);

        ::RemoveDirectoryW(afterAccess2Wide.c_str());
        ::RemoveDirectoryW(utf8::ToUtf16(afterAccess).c_str());
        ::RemoveDirectoryW(shouldNotSeeWide.c_str());
        ::RemoveDirectoryW(accessDeniedWide.c_str());

        ::RemoveDirectoryW(utf8::ToUtf16(six).c_str());
        ::RemoveDirectoryW(utf8::ToUtf16(five).c_str());
        ::RemoveDirectoryW(utf8::ToUtf16(four).c_str());
        ::RemoveDirectoryW(utf8::ToUtf16(three).c_str());
        ::RemoveDirectoryW(utf8::ToUtf16(two).c_str());
        ::RemoveDirectoryW(utf8::ToUtf16(one).c_str());

        ::RemoveDirectoryW(utf8::ToUtf16(errorRootPath).c_str());
        ::RemoveDirectoryW(utf8::ToUtf16(basicRootPath).c_str());
        ::RemoveDirectoryW(utf8::ToUtf16(rootPath).c_str());
    }
};

TEST_F(FindFileFixture, FindFilesBasic)
{
    Instalog::SystemFacades::FindFiles handle(Append(basicRootPath, "*"));

    char const* expectedResults[] = {"One", "Three", "Two", };

    for (std::size_t idx = 0; idx < _countof(expectedResults); ++idx)
    {
        EXPECT_TRUE(handle.Next());
        auto const expected = Append(basicRootPath, expectedResults[idx]);
        EXPECT_EQ(expected, handle.GetRecord().GetFileName());
    }

    EXPECT_FALSE(handle.Next());
    EXPECT_EQ(ERROR_NO_MORE_FILES, handle.LastError());
    EXPECT_FALSE(handle.Next());
    EXPECT_EQ(ERROR_NO_MORE_FILES, handle.LastError());
}

TEST_F(FindFileFixture, FindFilesWithDots)
{
    using Instalog::SystemFacades::FindFiles;
    using Instalog::SystemFacades::FindFilesOptions;

    Instalog::SystemFacades::FindFiles handle(
        Append(basicRootPath, "*"), FindFilesOptions::IncludeDotDirectories);

    char const* expectedResults[] = {
        ".", "..", "One", "Three", "Two", };

    for (std::size_t idx = 0; idx < _countof(expectedResults); ++idx)
    {
        EXPECT_TRUE(handle.Next());
        auto const expected = Append(basicRootPath, expectedResults[idx]);
        EXPECT_EQ(expected, handle.GetRecord().GetFileName());
    }

    EXPECT_FALSE(handle.Next());
    EXPECT_EQ(ERROR_NO_MORE_FILES, handle.LastError());
    EXPECT_FALSE(handle.Next());
    EXPECT_EQ(ERROR_NO_MORE_FILES, handle.LastError());
}

TEST_F(FindFileFixture, FindFilesFileNonexistent)
{
    Instalog::SystemFacades::FindFiles handle(
        Append(basicRootPath, "nonexistent"));
    EXPECT_THROW(handle.GetRecord(), std::logic_error);
    EXPECT_FALSE(handle.TryGetRecord().is_valid());
    EXPECT_TRUE(handle.Next());
    EXPECT_EQ(ERROR_FILE_NOT_FOUND, handle.LastError());
    EXPECT_THROW(handle.GetRecord(),
                 Instalog::SystemFacades::ErrorFileNotFoundException);
    EXPECT_FALSE(handle.Next());
    EXPECT_EQ(ERROR_NO_MORE_FILES, handle.LastError());
    EXPECT_THROW(handle.GetRecord(), Instalog::SystemFacades::Win32Exception);
}

TEST_F(FindFileFixture, FindFilesPathNonexistent)
{
    auto const pattern = Append(basicRootPath, "nonexistent\\nonexistent");
    Instalog::SystemFacades::FindFiles handle(pattern);
    EXPECT_FALSE(handle.TryGetRecord().is_valid());
    EXPECT_THROW(handle.GetRecord(), std::logic_error);
    EXPECT_TRUE(handle.Next());
    EXPECT_EQ(ERROR_PATH_NOT_FOUND, handle.LastError());
    EXPECT_THROW(handle.GetRecord(),
                 Instalog::SystemFacades::ErrorPathNotFoundException);
    EXPECT_FALSE(handle.Next());
    EXPECT_EQ(ERROR_NO_MORE_FILES, handle.LastError());
    EXPECT_THROW(handle.GetRecord(), Instalog::SystemFacades::Win32Exception);
}

TEST_F(FindFileFixture, FindFilesSingle)
{
    auto const pattern = Append(basicRootPath, "ON*");
    Instalog::SystemFacades::FindFiles handle(pattern);
    EXPECT_TRUE(handle.Next());
    auto const expected = Append(basicRootPath, "One");
    EXPECT_EQ(expected, handle.GetRecord().GetFileName());
}

TEST_F(FindFileFixture, FindFilesRecursiveWithDots)
{
    using Instalog::SystemFacades::FindFiles;
    using Instalog::SystemFacades::FindFilesOptions;

    Instalog::SystemFacades::FindFiles handle(
        Append(basicRootPath, "*"),
        FindFilesOptions::RecursiveSearch |
            FindFilesOptions::IncludeDotDirectories);

    char const* expectedResults[] = {
        ".",               "..",    "One", "One\\Four",
        "One\\Four\\Five", "Three", "Two", "Two\\Six"};

    for (std::size_t idx = 0; idx < _countof(expectedResults); ++idx)
    {
        EXPECT_TRUE(handle.Next());
        auto const expected = Append(basicRootPath, expectedResults[idx]);
        EXPECT_EQ(expected, handle.GetRecord().GetFileName());
    }

    EXPECT_FALSE(handle.Next());
    EXPECT_EQ(ERROR_NO_MORE_FILES, handle.LastError());
    EXPECT_FALSE(handle.Next());
    EXPECT_EQ(ERROR_NO_MORE_FILES, handle.LastError());
}

TEST_F(FindFileFixture, FindFilesRecursiveWithErrors)
{
    using Instalog::SystemFacades::FindFiles;
    using Instalog::SystemFacades::FindFilesOptions;

    Instalog::SystemFacades::FindFiles handle(
        Append(rootPath, "*"), FindFilesOptions::RecursiveSearch);

    char const* expectedResults[] = {
        "Basic",                "Basic\\One",
        "Basic\\One\\Four",     "Basic\\One\\Four\\Five",
        "Basic\\Three",         "Basic\\Two",
        "Basic\\Two\\Six",      "Errors",
        "Errors\\AccessDenied", "Errors\\After",
        "Errors\\After\\After2"};

    for (std::size_t idx = 0; idx < _countof(expectedResults); ++idx)
    {
        EXPECT_TRUE(handle.NextSuccess());
        auto const expected = Append(rootPath, expectedResults[idx]);
        EXPECT_EQ(expected, handle.GetRecord().GetFileName());
    }

    EXPECT_FALSE(handle.NextSuccess());
    EXPECT_EQ(ERROR_NO_MORE_FILES, handle.LastError());
    EXPECT_FALSE(handle.NextSuccess());
    EXPECT_EQ(ERROR_NO_MORE_FILES, handle.LastError());
}

TEST_F(FindFileFixture, FindFilesRecursiveWithErrorsErrorsChecked)
{
    using Instalog::SystemFacades::FindFiles;
    using Instalog::SystemFacades::FindFilesOptions;

    Instalog::SystemFacades::FindFiles handle(
        Append(rootPath, "*"), FindFilesOptions::RecursiveSearch);

    char const* expectedResults[] = {
        "Basic",                  "Basic\\One",   "Basic\\One\\Four",
        "Basic\\One\\Four\\Five", "Basic\\Three", "Basic\\Two",
        "Basic\\Two\\Six",        "Errors",       "Errors\\AccessDenied", };

    for (std::size_t idx = 0; idx < _countof(expectedResults); ++idx)
    {
        EXPECT_TRUE(handle.Next());
        auto const expected = Append(rootPath, expectedResults[idx]);
        EXPECT_EQ(expected, handle.GetRecord().GetFileName());
    }

    EXPECT_TRUE(handle.Next());
    EXPECT_EQ(ERROR_ACCESS_DENIED, handle.LastError());
    EXPECT_TRUE(handle.Next());
    EXPECT_EQ(ERROR_SUCCESS, handle.LastError());
    EXPECT_EQ(afterAccess, handle.GetRecord().GetFileName());
    EXPECT_TRUE(handle.Next());
    EXPECT_EQ(ERROR_SUCCESS, handle.LastError());
    EXPECT_EQ(afterAccess2, handle.GetRecord().GetFileName());
    EXPECT_TRUE(handle.Next());
    EXPECT_EQ(ERROR_ACCESS_DENIED, handle.LastError());
    EXPECT_FALSE(handle.Next());
    EXPECT_EQ(ERROR_NO_MORE_FILES, handle.LastError());
    EXPECT_FALSE(handle.Next());
    EXPECT_EQ(ERROR_NO_MORE_FILES, handle.LastError());
}

TEST_F(FindFileFixture, FindFilesRecursiveWithDotsWithErrors)
{
    using Instalog::SystemFacades::FindFiles;
    using Instalog::SystemFacades::FindFilesOptions;

    Instalog::SystemFacades::FindFiles handle(
        Append(rootPath, "*"),
        FindFilesOptions::RecursiveSearch |
            FindFilesOptions::IncludeDotDirectories);

    char const* expectedResults[] = {
        ".",                    "..",
        "Basic",                "Basic\\One",
        "Basic\\One\\Four",     "Basic\\One\\Four\\Five",
        "Basic\\Three",         "Basic\\Two",
        "Basic\\Two\\Six",      "Errors",
        "Errors\\AccessDenied", "Errors\\After",
        "Errors\\After\\After2"};

    for (std::size_t idx = 0; idx < _countof(expectedResults); ++idx)
    {
        EXPECT_TRUE(handle.NextSuccess());
        auto const expected = Append(rootPath, expectedResults[idx]);
        EXPECT_EQ(expected, handle.GetRecord().GetFileName());
    }

    EXPECT_FALSE(handle.NextSuccess());
    EXPECT_EQ(ERROR_NO_MORE_FILES, handle.LastError());
    EXPECT_FALSE(handle.NextSuccess());
    EXPECT_EQ(ERROR_NO_MORE_FILES, handle.LastError());
}
