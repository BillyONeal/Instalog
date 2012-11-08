// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include "../LogCommon/File.hpp"
#include "../LogCommon/Path.hpp"
#include "../LogCommon/Win32Exception.hpp"

using Instalog::SystemFacades::File;
using Instalog::SystemFacades::FindFiles;
using Instalog::SystemFacades::ErrorFileNotFoundException;
using Instalog::SystemFacades::ErrorPathNotFoundException;
using Instalog::SystemFacades::ErrorAccessDeniedException;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_CLASS(FileTests)
{
public:
    TEST_METHOD(FileCanOpenDefault)
    {
        Assert::ExpectException<ErrorFileNotFoundException>([] { File unitUnderTest(L"./CanOpenDefault.txt"); });
    }

    TEST_METHOD(FileCanOpenWithOtherOptions)
    {
        File unitUnderTest(L"./CanOpenWithOtherOptions.txt", GENERIC_READ, 0, 0, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE);
    }

    TEST_METHOD(FileCloseActuallyCalled)
    {
        {
            File unitUnderTest(L"./CloseActuallyCalled.txt", GENERIC_READ, 0, 0, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE);
        }
        Assert::IsFalse(File::Exists(L"./CloseActuallyCalled.txt"));
    }

    TEST_METHOD(FileGetSizeHandle)
    {
        std::vector<char> bytesToWrite;
        bytesToWrite.push_back('t');
        bytesToWrite.push_back('e');
        bytesToWrite.push_back('s');
        bytesToWrite.push_back('t');

        // Write to the file
        {
            File fileToWriteTo(L"./GetSizeHandle.txt", GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS);
            fileToWriteTo.WriteBytes(bytesToWrite);
        }

        // Read from the file
        {
            File fileToReadFrom(L"./GetSizeHandle.txt", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE);
            Assert::AreEqual(4ull, fileToReadFrom.GetSize());
        }
    }

    TEST_METHOD(FileGetAttributesHandle)
    {
        // Write to the file
        {
            File fileToWriteTo(L"./GetAttributesHandle.txt", GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS);
        }

        // Read from the file
        {
            File fileToReadFrom(L"./GetAttributesHandle.txt", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE);
            Assert::AreEqual<int>(FILE_ATTRIBUTE_ARCHIVE, fileToReadFrom.GetAttributes());
        }
    }

    TEST_METHOD(FileGetExtendedAttributes)
    {
        File explorer(L"C:\\Windows\\Explorer.exe");
        BY_HANDLE_FILE_INFORMATION a = explorer.GetExtendedAttributes();

        WIN32_FILE_ATTRIBUTE_DATA b = File::GetExtendedAttributes(L"C:\\Windows\\Explorer.exe");

        Assert::AreEqual(a.dwFileAttributes, b.dwFileAttributes);
        Assert::AreEqual(a.ftCreationTime.dwLowDateTime, b.ftCreationTime.dwLowDateTime);
        Assert::AreEqual(a.ftCreationTime.dwHighDateTime, b.ftCreationTime.dwHighDateTime);
        Assert::AreEqual(a.ftLastAccessTime.dwLowDateTime, b.ftLastAccessTime.dwLowDateTime);
        Assert::AreEqual(a.ftLastAccessTime.dwHighDateTime, b.ftLastAccessTime.dwHighDateTime);
        Assert::AreEqual(a.ftLastWriteTime.dwLowDateTime, b.ftLastWriteTime.dwLowDateTime);
        Assert::AreEqual(a.ftLastWriteTime.dwHighDateTime, b.ftLastWriteTime.dwHighDateTime);
        Assert::AreEqual(a.nFileSizeHigh, b.nFileSizeHigh);
        Assert::AreEqual(a.nFileSizeLow, b.nFileSizeLow);
    }

    TEST_METHOD(FileGetSizeNoHandle)
    {
        std::vector<char> bytesToWrite;
        bytesToWrite.push_back('t');
        bytesToWrite.push_back('e');
        bytesToWrite.push_back('s');
        bytesToWrite.push_back('t');

        // Write to the file
        {
            File fileToWriteTo(L"./GetSizeNoHandle.txt", GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS);
            fileToWriteTo.WriteBytes(bytesToWrite);
        }

        Assert::AreEqual(4ull, File::GetSize(L"./GetSizeNoHandle.txt"));

        File::Delete(L"GetSizeNoHandle.txt");
    }

    TEST_METHOD(FileGetAttributesNoHandle)
    {
        // Write to the file
        {
            File fileToWriteTo(L"./GetAttributesNoHandle.txt", GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS);
        }

        Assert::AreEqual<int>(FILE_ATTRIBUTE_ARCHIVE, File::GetAttributes(L"./GetAttributesNoHandle.txt"));

        File::Delete(L"./GetAttributesNoHandle.txt");
    }

    TEST_METHOD(FileCanReadBytes)
    {
        File explorer(L"C:\\Windows\\Explorer.exe");
        std::vector<char> bytes = explorer.ReadBytes(2);
        Assert::AreEqual('M', bytes[0]);
        Assert::AreEqual('Z', bytes[1]);
    }

    TEST_METHOD(FileCanWriteBytes)
    {
        std::vector<char> bytesToWrite;
        bytesToWrite.push_back('t');
        bytesToWrite.push_back('e');
        bytesToWrite.push_back('s');
        bytesToWrite.push_back('t');

        // Write to the file
        {
            File fileToWriteTo(L"./CanWriteBytes.txt", GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS);
            Assert::IsTrue(fileToWriteTo.WriteBytes(bytesToWrite));
        }

        // Read from the file
        {
            File fileToReadFrom(L"./CanWriteBytes.txt", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE);
            std::vector<char> bytesRead = fileToReadFrom.ReadBytes(4);
            Assert::IsTrue(bytesToWrite == bytesRead);
        }
    }

    TEST_METHOD(FileCantWriteBytesToReadOnlyFile)
    {
        File fileToWriteTo(L"./CantWriteBytesToReadOnlyFile.txt", GENERIC_READ, 0, 0, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE);

        std::vector<char> bytesToWrite;
        bytesToWrite.push_back('t');
        bytesToWrite.push_back('e');
        bytesToWrite.push_back('s');
        bytesToWrite.push_back('t');

        Assert::ExpectException<ErrorAccessDeniedException>([&] { fileToWriteTo.WriteBytes(bytesToWrite); });
    }

    TEST_METHOD(FileCanDelete)
    {
        {
            File unitUnderTest(L"./DeleteMe.txt", GENERIC_READ, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL);
        }
        File::Delete(L"./DeleteMe.txt");
        Assert::IsFalse(File::Exists(L"./DeleteMe.txt"));
    }

    TEST_METHOD(FileDeleteChecksError)
    {
        Assert::ExpectException<ErrorFileNotFoundException>([] { File::Delete(L"./IDoNotExist.txt"); });
    }

    TEST_METHOD(FileFileExists)
    {
        Assert::IsTrue(File::Exists(L"C:\\Windows\\Explorer.exe"));
    }

    TEST_METHOD(FileDirectoryExists)
    {
        Assert::IsTrue(File::Exists(L"C:\\Windows"));
    }

    TEST_METHOD(FileNotExists)
    {
        Assert::IsFalse(File::Exists(L".\\I Do Not Exist!"));
    }

    TEST_METHOD(FileIsDirectoryExists)
    {
        Assert::IsTrue(File::IsDirectory(L"C:\\Windows"));
    }

    TEST_METHOD(FileIsDirectoryNotExists)
    {
        Assert::IsFalse(File::IsDirectory(L".\\I Do Not Exist!"));
    }

    TEST_METHOD(FileIsDirectoryFile)
    {
        Assert::IsFalse(File::IsDirectory(L"C:\\Windows\\Explorer.exe"));
    }

    TEST_METHOD(FileIsExecutable)
    {
        Assert::IsTrue(File::IsExecutable(L"C:\\Windows\\Explorer.exe"));
    }

    TEST_METHOD(FileIsExecutablNonExecutable)
    {
        Assert::IsFalse(File::IsExecutable(L"C:\\Windows\\System32\\drivers\\etc\\hosts"));
    }

    TEST_METHOD(FileIsExecutableDirectory)
    {
        Assert::IsFalse(File::IsExecutable(L"C:\\Windows"));
    }

    TEST_METHOD(FileGetsCompanyInformation)
    {
        Assert::AreEqual<std::wstring>(L"Microsoft Corporation", File::GetCompany(L"C:\\Windows\\Explorer.exe"));
    }

    TEST_METHOD(FileExtendedAttributesStaticFailsNonexistent)
    {
        Assert::ExpectException<ErrorPathNotFoundException>([] { File::GetExtendedAttributes(L"C:\\Nonexistent\\Nonexistent"); });
    }

    TEST_METHOD(FileAttributesStaticFailsNonexistent)
    {
        Assert::ExpectException<ErrorPathNotFoundException>([] { File::GetAttributes(L"C:\\Nonexistent\\Nonexistent"); });
    }

    TEST_METHOD(FileGetSizeStaticFailsNonexistent)
    {
        Assert::ExpectException<ErrorPathNotFoundException>([] { File::GetSize(L"C:\\Nonexistent\\Nonexistent"); });
    }

    TEST_METHOD(FileFileIsDefaultConstructable)
    {
        File f;
    }

    TEST_METHOD(FileFileIsMoveConstructable)
    {
        File f;
        File fTest(std::move(f));
    }

    TEST_METHOD(FileFileIsMoveAssignable)
    {
        File f;
        File fTest;
        fTest = std::move(f);
    }

    TEST_METHOD(FileExclusiveMatch)
    {
        Assert::IsTrue(File::IsExclusiveFile(L"C:\\Windows\\Explorer.exe"));
    }

    TEST_METHOD(FileExclusiveNoMatch)
    {
        Assert::IsFalse(File::IsExclusiveFile(L"C:\\Nonexistent\\Nonexistent\\Nonexistent"));
    }

    TEST_METHOD(FileExclusiveNoMatchDir)
    {
        Assert::IsFalse(File::IsExclusiveFile(L"C:\\Windows"));
    }

    TEST_METHOD(FindFilesNonExistentFile)
    {
        FindFiles(L"C:\\Nonexistent");
    }

    TEST_METHOD(FindFilesNonExistentDirectory)
    {
        FindFiles(L"C:\\Nonexistent\\*");
    }

    TEST_METHOD(FindFilesHostsExists)
    {
        bool foundHosts = false;
        FindFiles files(L"C:\\Windows\\System32\\drivers\\etc\\*");

        for(; foundHosts == false && files.IsValid(); files.Next())
        {
            if (boost::iequals(files.data.cFileName, L"hosts"))
            {
                foundHosts = true;
            }
        }

        Assert::IsTrue(foundHosts);
    }

    TEST_METHOD(FindFilesOnlyHostsStarFollowing)
    {
        bool foundHosts = false;
        FindFiles files(L"C:\\Windows\\System32\\drivers\\etc\\hos*");

        for(; foundHosts == false && files.IsValid(); files.Next())
        {
            std::wcout << files.data.cFileName << std::endl;
            if (boost::iequals(files.data.cFileName, L"hosts"))
            {
                foundHosts = true;
            }
            else
            {
                Assert::Fail(L"Got an entry that wasn't hosts");
            }
        }

        Assert::IsTrue(foundHosts);
    }

    TEST_METHOD(FindFilesOnlyHostsStarPreceding)
    {
        bool foundHosts = false;
        FindFiles files(L"C:\\Windows\\System32\\drivers\\etc\\*ts");

        for(; foundHosts == false && files.IsValid(); files.Next())
        {
            if (boost::iequals(files.data.cFileName, L"hosts"))
            {
                foundHosts = true;
            }
            else
            {
                Assert::Fail(L"Got an entry that wasn't hosts");
            }
        }

        Assert::IsTrue(foundHosts);
    }

    TEST_METHOD(FindFilesHostsExistsRecursive)
    {
        bool foundHosts = false;
        FindFiles files(L"C:\\Windows\\System32\\drivers\\*", true);

        for(; foundHosts == false && files.IsValid(); files.Next())
        {
            if (boost::iequals(files.data.cFileName, L"etc\\hosts"))
            {
                foundHosts = true;
            }
        }

        Assert::IsTrue(foundHosts);
    }

    TEST_METHOD(FindFilesHostsExistsRecursiveTwoLevels)
    {
        bool foundHosts = false;
        FindFiles files(L"C:\\Windows\\System32\\*", true);

        for(; foundHosts == false && files.IsValid(); files.Next())
        {
            if (boost::iequals(files.data.cFileName, L"drivers\\etc\\hosts"))
            {
                foundHosts = true;
            }

        }

        Assert::IsTrue(foundHosts);
    }

    TEST_METHOD(FindFilesHostsNotExistsNotRecursive)
    {
        bool foundHosts = false;
        FindFiles files(L"C:\\Windows\\System32\\drivers\\*");

        for(; foundHosts == false && files.IsValid(); files.Next())
        {
            if (boost::icontains(files.data.cFileName, L"hosts"))
            {
                foundHosts = true;
            }

        }

        Assert::IsFalse(foundHosts);
    }

    TEST_METHOD(FindFilesHostsExistsNoSubpath)
    {
        bool foundHosts = false;
        FindFiles files(L"C:\\Windows\\System32\\drivers\\*", true, false);

        for(; foundHosts == false && files.IsValid(); files.Next())
        {
            if (boost::iequals(files.data.cFileName, L"hosts"))
            {
                foundHosts = true;
            }
        }

        Assert::IsTrue(foundHosts);
    }

    TEST_METHOD(FindFilesNoDots)
    {
        FindFiles files(L"C:\\Windows\\System32\\drivers\\etc\\*");

        for(; files.IsValid(); files.Next())
        {
            if (boost::ends_with(files.data.cFileName, L"."))
            {
                Assert::Fail(L". or .. directory mistakenly included in enumeration");
            }
            std::wcout << files.data.cFileName << std::endl;
        } 
    }

    TEST_METHOD(FindFilesNoDotsRecursive)
    {
        FindFiles files(L"C:\\Windows\\System32\\drivers\\*", true);

        for(; files.IsValid(); files.Next())
        {
            if (boost::ends_with(files.data.cFileName, L"."))
            {
                Assert::Fail(L". or .. directory mistakenly included in enumeration");
            }
        }
    }

    TEST_METHOD(FindFilesDots)
    {
        FindFiles files(L"C:\\Windows\\System32\\drivers\\etc\\*", false, true, false);

        Assert::AreEqual(L".", files.data.cFileName);
        files.Next();
        Assert::AreEqual(L"..", files.data.cFileName);
    }

};

