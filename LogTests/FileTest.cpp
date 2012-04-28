// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include "LogCommon/File.hpp"
#include "LogCommon/Path.hpp"
#include "LogCommon/Win32Exception.hpp"

using Instalog::SystemFacades::File;
using Instalog::SystemFacades::FindFiles;
using Instalog::SystemFacades::ErrorFileNotFoundException;
using Instalog::SystemFacades::ErrorPathNotFoundException;
using Instalog::SystemFacades::ErrorAccessDeniedException;

TEST(File, CanOpenDefault)
{
	ASSERT_THROW(File unitUnderTest(L"./CanOpenDefault.txt"), ErrorFileNotFoundException);
}

TEST(File, CanOpenWithOtherOptions)
{
	File unitUnderTest(L"./CanOpenWithOtherOptions.txt", GENERIC_READ, 0, 0, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE);
}

TEST(File, CloseActuallyCalled)
{
	{
		File unitUnderTest(L"./CloseActuallyCalled.txt", GENERIC_READ, 0, 0, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE);
	}
	ASSERT_FALSE(File::Exists(L"./CloseActuallyCalled.txt"));
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
		File fileToWriteTo(L"./GetSizeHandle.txt", GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS);
		fileToWriteTo.WriteBytes(bytesToWrite);
	}

	// Read from the file
	{
		File fileToReadFrom(L"./GetSizeHandle.txt", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE);
		EXPECT_EQ(4, fileToReadFrom.GetSize());
	}
}

TEST(File, GetAttributesHandle)
{
	// Write to the file
	{
		File fileToWriteTo(L"./GetAttributesHandle.txt", GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS);
	}

	// Read from the file
	{
		File fileToReadFrom(L"./GetAttributesHandle.txt", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE);
		EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE, fileToReadFrom.GetAttributes());
	}
}

TEST(File, GetExtendedAttributes)
{
	File explorer(L"C:\\Windows\\Explorer.exe");
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
		File fileToWriteTo(L"./GetSizeNoHandle.txt", GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS);
		fileToWriteTo.WriteBytes(bytesToWrite);
	}

	EXPECT_EQ(4, File::GetSize(L"./GetSizeNoHandle.txt"));

	File::Delete(L"GetSizeNoHandle.txt");
}

TEST(File, GetAttributesNoHandle)
{
	// Write to the file
	{
		File fileToWriteTo(L"./GetAttributesNoHandle.txt", GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS);
	}

	EXPECT_EQ(FILE_ATTRIBUTE_ARCHIVE, File::GetAttributes(L"./GetAttributesNoHandle.txt"));

	File::Delete(L"./GetAttributesNoHandle.txt");
}

TEST(File, CanReadBytes)
{
	File explorer(L"C:\\Windows\\Explorer.exe");
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
		File fileToWriteTo(L"./CanWriteBytes.txt", GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS);
		EXPECT_TRUE(fileToWriteTo.WriteBytes(bytesToWrite));
	}

	// Read from the file
	{
		File fileToReadFrom(L"./CanWriteBytes.txt", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE);
		std::vector<char> bytesRead = fileToReadFrom.ReadBytes(4);
		EXPECT_EQ(bytesToWrite, bytesRead);
	}
}

TEST(File, CantWriteBytesToReadOnlyFile)
{
	File fileToWriteTo(L"./CantWriteBytesToReadOnlyFile.txt", GENERIC_READ, 0, 0, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE);

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
		File unitUnderTest(L"./DeleteMe.txt", GENERIC_READ, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL);
	}
	File::Delete(L"./DeleteMe.txt");
	ASSERT_FALSE(File::Exists(L"./DeleteMe.txt"));
}

TEST(File, DeleteChecksError)
{
	ASSERT_THROW(File::Delete(L"./IDoNotExist.txt"), ErrorFileNotFoundException);
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
	ASSERT_FALSE(File::Exists(L".\\I Do Not Exist!"));
}

TEST(File, IsDirectoryExists)
{
	ASSERT_TRUE(File::IsDirectory(L"C:\\Windows"));
}

