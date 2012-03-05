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

TEST(File, CanDelete)
{
	{
		File unitUnderTest(L"./DeleteMe.txt", GENERIC_READ, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL);
	}
	File::Delete(L"./DeleteMe.txt");
	ASSERT_FALSE(File::Exists(L"./DeleteMe.txt"));
}
