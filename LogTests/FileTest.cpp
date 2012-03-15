#include "pch.hpp"
#include "gtest/gtest.h"
#include "LogCommon/File.hpp"
#include "LogCommon/Win32Exception.hpp"

using Instalog::SystemFacades::File;
using Instalog::SystemFacades::ErrorFileNotFoundException;
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

// TEST(File, GetSizeUsingHandle)
// {
// 	File unitUnderTest(L"./CloseActuallyCalled.txt", GENERIC_READ, 0, 0, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE);
// }

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
		File fileToWriteTo(L"./CanWriteBytes.txt", GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_NEW);
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
	File fileToWriteTo(L"./CantWriteBytesToReadOnlyFile.txt", GENERIC_READ, 0, 0, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE);

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

// TEST(File, GetsCompanyInformation)
// {
// 	File f(L"C:\\Windows\\Explorer.exe");
// 	EXPECT_EQ(L"Microsoft Corporation", f.GetCompany());
// }