TEST(File, IsDirectoryNotExists)
{
	ASSERT_FALSE(File::IsDirectory(L".\\I Do Not Exist!"));
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

TEST(FindFiles, NonExistentFile)
{
	ASSERT_THROW(FindFiles(L"C:\\Nonexistent"), ErrorFileNotFoundException);
}

TEST(FindFiles, NonExistentDirectory)
{
	ASSERT_THROW(FindFiles(L"C:\\Nonexistent\\*"), ErrorPathNotFoundException);
}

TEST(FindFiles, HostsExists)
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

	ASSERT_TRUE(foundHosts);
}

TEST(FindFiles, OnlyHostsStarFollowing)
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
			FAIL() << "Got an entry that wasn't hosts";
		}
	}

	ASSERT_TRUE(foundHosts);
}

TEST(FindFiles, OnlyHostsStarPreceding)
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
			FAIL() << "Got an entry that wasn't hosts";
		}
	}

	ASSERT_TRUE(foundHosts);
}

TEST(FindFiles, HostsExistsRecursive)
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

	ASSERT_TRUE(foundHosts);
}

TEST(FindFiles, HostsExistsRecursiveTwoLevels)
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

	ASSERT_TRUE(foundHosts);
}

TEST(FindFiles, HostsNotExistsNotRecursive)
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

	ASSERT_FALSE(foundHosts);
}

TEST(FindFiles, HostsExistsNoSubpath)
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

	ASSERT_TRUE(foundHosts);
}

TEST(FindFiles, NoDots)
{
	FindFiles files(L"C:\\Windows\\System32\\drivers\\etc\\*");

	for(; files.IsValid(); files.Next())
	{
		if (boost::ends_with(files.data.cFileName, L"."))
		{
			FAIL() << ". or .. directory mistakenly included in enumeration";
		}
		std::wcout << files.data.cFileName << std::endl;
	} 
}

TEST(FindFiles, NoDotsRecursive)
{
	FindFiles files(L"C:\\Windows\\System32\\drivers\\*", true);

	for(; files.IsValid(); files.Next())
	{
		if (boost::ends_with(files.data.cFileName, L"."))
		{
			FAIL() << ". or .. directory mistakenly included in enumeration";
		}
	}
}

TEST(FindFiles, Dots)
{
	FindFiles files(L"C:\\Windows\\System32\\drivers\\etc\\*", false, true, false);

	EXPECT_EQ(L".", std::wstring(files.data.cFileName));
	files.Next();
	EXPECT_EQ(L"..", std::wstring(files.data.cFileName));
}

struct FileItDirectoryFixture : public testing::Test
{
	std::wstring tempPath;

	virtual void SetUp()
	{
		tempPath = L"%TEMP%\\ThisDirectoryShouldBeEmpty";
		Instalog::Path::ResolveFromCommandLine(tempPath);

		if (::CreateDirectory(tempPath.c_str(), NULL) == false)
		{
			DWORD lastError = ::GetLastError();
			if (lastError != ERROR_ALREADY_EXISTS)
			{
				FAIL() << "Could not create directory for FileItDirectoryFixture";
			}
		}
	}

	virtual void TearDown()
	{
		if (::RemoveDirectory(tempPath.c_str()) == false)
		{
			FAIL() << "Could not remove directory after FileItDirectoryFixture.  Remove this manually";
		}
	}
};

TEST_F(FileItDirectoryFixture, EmptyDirectory)
{
	FindFiles files(std::wstring(tempPath).append(L"\\*"));

	ASSERT_FALSE(files.IsValid());
}

TEST_F(FileItDirectoryFixture, EmptyDirectoryDots)
{
	FindFiles files(std::wstring(tempPath).append(L"\\*"), false, true, false);

	EXPECT_EQ(L".", std::wstring(files.data.cFileName));
	EXPECT_TRUE(files.IsValid());
	files.Next();
	EXPECT_EQ(L"..", std::wstring(files.data.cFileName));
	EXPECT_TRUE(files.IsValid());
}