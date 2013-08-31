// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <sddl.h>
#include <aclapi.h>
#include "gtest/gtest.h"
#include "../LogCommon/File.hpp"
#include "../LogCommon/Path.hpp"
#include "../LogCommon/Win32Exception.hpp"
#include "../LogCommon/Win32Glue.hpp"

using Instalog::SystemFacades::File;
using Instalog::SystemFacades::FindFiles;
using Instalog::SystemFacades::ErrorFileNotFoundException;
using Instalog::SystemFacades::ErrorPathNotFoundException;
using Instalog::SystemFacades::ErrorAccessDeniedException;

static std::wstring GenerateTestDirectory()
{
    std::wstring result;
    result.resize(MAX_PATH);
    result.resize(::GetTempPathW(MAX_PATH, &result[0]));
    result = Instalog::Path::Append(std::move(result), L"InstalogTesting");
    ::CreateDirectoryW(result.c_str(), 0);
    result.push_back(L'\\');
    return std::move(result);
}

static std::wstring GetTestPath(std::wstring const& suffix)
{
    static auto startingDirectory = ::GenerateTestDirectory();
    auto result = startingDirectory;
    result.append(suffix);
    return std::move(result);
}

TEST(File, CanOpenDefault)
{
    ASSERT_THROW(File unitUnderTest(GetTestPath(L"CanOpenDefault.txt")), ErrorFileNotFoundException);
}

TEST(File, CanOpenWithOtherOptions)
{
    File unitUnderTest(GetTestPath(L"CanOpenWithOtherOptions.txt"), GENERIC_READ, 0, 0, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE);
}

TEST(File, CloseActuallyCalled)
{
    {
        File unitUnderTest(GetTestPath(L"CloseActuallyCalled.txt"), GENERIC_READ, 0, 0, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE);
    }
    ASSERT_FALSE(File::Exists(GetTestPath(L"CloseActuallyCalled.txt")));
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
        File fileToWriteTo(GetTestPath(L"GetSizeHandle.txt"), GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS);
        fileToWriteTo.WriteBytes(bytesToWrite);
    }

    // Read from the file
    {
        File fileToReadFrom(GetTestPath(L"GetSizeHandle.txt"), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE);
        EXPECT_EQ(4, fileToReadFrom.GetSize());
    }
}

TEST(File, GetAttributesHandle)
{
    // Write to the file
    {
        File fileToWriteTo(GetTestPath(L"GetAttributesHandle.txt"), GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS);
    }

    // Read from the file
    {
        File fileToReadFrom(GetTestPath(L"GetAttributesHandle.txt"), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE);
        EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE, fileToReadFrom.GetAttributes());
    }
}

TEST(File, GetExtendedAttributes)
{
    // We just take an example file and make the assumption that if we are consistent that's right.
    
    File explorer(L"C:\\Windows\\Explorer.exe", FILE_READ_EA | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING);
    BY_HANDLE_FILE_INFORMATION a = explorer.GetExtendedAttributes();

    WIN32_FILE_ATTRIBUTE_DATA b = File::GetExtendedAttributes(L"C:\\Windows\\Explorer.exe");

    EXPECT_EQ(a.dwFileAttributes, b.dwFileAttributes);
    EXPECT_EQ(a.ftCreationTime.dwLowDateTime, b.ftCreationTime.dwLowDateTime);
    EXPECT_EQ(a.ftCreationTime.dwHighDateTime, b.ftCreationTime.dwHighDateTime);
    EXPECT_EQ(a.ftLastAccessTime.dwLowDateTime, b.ftLastAccessTime.dwLowDateTime);
    EXPECT_EQ(a.ftLastAccessTime.dwHighDateTime, b.ftLastAccessTime.dwHighDateTime);
    EXPECT_EQ(a.ftLastWriteTime.dwLowDateTime, b.ftLastWriteTime.dwLowDateTime);
    EXPECT_EQ(a.ftLastWriteTime.dwHighDateTime, b.ftLastWriteTime.dwHighDateTime);
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
        File fileToWriteTo(GetTestPath(L"GetSizeNoHandle.txt"), GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS);
        fileToWriteTo.WriteBytes(bytesToWrite);
    }

    EXPECT_EQ(4, File::GetSize(GetTestPath(L"GetSizeNoHandle.txt")));

    File::Delete(GetTestPath(L"GetSizeNoHandle.txt"));
}

TEST(File, GetAttributesNoHandle)
{
    // Write to the file
    {
        File fileToWriteTo(GetTestPath(L"GetAttributesNoHandle.txt"), GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS);
    }

    EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE, File::GetAttributes(GetTestPath(L"GetAttributesNoHandle.txt")));

    File::Delete(GetTestPath(L"GetAttributesNoHandle.txt"));
}

