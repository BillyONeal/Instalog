// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include <array>
#include "../LogCommon/Win32Glue.hpp"
#include "../LogCommon/Registry.hpp"
#include "../LogCommon/Win32Exception.hpp"

using namespace Instalog::SystemFacades;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

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
	Assert::IsTrue(std::includes(vec.begin(), vec.end(), defaultItems.begin(), defaultItems.end()));
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

//These conversions should succeed.
static wchar_t wordData[] = L"42";
static wchar_t wordDataAppended[] = L"42  appended"; //Should fail

//These are too long for a DWORD, but conversion to qword should be okay.
static wchar_t wordDataTooLong[] = L"4294967296";

//These are too long to fit in a QWord, so conversions against them should fail.
static wchar_t wordDataTooLongQ[] = L"18446744073709551616";

static wchar_t commaTest[] = L"Foo, bar, baz";

TEST_CLASS(RegistryTests)
{
public:
	TEST_METHOD(IsDefaultConstructable)
	{
		RegistryKey key;
		Assert::IsFalse(key.Valid());
	}

	TEST_METHOD(CanCreateKey)
	{
		RegistryKey keyUnderTest = RegistryKey::Create(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE | DELETE);
		if (keyUnderTest.Invalid())
		{
			DWORD last = ::GetLastError();
			Win32Exception::ThrowFromNtError(last);
		}
		HKEY hTest;
		LSTATUS ls = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\NonexistentTestKeyHere", 0, KEY_ALL_ACCESS, &hTest);
		Assert::AreEqual(ERROR_SUCCESS, ls);
		::RegCloseKey(hTest);
		keyUnderTest.Delete();
	}

	TEST_METHOD(CanOpenKey)
	{
		RegistryKey keyUnderTest = RegistryKey::Create(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE | DELETE);
		if (keyUnderTest.Invalid())
		{
			DWORD last = ::GetLastError();
			Win32Exception::ThrowFromNtError(last);
		}
		RegistryKey keyOpenedAgain = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE);
		Assert::IsTrue(keyOpenedAgain.Valid());
		keyUnderTest.Delete();
	}

	TEST_METHOD(CantOpenNonexistentKey)
	{
		RegistryKey keyUnderTest = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", KEY_QUERY_VALUE);
		Assert::IsTrue(keyUnderTest.Invalid());
	}

	TEST_METHOD(CanCreateSubkey)
	{
		RegistryKey rootKey = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft");
		Assert::IsTrue(rootKey.Valid());
		RegistryKey subKey = RegistryKey::Create(rootKey, L"Example", KEY_ALL_ACCESS);
		if (subKey.Invalid())
		{
			DWORD last = ::GetLastError();
			Win32Exception::ThrowFromNtError(last);
		}
		subKey.Delete();
	}

	TEST_METHOD(CanDelete)
	{
		RegistryKey keyUnderTest = RegistryKey::Create(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere", DELETE);
		keyUnderTest.Delete();
		keyUnderTest = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft\\NonexistentTestKeyHere");
		Assert::IsTrue(keyUnderTest.Invalid());
	}

	TEST_METHOD(CanOpenSubkey)
	{
		RegistryKey rootKey = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft");
		Assert::IsTrue(rootKey.Valid());
		RegistryKey subKey = RegistryKey::Open(rootKey, L"Windows", KEY_ALL_ACCESS);
		Assert::IsTrue(subKey.Valid());
	}

	TEST_METHOD(GetsRightSizeInformation)
	{
		HKEY hKey;
		LSTATUS errorCheck = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM", 0, KEY_READ, &hKey);
		Assert::AreEqual(0l, errorCheck);
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
		Assert::AreEqual(convertedTime, sizeInfo.GetLastWriteTime());
		Assert::AreEqual<unsigned long>(subKeys, sizeInfo.GetNumberOfSubkeys());
		Assert::AreEqual<unsigned long>(values, sizeInfo.GetNumberOfValues());
	}

	TEST_METHOD(CanEnumerateSubKeyNames)
	{
		RegistryKey systemKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM", KEY_ENUMERATE_SUB_KEYS);
		Assert::IsTrue(systemKey.Valid());
		auto subkeyNames = systemKey.EnumerateSubKeyNames();
		std::sort(subkeyNames.begin(), subkeyNames.end());
		CheckVectorContainsSubkeys(subkeyNames);
	}

	TEST_METHOD(SubKeyNamesCanBeSorted)
	{
		RegistryKey systemKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM", KEY_ENUMERATE_SUB_KEYS);
		Assert::IsTrue(systemKey.Valid());
		std::vector<std::wstring> col(systemKey.EnumerateSubKeyNames());
		std::sort(col.begin(), col.end());
		Assert::IsTrue(std::is_sorted(col.begin(), col.end()));
	}

	TEST_METHOD(CanGetName)
	{
		RegistryKey servicesKey = RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services", KEY_QUERY_VALUE);
		Assert::IsTrue(servicesKey.Valid());
		if (!boost::iequals(L"\\REGISTRY\\MACHINE\\SYSTEM\\ControlSet001\\Services", servicesKey.GetName()))
		{
			std::wostringstream ss;
			ss << L"Expected \\REGISTRY\\MACHINE\\SYSTEM\\ControlSet001\\Services , got "<< servicesKey.GetName() << L" instead";
			Assert::Fail(ss.str().c_str());
		}
	}

	TEST_METHOD(CanGetSubKeysOpened)
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
};

