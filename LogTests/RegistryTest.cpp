#include "pch.hpp"
#include "gtest/gtest.h"
#include "LogCommon/Registry.hpp"
#include "LogCommon/Win32Exception.hpp"

using namespace Instalog::SystemFacades;

TEST(Registry, CanCreateKey)
{
	RegistryKey::Ptr keyUnderTest = RegistryKey::Create(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE);
	if (keyUnderTest.get() == nullptr)
	{
		DWORD last = ::GetLastError();
		Win32Exception::ThrowFromNtError(last);
	}
	HKEY hTest;
	LSTATUS ls = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\NonexistentTestKeyHere", 0, KEY_ALL_ACCESS, &hTest);
	EXPECT_EQ(ERROR_SUCCESS, ls);
	::RegCloseKey(hTest);
	::RegDeleteKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\NonexistentTestKeyHere");
}

TEST(Registry, CanOpenKey)
{
	RegistryKey::Ptr keyUnderTest = RegistryKey::Create(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE);
	if (keyUnderTest.get() == nullptr)
	{
		DWORD last = ::GetLastError();
		Win32Exception::ThrowFromNtError(last);
	}
	RegistryKey::Ptr keyOpenedAgain = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE);
	EXPECT_TRUE(keyOpenedAgain.get() != 0);
	::RegDeleteKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\NonexistentTestKeyHere");
}

TEST(Registry, CantOpenNonexistentKey)
{
	RegistryKey::Ptr keyOpenedAgain = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE);
	ASSERT_FALSE(keyOpenedAgain.get());
}