TEST(File, CanReadBytes)
{
    File explorer(L"C:\\Windows\\Explorer.exe", FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING);
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
        File fileToWriteTo(GetTestPath(L"CanWriteBytes.txt"), GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS);
        EXPECT_TRUE(fileToWriteTo.WriteBytes(bytesToWrite));
    }

    // Read from the file
    {
        File fileToReadFrom(GetTestPath(L"CanWriteBytes.txt"), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE);
        std::vector<char> bytesRead = fileToReadFrom.ReadBytes(4);
        EXPECT_EQ(bytesToWrite, bytesRead);
    }
}

TEST(File, CantWriteBytesToReadOnlyFile)
{
    File fileToWriteTo(GetTestPath(L"CantWriteBytesToReadOnlyFile.txt"), GENERIC_READ, 0, 0, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE);

    std::vector<char> bytesToWrite;
    bytesToWrite.push_back('t');
    bytesToWrite.push_back('e');
    bytesToWrite.push_back('s');
    bytesToWrite.push_back('t');

    EXPECT_THROW(fileToWriteTo.WriteBytes(bytesToWrite), ErrorAccessDeniedException);
}

TEST(File, CanDelete)
{
    {
        File unitUnderTest(GetTestPath(L"DeleteMe.txt"), GENERIC_READ, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL);
    }
    File::Delete(GetTestPath(L"DeleteMe.txt"));
    ASSERT_FALSE(File::Exists(GetTestPath(L"DeleteMe.txt")));
}

TEST(File, DeleteChecksError)
{
    ASSERT_THROW(File::Delete(GetTestPath(L"IDoNotExist.txt")), ErrorFileNotFoundException);
}

TEST(File, FileExists)
{
    ASSERT_TRUE(File::Exists(L"C:\\Windows\\Explorer.exe"));
}

TEST(File, DirectoryExists)
{
    ASSERT_TRUE(File::Exists(L"C:\\Windows"));
}

TEST(File, NotExists)
{
    ASSERT_FALSE(File::Exists(GetTestPath(L"I Do Not Exist")));
}

TEST(File, IsDirectoryExists)
{
    ASSERT_TRUE(File::IsDirectory(L"C:\\Windows"));
}

TEST(File, IsDirectoryNotExists)
{
    ASSERT_FALSE(File::IsDirectory(GetTestPath(L"I Do Not Exist")));
}

TEST(File, IsDirectoryFile)
{
    ASSERT_FALSE(File::IsDirectory(L"C:\\Windows\\Explorer.exe"));
}

TEST(File, IsExecutable)
{
    ASSERT_TRUE(File::IsExecutable(L"C:\\Windows\\Explorer.exe"));
}

TEST(File, IsExecutablNonExecutable)
{
    ASSERT_FALSE(File::IsExecutable(L"C:\\Windows\\System32\\drivers\\etc\\hosts"));
}

TEST(File, IsExecutableDirectory)
{
    ASSERT_FALSE(File::IsExecutable(L"C:\\Windows"));
}

TEST(File, GetsCompanyInformation)
{
    EXPECT_EQ(L"Microsoft Corporation", File::GetCompany(L"C:\\Windows\\Explorer.exe"));
}

TEST(File, ExtendedAttributesStaticFailsNonexistent)
{
    EXPECT_THROW(File::GetExtendedAttributes(L"C:\\Nonexistent\\Nonexistent"), ErrorPathNotFoundException);
}

TEST(File, AttributesStaticFailsNonexistent)
{
    EXPECT_THROW(File::GetAttributes(L"C:\\Nonexistent\\Nonexistent"), ErrorPathNotFoundException);
}

TEST(File, GetSizeStaticFailsNonexistent)
{
    EXPECT_THROW(File::GetSize(L"C:\\Nonexistent\\Nonexistent"), ErrorPathNotFoundException);
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
    ASSERT_PRED1(File::IsExclusiveFile, L"C:\\Windows\\Explorer.exe");
}

TEST(File, ExclusiveNoMatch)
{
    ASSERT_FALSE(File::IsExclusiveFile(L"C:\\Nonexistent\\Nonexistent\\Nonexistent"));
}