TEST_CLASS(RegistryValueTest)
{
	RegistryKey keyUnderTest;
	RegistryKey conversionsKey;
public:
	TEST_METHOD_INITIALIZE(SetUp)
	{
		HKEY hKey;
		DWORD exampleDword = 0xDEADBEEFul;
		unsigned __int64 exampleQWord = 0xBADC0FFEEBADBAD1ull;
		unsigned __int64 exampleSmallQword = exampleDword;
		LSTATUS errorCheck = ::RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\BillyONeal", 0, 0, 0, KEY_SET_VALUE | KEY_CREATE_SUB_KEY, 0, &hKey, 0);
		Assert::AreEqual(0l, errorCheck);
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
		::RegSetValueExW(hKey, L"ExampleQWord", 0, REG_QWORD, reinterpret_cast<BYTE const*>(&exampleQWord), sizeof(unsigned __int64));

		HKEY hConversions;
		errorCheck = ::RegCreateKeyExW(hKey, L"Conversions", 0, 0, 0, KEY_SET_VALUE, 0, &hConversions, 0);
		Assert::AreEqual(errorCheck, 0l);
		::RegSetValueExW(hConversions, L"SmallQword", 0, REG_QWORD, reinterpret_cast<BYTE const*>(&exampleSmallQword), sizeof(unsigned __int64));
		::RegSetValueExW(hConversions, L"WordData", 0, REG_SZ, reinterpret_cast<BYTE const*>(wordData), sizeof(wordData));
		::RegSetValueExW(hConversions, L"WordDataAppended", 0, REG_SZ, reinterpret_cast<BYTE const*>(wordDataAppended), sizeof(wordDataAppended));
		::RegSetValueExW(hConversions, L"WordDataTooLong", 0, REG_SZ, reinterpret_cast<BYTE const*>(wordDataTooLong), sizeof(wordDataTooLong));
		::RegSetValueExW(hConversions, L"WordDataTooLongQ", 0, REG_SZ, reinterpret_cast<BYTE const*>(wordDataTooLongQ), sizeof(wordDataTooLongQ));
		::RegSetValueExW(hConversions, L"CommaTest", 0, REG_SZ, reinterpret_cast<BYTE const*>(commaTest), sizeof(commaTest));
		::RegSetValueExW(hConversions, L"CommaTestExpand", 0, REG_EXPAND_SZ, reinterpret_cast<BYTE const*>(commaTest), sizeof(commaTest));

		::RegCloseKey(hConversions);
		::RegCloseKey(hKey);
		keyUnderTest = RegistryKey::Open(L"\\Registry\\Machine\\Software\\BillyONeal", KEY_QUERY_VALUE);
		conversionsKey = RegistryKey::Open(L"\\Registry\\Machine\\Software\\BillyONeal\\Conversions", KEY_QUERY_VALUE);
		Assert::IsTrue(keyUnderTest.Valid());
		Assert::IsTrue(conversionsKey.Valid());
	}
	void TearDown()
	{
		RegistryKey::Open(L"\\Registry\\Machine\\Software\\BillyONeal\\Conversions", DELETE).Delete();
		RegistryKey::Open(L"\\Registry\\Machine\\Software\\BillyONeal", DELETE).Delete();
	}

	std::vector<RegistryValueAndData> GetAndSort()
	{
		std::vector<RegistryValueAndData> resultValues(keyUnderTest.EnumerateValues());
		std::random_shuffle(resultValues.begin(), resultValues.end());
		std::sort(resultValues.begin(), resultValues.end());
		return std::move(resultValues);
	}

	TEST_METHOD(CanGetValueData)
	{
		auto data = keyUnderTest.GetValue(L"ExampleData");
		Assert::AreEqual<DWORD>(REG_SZ, data.GetType());
		Assert::AreEqual(sizeof(exampleData), data.size());
		Assert::IsTrue(std::equal(data.cbegin(), data.cend(), exampleDataCasted));
	}

	TEST_METHOD(CanGetDWordRawData)
	{
		auto data = keyUnderTest.GetValue(L"ExampleDword");
		Assert::AreEqual<DWORD>(REG_DWORD, data.GetType());
		Assert::AreEqual(sizeof(DWORD), data.size());
		union
		{
			DWORD dwordData;
			unsigned char charData[4];
		} buff;
		std::copy(data.cbegin(), data.cbegin() + 4, buff.charData);
		Assert::AreEqual(0xDEADBEEFl, buff.dwordData);
	}

	TEST_METHOD(CanGetLongValueData)
	{
		auto data = keyUnderTest.GetValue(L"ExampleLongData");
		Assert::AreEqual<DWORD>(REG_SZ, data.GetType());
		Assert::AreEqual(sizeof(exampleLongData), data.size());
		Assert::IsTrue(std::equal(data.cbegin(), data.cend(), exampleLongDataCasted));
	}

	TEST_METHOD(CanEnumerateValueNames)
	{
		auto names = keyUnderTest.EnumerateValueNames();
		Assert::AreEqual<std::size_t>(10, names.size());
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
			Assert::AreEqual(answers[idx], names[idx]);
		}
	}

	TEST_METHOD(CanEnumerateValuesAndData)
	{
		auto namesData = keyUnderTest.EnumerateValues();
		Assert::AreEqual<std::size_t>(10, namesData.size());
		std::sort(namesData.begin(), namesData.end());
		Assert::AreEqual<std::wstring>(L"ExampleData" , namesData[0].GetName());
		Assert::AreEqual<DWORD>(REG_SZ, namesData[0].GetType());
		Assert::AreEqual<std::wstring>(L"ExampleDword", namesData[4].GetName());
		Assert::AreEqual<DWORD>(REG_DWORD, namesData[4].GetType());
		Assert::AreEqual<std::wstring>(L"ExampleLongData", namesData[6].GetName());
		Assert::AreEqual<DWORD>(REG_SZ, namesData[6].GetType());
		Assert::IsTrue(std::equal(namesData[0].cbegin(), namesData[0].cend(), exampleDataCasted));
		DWORD const* dwordData = reinterpret_cast<DWORD const*>(&*namesData[4].cbegin());
		Assert::AreEqual(0xDEADBEEFl, *dwordData);
		Assert::IsTrue(std::equal(namesData[6].cbegin(), namesData[6].cend(), exampleLongDataCasted));
	}

	TEST_METHOD(CanSortValuesAndData)
	{
		auto namesData = keyUnderTest.EnumerateValues();
		Assert::AreEqual<std::size_t>(10, namesData.size());
		std::random_shuffle(namesData.begin(), namesData.end());
		std::sort(namesData.begin(), namesData.end());
		std::vector<std::wstring> out;
		out.resize(10);
		std::transform(namesData.begin(), namesData.end(), out.begin(), std::mem_fun_ref(&RegistryValueAndData::GetName));
		std::vector<std::wstring> outSorted(out);
		std::sort(outSorted.begin(), outSorted.end());
		Assert::IsTrue(outSorted == out);
	}

	TEST_METHOD(StringizeWorks)
	{
		auto underTest = GetAndSort();
		std::vector<std::wstring> stringized(underTest.size());
		std::transform(underTest.begin(), underTest.end(), stringized.begin(), std::mem_fun_ref(&BasicRegistryValue::GetString));
		Assert::IsTrue(stringized[0].size() <= _countof(exampleData));
		Assert::IsTrue(std::equal(stringized[0].cbegin(), stringized[0].cend(), exampleData));
		Assert::IsTrue(stringized[1] == L"hex:65,00,78,00,61,00,6D,00,70,00,6C,00,65,00,20"
			L",00,65,00,78,00,61,00,6D,00,70,00,6C,00,65,00,20,00,65,00,78,00,61,00"
			L",6D,00,70,00,6C,00,65,00,20,00,74,00,65,00,73,00,74,00,20,00,74,00,65"
			L",00,73,00,74,00,20,00,65,00,78,00,61,00,6D,00,70,00,6C,00,65,00,20,00"
			L",00,00,20,00,65,00,6D,00,62,00,65,00,64,00,64,00,65,00,64,00,00,00");
		Assert::IsTrue(stringized[2].size() <= _countof(exampleData));
		Assert::IsTrue(std::equal(stringized[2].cbegin(), stringized[2].cend(), exampleData));
		Assert::IsTrue(stringized[3] == L"hex(0):65,00,78,00,61,00,6D,00,70,00,6C,00,65,00"
			L",20,00,65,00,78,00,61,00,6D,00,70,00,6C,00,65,00,20,00,65,00,78,00,61"
			L",00,6D,00,70,00,6C,00,65,00,20,00,74,00,65,00,73,00,74,00,20,00,74,00"
			L",65,00,73,00,74,00,20,00,65,00,78,00,61,00,6D,00,70,00,6C,00,65,00,20"
			L",00,00,00,20,00,65,00,6D,00,62,00,65,00,64,00,64,00,65,00,64,00,00,00");
		Assert::AreEqual<std::wstring>(stringized[4], L"dword:DEADBEEF");
		Assert::AreEqual<std::wstring>(stringized[5], L"dword-be:DEADBEEF");
		Assert::IsTrue(stringized[6].size() <= _countof(exampleLongData));
		Assert::IsTrue(std::equal(stringized[6].cbegin(), stringized[6].cend(), exampleLongData));
		Assert::IsTrue(stringized[7].size() <= _countof(exampleLongData));
		Assert::IsTrue(std::equal(stringized[7].cbegin(), stringized[7].cend(), exampleLongData));
		Assert::AreEqual<std::wstring>(stringized[8], L"hex(7):46,00,6F,00,6F,00,00,00,62,00,61,00,72,00,00,00,62,00,61,00,7A,00,00,00,00,00,00,00");
		Assert::AreEqual<std::wstring>(stringized[9], L"qword:BADC0FFEEBADBAD1");
	}

	TEST_METHOD(StrictStringize)
	{
		auto underTest = GetAndSort();
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { underTest[1].GetStringStrict(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { underTest[3].GetStringStrict(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { underTest[4].GetStringStrict(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { underTest[5].GetStringStrict(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { underTest[8].GetStringStrict(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { underTest[9].GetStringStrict(); });
	}

	TEST_METHOD(DwordGet)
	{
		auto underTest = GetAndSort();
		Assert::AreEqual<DWORD>(0xDEADBEEF, underTest[4].GetDWord());
	}

	TEST_METHOD(DwordGetBe)
	{
		auto underTest = GetAndSort();
		Assert::AreEqual<DWORD>(0xDEADBEEF, underTest[5].GetDWord());
	}

	TEST_METHOD(DwordGetStrictRejectsBe)
	{
		auto underTest = GetAndSort();
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { underTest[5].GetDWordStrict(); });
	}

	TEST_METHOD(DwordGetSmallQword)
	{
		Assert::AreEqual<DWORD>(0xDEADBEEF, conversionsKey[L"SmallQWord"].GetDWord());
	}

	TEST_METHOD(DwordGetSmallQwordStrict)
	{
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { conversionsKey[L"SmallQWord"].GetDWordStrict(); });
	}

	TEST_METHOD(DwordGetLargeQword)
	{
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleQWord"].GetDWord(); });
	}

	TEST_METHOD(DwordGetFailsForInvalidInputs)
	{
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleData"].GetDWord(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleDataBinary"].GetDWord(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleDataExpand"].GetDWord(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleDataNone"].GetDWord(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleLongData"].GetDWord(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleLongDataExpand"].GetDWord(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleMultiSz"].GetDWord(); });
	}

	TEST_METHOD(DwordGetTriesStringConversions)
	{
		Assert::AreEqual<DWORD>(42, conversionsKey[L"WordData"].GetDWord());
	}

	TEST_METHOD(DwordGetFailsForInvalidStringConversions)
	{
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { conversionsKey[L"WordDataAppended"].GetDWord(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { conversionsKey[L"WordDataTooLong"].GetDWord(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { conversionsKey[L"WordDataTooLongQ"].GetDWord(); });
	}

	TEST_METHOD(QwordGet)
	{
		auto underTest = GetAndSort();
		Assert::AreEqual(0xBADC0FFEEBADBAD1ull, underTest[9].GetQWord());
	}

	TEST_METHOD(QwordGetDwordBe)
	{
		auto underTest = GetAndSort();
		Assert::AreEqual(0xDEADBEEFull, underTest[5].GetQWord());
	}

	TEST_METHOD(QwordGetStrictRejectsBe)
	{
		auto underTest = GetAndSort();
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { underTest[5].GetQWordStrict(); });
	}

	TEST_METHOD(QwordGetStrictRejectsDword)
	{
		auto underTest = GetAndSort();
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { underTest[4].GetQWordStrict(); });
	}

	TEST_METHOD(QwordGetDword)
	{
		Assert::AreEqual(0xDEADBEEFull, keyUnderTest[L"ExampleDword"].GetQWord());
	}

	TEST_METHOD(QwordGetFailsForInvalidInputs)
	{
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleData"].GetQWord(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleDataBinary"].GetQWord(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleDataExpand"].GetQWord(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleDataNone"].GetQWord(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleLongData"].GetQWord(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleLongDataExpand"].GetQWord(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleMultiSz"].GetQWord(); });
	}

	TEST_METHOD(QwordGetTriesStringConversions)
	{
		Assert::AreEqual(42ull, conversionsKey[L"WordData"].GetQWord());
		Assert::AreEqual(4294967296ull, conversionsKey[L"WordDataTooLong"].GetQWord());
	}

	TEST_METHOD(QwordGetFailsForInvalidStringConversions)
	{
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { conversionsKey[L"WordDataAppended"].GetQWord(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { conversionsKey[L"WordDataTooLongQ"].GetQWord(); });
	}

	TEST_METHOD(CanGetMultiStringArray)
	{
		std::vector<std::wstring> expected;
		expected.emplace_back(L"Foo");
		expected.emplace_back(L"bar");
		expected.emplace_back(L"baz");
		std::vector<std::wstring> enumerated(keyUnderTest[L"ExampleMultiSz"].GetMultiStringArray());
		Assert::IsTrue(expected == enumerated);
	}

	TEST_METHOD(MultiStringGetFailsForInvalidInputs)
	{
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleDword"].GetMultiStringArray(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleFDword"].GetMultiStringArray(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleQWord"].GetMultiStringArray(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleData"].GetMultiStringArray(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleDataBinary"].GetMultiStringArray(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleDataExpand"].GetMultiStringArray(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleDataNone"].GetMultiStringArray(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleLongData"].GetMultiStringArray(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleLongDataExpand"].GetMultiStringArray(); });
	}

	TEST_METHOD(CommaStringGetFailsForInvalidInputs)
	{
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleDword"].GetCommaStringArray(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleFDword"].GetCommaStringArray(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleQWord"].GetCommaStringArray(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleDataBinary"].GetCommaStringArray(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleDataNone"].GetCommaStringArray(); });
		Assert::ExpectException<InvalidRegistryDataTypeException>([&] { keyUnderTest[L"ExampleMultiSz"].GetCommaStringArray(); });
	}

	TEST_METHOD(CanGetCommaString)
	{
		std::vector<std::wstring> expected;
		expected.emplace_back(L"Foo");
		expected.emplace_back(L"bar");
		expected.emplace_back(L"baz");
		std::vector<std::wstring> enumerated(conversionsKey[L"CommaTest"].GetCommaStringArray());
		Assert::IsTrue(expected == enumerated);
		std::vector<std::wstring> enumeratedExpand(conversionsKey[L"CommaTestExpand"].GetCommaStringArray());
		Assert::IsTrue(expected == enumeratedExpand);
	}
};
