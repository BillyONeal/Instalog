#include "pch.hpp"
#include <array>
#include "gtest/gtest.h"
#include "LogCommon/Win32Glue.hpp"
#include "LogCommon/Registry.hpp"
#include "LogCommon/Win32Exception.hpp"

using namespace Instalog::SystemFacades;

TEST(Registry, IsDefaultConstructable)
{
	RegistryKey key;
	EXPECT_FALSE(key.Valid());
}

TEST(Registry, CanCreateKey)
{
	RegistryKey keyUnderTest = RegistryKey::Create(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE | DELETE);
	if (keyUnderTest.Invalid())
	{
		DWORD last = ::GetLastError();
		Win32Exception::ThrowFromNtError(last);
	}
	HKEY hTest;
	LSTATUS ls = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\NonexistentTestKeyHere", 0, KEY_ALL_ACCESS, &hTest);
	EXPECT_EQ(ERROR_SUCCESS, ls);
	::RegCloseKey(hTest);
	keyUnderTest.Delete();
}

TEST(Registry, CanOpenKey)
{
	RegistryKey keyUnderTest = RegistryKey::Create(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE | DELETE);
	if (keyUnderTest.Invalid())
	{
		DWORD last = ::GetLastError();
		Win32Exception::ThrowFromNtError(last);
	}
	RegistryKey keyOpenedAgain = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE);
	EXPECT_TRUE(keyOpenedAgain.Valid());
	keyUnderTest.Delete();
}

TEST(Registry, CantOpenNonexistentKey)
{
	RegistryKey keyUnderTest = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE);
	ASSERT_TRUE(keyUnderTest.Invalid());
}

TEST(Registry, CanCreateSubkey)
{
	RegistryKey rootKey = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft");
	ASSERT_TRUE(rootKey.Valid());
	RegistryKey subKey = RegistryKey::Create(rootKey, L"Example", KEY_ALL_ACCESS);
	if (subKey.Invalid())
	{
		DWORD last = ::GetLastError();
		Win32Exception::ThrowFromNtError(last);
	}
	subKey.Delete();
}