TEST(File, ExclusiveNoMatchDir)
{
    ASSERT_FALSE(File::IsExclusiveFile(L"C:\\Windows"));
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

    std::wstring rootPath;
    std::wstring basicRootPath;
    std::wstring errorRootPath;

    std::wstring one;
    std::wstring two;
    std::wstring three;
    std::wstring four;
    std::wstring five;
    std::wstring six;

    std::wstring accessDenied;
    std::wstring shouldNotSee;

    virtual void SetUp() override
    {
        // Figure out what the paths should be.
        rootPath = GetTestPath(L"FindFileTests");
        basicRootPath = Append(rootPath, L"Basic");
        errorRootPath = Append(rootPath, L"Errors");

        one = Append(basicRootPath, L"One");
        two = Append(basicRootPath, L"Two");
        three = Append(basicRootPath, L"Three");
        four = Append(one, L"Four");
        five = Append(four, L"Five");
        six = Append(two, L"Six");

        accessDenied = Append(errorRootPath, L"AccessDenied");
        shouldNotSee = Append(accessDenied, L"ShouldNotSee");

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
        ::AddAccessAllowedAceEx(
            acl,
            ACL_REVISION,
            OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
            WRITE_DAC | FILE_ADD_SUBDIRECTORY,
            sid
            );

        ::SetSecurityDescriptorDacl(&sd, TRUE, acl, FALSE);
        ::SetSecurityDescriptorControl(&sd, SE_DACL_PROTECTED, SE_DACL_PROTECTED);

        SECURITY_ATTRIBUTES sa = {};
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = &sd;

        ::CreateDirectoryW(rootPath.c_str(), nullptr);
        ::CreateDirectoryW(basicRootPath.c_str(), nullptr);
        ::CreateDirectoryW(errorRootPath.c_str(), nullptr);

        ::CreateDirectoryW(one.c_str(), nullptr);
        ::CreateDirectoryW(two.c_str(), nullptr);
        ::CreateDirectoryW(three.c_str(), nullptr);
        ::CreateDirectoryW(four.c_str(), nullptr);
        ::CreateDirectoryW(five.c_str(), nullptr);
        ::CreateDirectoryW(six.c_str(), nullptr);

        ::CreateDirectoryW(accessDenied.c_str(), &sa);
        ::CreateDirectoryW(shouldNotSee.c_str(), nullptr);
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
        ::SetSecurityDescriptorControl(&sd, SE_DACL_PROTECTED, SE_DACL_PROTECTED);

        ::SetNamedSecurityInfoW(
            &accessDenied[0],
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            nullptr,
            nullptr,
            acl,
            nullptr
            );

        ::SetNamedSecurityInfoW(
            &shouldNotSee[0],
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            nullptr,
            nullptr,
            acl,
            nullptr
            );

        ::RemoveDirectoryW(shouldNotSee.c_str());
        ::RemoveDirectoryW(accessDenied.c_str());

        ::RemoveDirectoryW(six.c_str());
        ::RemoveDirectoryW(five.c_str());
        ::RemoveDirectoryW(four.c_str());
        ::RemoveDirectoryW(three.c_str());
        ::RemoveDirectoryW(two.c_str());
        ::RemoveDirectoryW(one.c_str());

        ::RemoveDirectoryW(errorRootPath.c_str());
        ::RemoveDirectoryW(basicRootPath.c_str());
        ::RemoveDirectoryW(rootPath.c_str());
    }
};

TEST_F(FindFileFixture, FindFilesBasic)
{
    Instalog::SystemFacades::FindFiles handle(Append(basicRootPath, L"*"));

    wchar_t const* expectedResults[] = 
    {
        L"One",
        L"Three",
        L"Two",
    };

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

    Instalog::SystemFacades::FindFiles handle(Append(basicRootPath, L"*"), FindFilesOptions::IncludeDotDirectories);

    wchar_t const* expectedResults[] = 
    {
        L".",
        L"..",
        L"One",
        L"Three",
        L"Two",
    };

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
    Instalog::SystemFacades::FindFiles handle(Append(basicRootPath, L"nonexistent"));
    EXPECT_FALSE(handle.Next());
    EXPECT_EQ(ERROR_FILE_NOT_FOUND, handle.LastError());
}

TEST_F(FindFileFixture, FindFilesPathNonexistent)
{
    auto const pattern = Append(basicRootPath, L"nonexistent\\nonexistent");
    Instalog::SystemFacades::FindFiles handle(pattern);
    EXPECT_FALSE(handle.Next());
    EXPECT_EQ(ERROR_PATH_NOT_FOUND, handle.LastError());
}

TEST_F(FindFileFixture, FindFilesSingle)
{
    auto const pattern = Append(basicRootPath, L"ON*");
    Instalog::SystemFacades::FindFiles handle(pattern);
    EXPECT_TRUE(handle.Next());
    auto const expected = Append(basicRootPath, L"One");
    EXPECT_EQ(expected, handle.GetRecord().GetFileName());
}

TEST_F(FindFileFixture, FindFilesRecursive)
{
    using Instalog::SystemFacades::FindFiles;
    using Instalog::SystemFacades::FindFilesOptions;

    Instalog::SystemFacades::FindFiles handle(Append(basicRootPath, L"*"), FindFilesOptions::RecursiveSearch);

    wchar_t const* expectedResults[] = 
    {
        L"One",
        L"One\\Four",
        L"One\\Four\\Five",
        L"Three",
        L"Two",
        L"Two\\Six"
    };

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
