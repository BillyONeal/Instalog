#include "pch.hpp"
#include "gtest/gtest.h"
#include "LogCommon/Win32Glue.hpp"
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
	EXPECT_TRUE(keyOpenedAgain.get() != nullptr);
	keyUnderTest->Delete();
}

TEST(Registry, CantOpenNonexistentKey)
{
	RegistryKey::Ptr keyOpenedAgain = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE);
	ASSERT_TRUE(keyOpenedAgain.get() == nullptr);
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
	ASSERT_TRUE(keyUnderTest.get() == nullptr);
}

TEST(Registry, CanOpenSubkey)
{
	RegistryKey::Ptr rootKey = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft");
	ASSERT_TRUE(rootKey.get() != nullptr);
	RegistryKey::Ptr subKey = RegistryKey::Open(rootKey, L"Windows", KEY_ALL_ACCESS);
	EXPECT_TRUE(subKey.get() != nullptr);
}

TEST(Registry, GetsRightSizeInformation)
{
	HKEY hKey;
	LSTATUS errorCheck = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM", 0, KEY_READ, &hKey);
	ASSERT_EQ(0, errorCheck);
	DWORD subKeys;
	DWORD values;
	FILETIME lastTime;
	errorCheck = ::RegQueryInfoKeyW(
		hKey,
		nullptr,
		nullptr, 
		nullptr,
		&subKeys,
		nullptr,
		nullptr,
		&values,
		nullptr,
		nullptr,
		nullptr,
		&lastTime
	);
	unsigned __int64 convertedTime = FiletimeToInteger(lastTime);
	RegistryKey::Ptr systemKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM", KEY_READ);
	auto sizeInfo = systemKey->GetSizeInformation();
	ASSERT_EQ(convertedTime, sizeInfo.GetLastWriteTime());
	ASSERT_EQ(subKeys, sizeInfo.GetNumberOfSubkeys());
	ASSERT_EQ(values, sizeInfo.GetNumberOfValues());
}

TEST(Registry, CanEnumerateSubKeyNames)
{
	RegistryKey::Ptr systemKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM", KEY_ENUMERATE_SUB_KEYS);
	ASSERT_TRUE(systemKey);
	auto col = systemKey->EnumerateSubKeyNames();
	std::vector<std::wstring> defaultItems;
	defaultItems.push_back(L"ControlSet001");
	defaultItems.push_back(L"CurrentControlSet");
	defaultItems.push_back(L"DriverDatabase");
	defaultItems.push_back(L"HardwareConfig");
	defaultItems.push_back(L"MountedDevices");
	defaultItems.push_back(L"Select");
	defaultItems.push_back(L"Setup");
	defaultItems.push_back(L"WPA");
	std::sort(defaultItems.begin(), defaultItems.end());
	ASSERT_TRUE(std::includes(col.begin(), col.end(), defaultItems.begin(), defaultItems.end()));
}

TEST(Registry, SubKeyNamesAreSorted)
{
	RegistryKey::Ptr systemKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM", KEY_ENUMERATE_SUB_KEYS);
	ASSERT_TRUE(systemKey);
	std::vector<std::wstring> col(systemKey->EnumerateSubKeyNames());
	std::vector<std::wstring> copied(col);
	std::sort(copied.begin(), copied.end());
	EXPECT_EQ(col, copied);
}