TEST_CLASS(FileItDirectoryFixture) 
{
    std::wstring tempPath;
public:
    TEST_METHOD_INITIALIZE(SetUp)
    {
        tempPath = L"%TEMP%\\ThisDirectoryShouldBeEmpty";
        Instalog::Path::ResolveFromCommandLine(tempPath);

        if (::CreateDirectory(tempPath.c_str(), NULL) == false)
        {
            DWORD lastError = ::GetLastError();
            if (lastError != ERROR_ALREADY_EXISTS)
            {
                Assert::Fail(L"Could not create directory for FileItDirectoryFixture");
            }
        }
    }

    TEST_METHOD_CLEANUP(TearDown)
    {
        if (::RemoveDirectory(tempPath.c_str()) == false)
        {
            Assert::Fail(L"Could not remove directory after FileItDirectoryFixture.  Remove this manually");
        }
    }

    TEST_METHOD(FileItDirectoryFixtureEmptyDirectory)
    {
        FindFiles files(std::wstring(tempPath).append(L"\\*"));

        Assert::IsFalse(files.IsValid());
    }

    TEST_METHOD(FileItDirectoryFixtureEmptyDirectoryDots)
    {
        FindFiles files(std::wstring(tempPath).append(L"\\*"), false, true, false);

        Assert::AreEqual<std::wstring>(L".", files.data.cFileName);
        Assert::IsTrue(files.IsValid());
        files.Next();
        Assert::AreEqual<std::wstring>(L"..", files.data.cFileName);
        Assert::IsTrue(files.IsValid());
    }

};