TEST(Registry, CanDelete)
{
	RegistryKey keyUnderTest = RegistryKey::Create(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", DELETE);
	keyUnderTest.Delete();
	keyUnderTest = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere");
	ASSERT_TRUE(keyUnderTest.Invalid());
}

TEST(Registry, CanOpenSubkey)
{
	RegistryKey rootKey = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft");
	ASSERT_TRUE(rootKey.Valid());
	RegistryKey subKey = RegistryKey::Open(rootKey, L"Windows", KEY_ALL_ACCESS);
	EXPECT_TRUE(subKey.Valid());
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
	RegistryKey systemKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM", KEY_READ);
	auto sizeInfo = systemKey.GetSizeInformation();
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
	RegistryKey systemKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM", KEY_ENUMERATE_SUB_KEYS);
	ASSERT_TRUE(systemKey.Valid());
	auto subkeyNames = systemKey.EnumerateSubKeyNames();
	std::sort(subkeyNames.begin(), subkeyNames.end());
	CheckVectorContainsSubkeys(subkeyNames);
}

TEST(Registry, SubKeyNamesCanBeSorted)
{
	RegistryKey systemKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM", KEY_ENUMERATE_SUB_KEYS);
	ASSERT_TRUE(systemKey.Valid());
	std::vector<std::wstring> col(systemKey.EnumerateSubKeyNames());
	std::sort(col.begin(), col.end());
	ASSERT_TRUE(std::is_sorted(col.begin(), col.end()));
}

TEST(Registry, CanGetName)
{
	RegistryKey servicesKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services", KEY_QUERY_VALUE);
	ASSERT_TRUE(servicesKey.Valid());
	if (!boost::iequals(L"\\REGISTRY\\MACHINE\\SYSTEM\\ControlSet001\\Services", servicesKey.GetName()))
	{
		GTEST_FAIL() << L"Expected \\REGISTRY\\MACHINE\\SYSTEM\\ControlSet001\\Services , got "<< servicesKey.GetName() << L" instead";
	}
}

TEST(Registry, CanGetSubKeysOpened)
{
	RegistryKey systemKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM", KEY_ENUMERATE_SUB_KEYS);
	std::vector<RegistryKey> subkeys(systemKey.EnumerateSubKeys(KEY_QUERY_VALUE));
	std::vector<std::wstring> names(subkeys.size());
	std::transform(subkeys.cbegin(), subkeys.cend(), names.begin(),
		[] (RegistryKey const& p) -> std::wstring {
		std::wstring name(p.GetName());
		name.erase(name.begin(), std::find(name.rbegin(), name.rend(), L'\\').base());
		return std::move(name);
	});
	std::sort(names.begin(), names.end());
	CheckVectorContainsSubkeys(names);
}

static wchar_t exampleData[] = L"example example example test test example \0 embedded";
static auto exampleDataCasted = reinterpret_cast<BYTE const*>(exampleData);
static wchar_t exampleLongData[] = 
	L"example example example test test example \0 embedded"
	L"example example example test test example \0 embedded"
	L"example example example test test example \0 embedded"
	L"example example example test test example \0 embedded"
	L"example example example test test example \0 embedded"
	L"example example example test test example \0 embedded"
	L"example example example test test example \0 embedded"
	L"example example example test test example \0 embedded"
	L"example example example test test example \0 embedded"
	L"example example example test test example \0 embedded"
	L"example example example test test example \0 embedded";
static auto exampleLongDataCasted = reinterpret_cast<BYTE const*>(exampleLongData);
static wchar_t exampleMultiSz[] =
	L"Foo\0bar\0baz\0\0";
static auto exampleMultiSzCasted = reinterpret_cast<BYTE const*>(exampleMultiSz);

struct RegistryValueTest : public testing::Test
{
	RegistryKey keyUnderTest;
	void SetUp()
	{
		HKEY hKey;
		DWORD exampleDword = 0xDEADBEEF;
		__int64 exampleQWord = 0xBADC0FFEEBADBAD1ll;
		LSTATUS errorCheck = ::RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\BillyONeal", 0, 0, 0, KEY_SET_VALUE, 0, &hKey, 0);
		ASSERT_EQ(0, errorCheck);
		::RegSetValueExW(hKey, L"ExampleDataNone", 0, REG_NONE, exampleDataCasted, sizeof(exampleData));
		::RegSetValueExW(hKey, L"ExampleDataBinary", 0, REG_BINARY, exampleDataCasted, sizeof(exampleData));
		::RegSetValueExW(hKey, L"ExampleData", 0, REG_SZ, exampleDataCasted, sizeof(exampleData));
		::RegSetValueExW(hKey, L"ExampleLongData", 0, REG_SZ, exampleLongDataCasted, sizeof(exampleLongData));
		::RegSetValueExW(hKey, L"ExampleDataExpand", 0, REG_EXPAND_SZ, exampleDataCasted, sizeof(exampleData));
		::RegSetValueExW(hKey, L"ExampleLongDataExpand", 0, REG_EXPAND_SZ, exampleLongDataCasted, sizeof(exampleLongData));
		::RegSetValueExW(hKey, L"ExampleMultiSz", 0, REG_MULTI_SZ, exampleMultiSzCasted, sizeof(exampleMultiSz));
		auto dwordPtr = reinterpret_cast<unsigned char const*>(&exampleDword);
		std::array<unsigned char, 4> dwordArray;
		std::copy(dwordPtr, dwordPtr + 4, dwordArray.begin());
		::RegSetValueExW(hKey, L"ExampleDword", 0, REG_DWORD, &dwordArray[0], sizeof(DWORD));
		std::reverse(dwordArray.begin(), dwordArray.end());
		::RegSetValueExW(hKey, L"ExampleFDword", 0, REG_DWORD_BIG_ENDIAN, &dwordArray[0], sizeof(DWORD));
		::RegSetValueExW(hKey, L"ExampleQWord", 0, REG_QWORD, reinterpret_cast<BYTE const*>(&exampleQWord), sizeof(__int64));
		::RegCloseKey(hKey);
		keyUnderTest = RegistryKey::Open(L"\\Registry\\Machine\\Software\\BillyONeal", KEY_QUERY_VALUE);
		ASSERT_TRUE(keyUnderTest.Valid());
	}
	void TearDown()
	{
		RegistryKey::Open(L"\\Registry\\Machine\\Software\\BillyONeal", DELETE).Delete();
	}

	std::vector<RegistryValueAndData> GetAndSort()
	{
		std::vector<RegistryValueAndData> resultValues(keyUnderTest.EnumerateValues());
		std::random_shuffle(resultValues.begin(), resultValues.end());
		std::sort(resultValues.begin(), resultValues.end());
		return std::move(resultValues);
	}
};

TEST_F(RegistryValueTest, CanGetValueData)
{
	auto data = keyUnderTest.GetValue(L"ExampleData");
	ASSERT_EQ(REG_SZ, data.GetType());
	ASSERT_EQ(sizeof(exampleData), data.size());
	ASSERT_TRUE(std::equal(data.cbegin(), data.cend(), exampleDataCasted));
}

TEST_F(RegistryValueTest, CanGetDwordRawData)
{
	auto data = keyUnderTest.GetValue(L"ExampleDword");
	ASSERT_EQ(REG_DWORD, data.GetType());
	ASSERT_EQ(sizeof(DWORD), data.size());
	union
	{
		DWORD dwordData;
		unsigned char charData[4];
	} buff;
	std::copy(data.cbegin(), data.cbegin() + 4, buff.charData);
	ASSERT_EQ(0xDEADBEEF, buff.dwordData);
}

TEST_F(RegistryValueTest, CanGetLongValueData)
{
	auto data = keyUnderTest.GetValue(L"ExampleLongData");
	ASSERT_EQ(REG_SZ, data.GetType());
	ASSERT_EQ(sizeof(exampleLongData), data.size());
	ASSERT_TRUE(std::equal(data.cbegin(), data.cend(), exampleLongDataCasted));
}

TEST_F(RegistryValueTest, CanEnumerateValueNames)
{
	auto names = keyUnderTest.EnumerateValueNames();
	ASSERT_EQ(10, names.size());
	std::sort(names.begin(), names.end());
	std::array<std::wstring, 10> answers;
	answers[0] = L"ExampleData";
	answers[1] = L"ExampleDataBinary";
	answers[2] = L"ExampleDataExpand";
	answers[3] = L"ExampleDataNone";
	answers[4] = L"ExampleDword";
	answers[5] = L"ExampleFDword";
	answers[6] = L"ExampleLongData";
	answers[7] = L"ExampleLongDataExpand";
	answers[8] = L"ExampleMultiSz";
	answers[9] = L"ExampleQWord";
	for (auto idx = 0ul; idx < names.size(); ++idx)
	{
		EXPECT_EQ(answers[idx], names[idx]);
	}
}

TEST_F(RegistryValueTest, CanEnumerateValuesAndData)
{
	auto namesData = keyUnderTest.EnumerateValues();
	ASSERT_EQ(10, namesData.size());
	std::sort(namesData.begin(), namesData.end());
	ASSERT_EQ(L"ExampleData" , namesData[0].GetName());
	ASSERT_EQ(REG_SZ, namesData[0].GetType());
	ASSERT_EQ(L"ExampleDword", namesData[4].GetName());
	ASSERT_EQ(REG_DWORD, namesData[4].GetType());
	ASSERT_EQ(L"ExampleLongData", namesData[6].GetName());
	ASSERT_EQ(REG_SZ, namesData[6].GetType());
	ASSERT_TRUE(std::equal(namesData[0].cbegin(), namesData[0].cend(), exampleDataCasted));
	DWORD const* dwordData = reinterpret_cast<DWORD const*>(&*namesData[4].cbegin());
	ASSERT_EQ(0xDEADBEEF, *dwordData);
	ASSERT_TRUE(std::equal(namesData[6].cbegin(), namesData[6].cend(), exampleLongDataCasted));
}

TEST_F(RegistryValueTest, CanSortValuesAndData)
{
	auto namesData = keyUnderTest.EnumerateValues();
	ASSERT_EQ(10, namesData.size());
	std::random_shuffle(namesData.begin(), namesData.end());
	std::sort(namesData.begin(), namesData.end());
	std::vector<std::wstring> out;
	out.resize(10);
	std::transform(namesData.begin(), namesData.end(), out.begin(), std::mem_fun_ref(&RegistryValueAndData::GetName));
	std::vector<std::wstring> outSorted(out);
	std::sort(outSorted.begin(), outSorted.end());
	ASSERT_EQ(outSorted, out);
}

TEST_F(RegistryValueTest, StringizeWorks)
{
	auto underTest = GetAndSort();
	std::vector<std::wstring> stringized(underTest.size());
	std::transform(underTest.begin(), underTest.end(), stringized.begin(), std::mem_fun_ref(&BasicRegistryValue::GetString));
	ASSERT_LE(stringized[0].size(), _countof(exampleData));
	EXPECT_TRUE(std::equal(stringized[0].cbegin(), stringized[0].cend(), exampleData));
	EXPECT_EQ(stringized[1], L"hex:65,00,78,00,61,00,6D,00,70,00,6C,00,65,00,20"
		L",00,65,00,78,00,61,00,6D,00,70,00,6C,00,65,00,20,00,65,00,78,00,61,00"
		L",6D,00,70,00,6C,00,65,00,20,00,74,00,65,00,73,00,74,00,20,00,74,00,65"
		L",00,73,00,74,00,20,00,65,00,78,00,61,00,6D,00,70,00,6C,00,65,00,20,00"
		L",00,00,20,00,65,00,6D,00,62,00,65,00,64,00,64,00,65,00,64,00,00,00");
	ASSERT_LE(stringized[2].size(), _countof(exampleData));
	EXPECT_TRUE(std::equal(stringized[2].cbegin(), stringized[2].cend(), exampleData));
	EXPECT_EQ(stringized[3], L"hex(0):65,00,78,00,61,00,6D,00,70,00,6C,00,65,00"
		L",20,00,65,00,78,00,61,00,6D,00,70,00,6C,00,65,00,20,00,65,00,78,00,61"
		L",00,6D,00,70,00,6C,00,65,00,20,00,74,00,65,00,73,00,74,00,20,00,74,00"
		L",65,00,73,00,74,00,20,00,65,00,78,00,61,00,6D,00,70,00,6C,00,65,00,20"
		L",00,00,00,20,00,65,00,6D,00,62,00,65,00,64,00,64,00,65,00,64,00,00,00");
	EXPECT_EQ(stringized[4], L"dword:DEADBEEF");
	EXPECT_EQ(stringized[5], L"dword-be:DEADBEEF");
	ASSERT_LE(stringized[6].size(), _countof(exampleLongData));
	EXPECT_TRUE(std::equal(stringized[6].cbegin(), stringized[6].cend(), exampleLongData));
	ASSERT_LE(stringized[7].size(), _countof(exampleLongData));
	EXPECT_TRUE(std::equal(stringized[7].cbegin(), stringized[7].cend(), exampleLongData));
	EXPECT_EQ(stringized[8], L"hex(7):46,00,6F,00,6F,00,00,00,62,00,61,00,72,00,00,00,62,00,61,00,7A,00,00,00,00,00,00,00");
	EXPECT_EQ(stringized[9], L"qword:BADC0FFEEBADBAD1");
}


