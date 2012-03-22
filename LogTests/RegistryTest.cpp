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
	RegCloseKey(hKey);
	unsigned __int64 convertedTime = Instalog::FiletimeToInteger(lastTime);
	RegistryKey::Ptr systemKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM", KEY_READ);
	auto sizeInfo = systemKey->GetSizeInformation();
	ASSERT_EQ(convertedTime, sizeInfo.GetLastWriteTime());
	ASSERT_EQ(subKeys, sizeInfo.GetNumberOfSubkeys());
	ASSERT_EQ(values, sizeInfo.GetNumberOfValues());
}

static std::vector<std::wstring> GetDefaultSystemKeySubkeys()
{
	std::vector<std::wstring> defaultItems;
	defaultItems.push_back(L"ControlSet001");
	defaultItems.push_back(L"MountedDevices");
	defaultItems.push_back(L"Select");
	defaultItems.push_back(L"Setup");
	defaultItems.push_back(L"WPA");
	std::sort(defaultItems.begin(), defaultItems.end());
	return std::move(defaultItems);
}

static void CheckVectorContainsSubkeys(std::vector<std::wstring> const& vec)
{
	auto defaultItems = GetDefaultSystemKeySubkeys();
	ASSERT_TRUE(std::includes(vec.begin(), vec.end(), defaultItems.begin(), defaultItems.end()));
}

TEST(Registry, CanEnumerateSubKeyNames)
{
	RegistryKey::Ptr systemKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM", KEY_ENUMERATE_SUB_KEYS);
	ASSERT_TRUE(systemKey != 0);
	auto subkeyNames = systemKey->EnumerateSubKeyNames();
	std::sort(subkeyNames.begin(), subkeyNames.end());
	CheckVectorContainsSubkeys(subkeyNames);
}

TEST(Registry, SubKeyNamesAreSorted)
{
	RegistryKey::Ptr systemKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM", KEY_ENUMERATE_SUB_KEYS);
	ASSERT_TRUE(systemKey != 0);
	std::vector<std::wstring> col(systemKey->EnumerateSubKeyNames());
	std::vector<std::wstring> copied(col);
	std::sort(copied.begin(), copied.end());
	EXPECT_EQ(col, copied);
}

TEST(Registry, CanGetName)
{
	RegistryKey::Ptr servicesKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services", KEY_QUERY_VALUE);
	ASSERT_TRUE(servicesKey != 0);
	if (!boost::iequals(L"\\REGISTRY\\MACHINE\\SYSTEM\\ControlSet001\\Services", servicesKey->GetName()))
	{
		GTEST_FAIL() << L"Expected \\REGISTRY\\MACHINE\\SYSTEM\\ControlSet001\\Services , got "<< servicesKey->GetName() << L" instead";
	}
}

TEST(Registry, CanGetSubKeysOpened)
{
	RegistryKey::Ptr systemKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM", KEY_ENUMERATE_SUB_KEYS);
	std::vector<RegistryKey::Ptr> subkeys(systemKey->EnumerateSubKeys(KEY_QUERY_VALUE));
	std::vector<std::wstring> names(subkeys.size());
	std::transform(subkeys.cbegin(), subkeys.cend(), names.begin(),
		[] (RegistryKey::Ptr const& p) -> std::wstring {
		std::wstring name(p->GetName());
		name.erase(name.begin(), std::find(name.rbegin(), name.rend(), L'\\').base());
		return std::move(name);
	});
	std::sort(names.begin(), names.end());
	CheckVectorContainsSubkeys(names);
}

static unsigned char exampleData[] = "example example example test test example \0 embedded";
static unsigned char exampleLongData[] = 
	"example example example test test example \0 embedded"
	"example example example test test example \0 embedded"
	"example example example test test example \0 embedded"
	"example example example test test example \0 embedded"
	"example example example test test example \0 embedded"
	"example example example test test example \0 embedded"
	"example example example test test example \0 embedded"
	"example example example test test example \0 embedded"
	"example example example test test example \0 embedded"
	"example example example test test example \0 embedded"
	"example example example test test example \0 embedded";

struct RegistryValueTest : public testing::Test
{
	RegistryKey::Ptr keyUnderTest;
	void SetUp()
	{
		HKEY hKey;
		LSTATUS errorCheck = ::RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\BillyONeal", 0, 0, 0, KEY_SET_VALUE, 0, &hKey, 0);
		ASSERT_EQ(0, errorCheck);
		::RegSetValueExW(hKey, L"ExampleData", 0, 0, exampleData, sizeof(exampleData));
		::RegSetValueExW(hKey, L"ExampleLongData", 0, 0, exampleLongData, sizeof(exampleLongData));
		::RegCloseKey(hKey);
		keyUnderTest = RegistryKey::Open(L"\\Registry\\Machine\\Software\\BillyONeal", KEY_QUERY_VALUE);
		ASSERT_TRUE(keyUnderTest != 0);
	}
	void TearDown()
	{
		RegistryKey::Open(L"\\Registry\\Machine\\Software\\BillyONeal", DELETE)->Delete();
	}
};

TEST_F(RegistryValueTest, ValuesHaveName)
{
	ASSERT_EQ(L"ExampleData", keyUnderTest->GetValue(L"ExampleData").GetName());
}

TEST_F(RegistryValueTest, CanGetValueData)
{
	auto data = keyUnderTest->GetValue(L"ExampleData").GetData();
	ASSERT_EQ(sizeof(exampleData), data.GetData().size());
	ASSERT_TRUE(std::equal(data.GetData().begin(), data.GetData().end(), exampleData));
}

TEST_F(RegistryValueTest, CanGetLongValueData)
{
	auto data = keyUnderTest->GetValue(L"ExampleLongData").GetData();
	ASSERT_EQ(sizeof(exampleLongData), data.GetData().size());
	ASSERT_TRUE(std::equal(data.GetData().begin(), data.GetData().end(), exampleLongData));
}
