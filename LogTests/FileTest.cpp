#include "pch.hpp"
#include "gtest/gtest.h"
#include "LogCommon/File.hpp"
#include "LogCommon/Win32Exception.hpp"

using Instalog::SystemFacades::File;
using Instalog::SystemFacades::ErrorFileNotFoundException;

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

TEST(File, CanReadBytes)
{
	File explorer(L"C:\\Windows\\Explorer.exe");
	std::vector<char> bytes = explorer.ReadBytes(2);
	EXPECT_EQ('M', bytes[0]);
	EXPECT_EQ('Z', bytes[1]);
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
