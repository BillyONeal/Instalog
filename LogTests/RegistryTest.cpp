#include "pch.hpp"
#include "gtest/gtest.h"
#include "LogCommon/Registry.hpp"
#include "LogCommon/Win32Exception.hpp"

using namespace Instalog::SystemFacades;

TEST(Registry, CanCreateKey)
{
	RegistryKey::Ptr keyUnderTest = RegistryKey::Create(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE | DELETE);
	if (keyUnderTest.get() == nullptr)
	{
		DWORD last = ::GetLastError();
		Win32Exception::ThrowFromNtError(last);
	}
	HKEY hTest;
	LSTATUS ls = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\NonexistentTestKeyHere", 0, KEY_ALL_ACCESS, &hTest);
	EXPECT_EQ(ERROR_SUCCESS, ls);
	::RegCloseKey(hTest);
	keyUnderTest->Delete();
}

TEST(Registry, CanOpenKey)
{
	RegistryKey::Ptr keyUnderTest = RegistryKey::Create(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE | DELETE);
	if (keyUnderTest.get() == nullptr)
	{
		DWORD last = ::GetLastError();
		Win32Exception::ThrowFromNtError(last);
	}
	RegistryKey::Ptr keyOpenedAgain = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE);
	EXPECT_NE(nullptr, keyOpenedAgain.get());
	keyUnderTest->Delete();
}

TEST(Registry, CantOpenNonexistentKey)
{
	RegistryKey::Ptr keyOpenedAgain = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE);
	ASSERT_EQ(nullptr, keyOpenedAgain.get());
}

TEST(Registry, CanCreateSubkey)
{
	RegistryKey::Ptr rootKey = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft");
	ASSERT_NE(nullptr, rootKey.get());
	RegistryKey::Ptr subKey = RegistryKey::Create(rootKey, L"Example", KEY_ALL_ACCESS);
	if (subKey.get() == nullptr)
	{
		DWORD last = ::GetLastError();
		Win32Exception::ThrowFromNtError(last);
	}
	subKey->Delete();
}

TEST(Registry, CanDelete)
{
	RegistryKey::Ptr keyUnderTest = RegistryKey::Create(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", DELETE);
	keyUnderTest->Delete();
	keyUnderTest = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere");
	ASSERT_EQ(nullptr, keyUnderTest.get());
}

TEST(Registry, CanOpenSubkey)
{
	RegistryKey::Ptr rootKey = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft");
	ASSERT_NE(nullptr, rootKey.get());
	RegistryKey::Ptr subKey = RegistryKey::Open(rootKey, L"Windows", KEY_ALL_ACCESS);
	EXPECT_NE(nullptr, subKey.get());
}
