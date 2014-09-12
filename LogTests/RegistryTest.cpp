// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "../LogCommon/Registry.hpp"
#include <array>
#include <functional>
#include <windows.h>
#include "sddl.h"
#include "gtest/gtest.h"
#include "../LogCommon/Win32Glue.hpp"
#include "../LogCommon/Win32Exception.hpp"
#include "../LogCommon/Utf8.hpp"

using namespace Instalog::SystemFacades;

static std::string GetCurrentUserRelativeKeyPath(char const* other)
{
    std::wstring resultStr;
    resultStr.reserve(1024);
    resultStr.append(L"\\Registry\\User\\");
    union
    {
        TOKEN_USER user;
        unsigned char storage[SECURITY_MAX_SID_SIZE + sizeof(TOKEN_USER)];
    };
    HANDLE hToken;
    DWORD len = 0;
    ::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken);
    ::GetTokenInformation(
        hToken, TokenUser, &user, SECURITY_MAX_SID_SIZE, &len);
    LPWSTR result;
    ::ConvertSidToStringSidW(user.User.Sid, &result);
    resultStr.append(result);
    ::LocalFree(result);
    return utf8::ToUtf8(resultStr) + other;
}

TEST(Registry, IsDefaultConstructable)
{
    RegistryKey key;
    EXPECT_FALSE(key.Valid());
}

TEST(Registry, CanCreateKey)
{
    RegistryKey keyUnderTest = RegistryKey::Create(
        GetCurrentUserRelativeKeyPath(
            "\\Software\\Microsoft\\NonexistentTestKeyHere"),
        KEY_QUERY_VALUE | DELETE);
    if (keyUnderTest.Invalid())
    {
        DWORD last = ::GetLastError();
        Win32Exception::ThrowFromNtError(last);
    }
    HKEY hTest;
    LSTATUS ls = ::RegOpenKeyExW(HKEY_CURRENT_USER,
                                 L"SOFTWARE\\Microsoft\\NonexistentTestKeyHere",
                                 0,
                                 KEY_ALL_ACCESS,
                                 &hTest);
    EXPECT_EQ(ERROR_SUCCESS, ls);
    ::RegCloseKey(hTest);
    keyUnderTest.Delete();
}

TEST(Registry, CanOpenKey)
{
    RegistryKey keyUnderTest = RegistryKey::Create(
        GetCurrentUserRelativeKeyPath(
            "\\Software\\Microsoft\\NonexistentTestKeyHere"),
        KEY_QUERY_VALUE | DELETE);
    if (keyUnderTest.Invalid())
    {
        DWORD last = ::GetLastError();
        Win32Exception::ThrowFromNtError(last);
    }
    RegistryKey keyOpenedAgain =
        RegistryKey::Open(GetCurrentUserRelativeKeyPath(
                              "\\Software\\Microsoft\\NonexistentTestKeyHere"),
                          KEY_QUERY_VALUE);
    EXPECT_TRUE(keyOpenedAgain.Valid());
    keyUnderTest.Delete();
}

TEST(Registry, CantOpenNonexistentKey)
{
    RegistryKey keyUnderTest =
        RegistryKey::Open(GetCurrentUserRelativeKeyPath(
                              "\\Software\\Microsoft\\NonexistentTestKeyHere"),
                          KEY_QUERY_VALUE);
    ASSERT_TRUE(keyUnderTest.Invalid());
}

TEST(Registry, CanCreateSubkey)
{
    RegistryKey rootKey = RegistryKey::Open(
        GetCurrentUserRelativeKeyPath("\\Software\\Microsoft"));
    ASSERT_TRUE(rootKey.Valid());
    RegistryKey subKey =
        RegistryKey::Create(rootKey, "Example", KEY_ALL_ACCESS);
    if (subKey.Invalid())
    {
        DWORD last = ::GetLastError();
        Win32Exception::ThrowFromNtError(last);
    }
    subKey.Delete();
}

TEST(Registry, CanDelete)
{
    RegistryKey keyUnderTest = RegistryKey::Create(
        GetCurrentUserRelativeKeyPath(
            "\\Software\\Microsoft\\NonexistentTestKeyHere"),
        DELETE);
    keyUnderTest.Delete();
    keyUnderTest = RegistryKey::Open(GetCurrentUserRelativeKeyPath(
        "\\Software\\Microsoft\\NonexistentTestKeyHere"));
    ASSERT_TRUE(keyUnderTest.Invalid());
}

TEST(Registry, CanOpenSubkey)
{
    RegistryKey rootKey = RegistryKey::Open(
        GetCurrentUserRelativeKeyPath("\\Software\\Microsoft"));
    ASSERT_TRUE(rootKey.Valid());
    RegistryKey subKey = RegistryKey::Open(rootKey, "Windows", KEY_ALL_ACCESS);
    EXPECT_TRUE(subKey.Valid());
}

TEST(Registry, GetsRightSizeInformation)
{
    HKEY hKey;
    LSTATUS errorCheck =
        ::RegOpenKeyExW(HKEY_CURRENT_USER, L"Software", 0, KEY_READ, &hKey);
    ASSERT_EQ(0, errorCheck);
    DWORD subKeys;
    DWORD values;
    FILETIME lastTime;
    errorCheck = ::RegQueryInfoKeyW(hKey,
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
                                    &lastTime);
    RegCloseKey(hKey);
    std::uint64_t convertedTime = Instalog::FiletimeToInteger(lastTime);
    RegistryKey systemKey = RegistryKey::Open(
        GetCurrentUserRelativeKeyPath("\\Software"), KEY_READ);
    auto sizeInfo = systemKey.GetSizeInformation();
    ASSERT_EQ(convertedTime, sizeInfo.GetLastWriteTime());
    ASSERT_EQ(subKeys, sizeInfo.GetNumberOfSubkeys());
    ASSERT_EQ(values, sizeInfo.GetNumberOfValues());
}

static std::vector<std::string> GetDefaultUserKeySubkeys()
{
    std::vector<std::string> defaultItems;
    defaultItems.emplace_back("Console");
    defaultItems.emplace_back("Environment");
    defaultItems.emplace_back("Software");
    std::sort(defaultItems.begin(), defaultItems.end());
    return defaultItems;
}

static void CheckVectorContainsUserSubkeys(std::vector<std::string> const& vec)
{
    auto defaultItems = GetDefaultUserKeySubkeys();
    ASSERT_TRUE(std::includes(
        vec.begin(), vec.end(), defaultItems.begin(), defaultItems.end()));
}

TEST(Registry, CanEnumerateSubKeyNames)
{
    RegistryKey systemKey = RegistryKey::Open(
        GetCurrentUserRelativeKeyPath(""), KEY_ENUMERATE_SUB_KEYS);
    ASSERT_TRUE(systemKey.Valid());
    auto subkeyNames = systemKey.EnumerateSubKeyNames();
    std::sort(subkeyNames.begin(), subkeyNames.end());
    CheckVectorContainsUserSubkeys(subkeyNames);
}

TEST(Registry, SubKeyNamesCanBeSorted)
{
    RegistryKey systemKey = RegistryKey::Open(
        GetCurrentUserRelativeKeyPath(""), KEY_ENUMERATE_SUB_KEYS);
    ASSERT_TRUE(systemKey.Valid());
    std::vector<std::string> col(systemKey.EnumerateSubKeyNames());
    std::sort(col.begin(), col.end());
    ASSERT_TRUE(std::is_sorted(col.begin(), col.end()));
}

TEST(Registry, CanGetName)
{
    RegistryKey userSoftwareKey = RegistryKey::Open(
        GetCurrentUserRelativeKeyPath("\\Software"), KEY_QUERY_VALUE);
    ASSERT_TRUE(userSoftwareKey.Valid());
    RegistryKey newLinkKey =
        RegistryKey::Create(GetCurrentUserRelativeKeyPath("\\ExampleLink"),
                            KEY_SET_VALUE | KEY_CREATE_LINK,
                            REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK);
    if (newLinkKey.Valid())
    {
        newLinkKey.SetValue("SymbolicLinkValue",
                            GetCurrentUserRelativeKeyPath("\\Software"),
                            REG_LINK);
    }

    RegistryKey shouldMatchSoftwareKey = RegistryKey::Open(
        GetCurrentUserRelativeKeyPath("\\ExampleLink"), KEY_QUERY_VALUE);
    ASSERT_STREQ(userSoftwareKey.GetName().c_str(),
                 shouldMatchSoftwareKey.GetName().c_str());
}

TEST(Registry, CanGetSubKeysOpened)
{
    RegistryKey systemKey = RegistryKey::Open(
        GetCurrentUserRelativeKeyPath(""), KEY_ENUMERATE_SUB_KEYS);
    std::vector<RegistryKey> subkeys(
        systemKey.EnumerateSubKeys(KEY_QUERY_VALUE));
    std::vector<std::string> names;
    for (RegistryKey const& p : subkeys)
    {
        std::string name(p.GetName());
        names.emplace_back(std::find(name.rbegin(), name.rend(), '\\').base(),
                           name.end());
    }

    std::sort(names.begin(), names.end());
    CheckVectorContainsUserSubkeys(names);
}

static wchar_t exampleData[] =
    L"example example example test test example \0 embedded";
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
static auto exampleLongDataCasted =
    reinterpret_cast<BYTE const*>(exampleLongData);
static wchar_t exampleMultiSz[] = L"Foo\0bar\0baz\0\0";
static auto exampleMultiSzCasted =
    reinterpret_cast<BYTE const*>(exampleMultiSz);

// These conversions should succeed.
static wchar_t wordData[] = L"42";
static wchar_t wordDataAppended[] = L"42  appended"; // Should fail

// These are too long for a DWORD, but conversion to qword should be okay.
static wchar_t wordDataTooLong[] = L"4294967296";

// These are too long to fit in a QWord, so conversions against them should
// fail.
static wchar_t wordDataTooLongQ[] = L"18446744073709551616";

static wchar_t commaTest[] = L"Foo, bar, baz";

struct RegistryValueTest : public testing::Test
{
    RegistryKey keyUnderTest;
    RegistryKey conversionsKey;
    void SetUp()
    {
        HKEY hKey;
        DWORD exampleDword = 0xDEADBEEFul;
        std::uint64_t exampleQWord = 0xBADC0FFEEBADBAD1ull;
        std::uint64_t exampleSmallQword = exampleDword;
        LSTATUS errorCheck =
            ::RegCreateKeyExW(HKEY_CURRENT_USER,
                              L"Software\\BillyONeal",
                              0,
                              0,
                              0,
                              KEY_SET_VALUE | KEY_CREATE_SUB_KEY,
                              0,
                              &hKey,
                              0);
        ASSERT_EQ(0, errorCheck);
        ::RegSetValueExW(hKey,
                         L"ExampleDataNone",
                         0,
                         REG_NONE,
                         exampleDataCasted,
                         sizeof(exampleData));
        ::RegSetValueExW(hKey,
                         L"ExampleDataBinary",
                         0,
                         REG_BINARY,
                         exampleDataCasted,
                         sizeof(exampleData));
        ::RegSetValueExW(hKey,
                         L"ExampleData",
                         0,
                         REG_SZ,
                         exampleDataCasted,
                         sizeof(exampleData));
        ::RegSetValueExW(hKey,
                         L"ExampleLongData",
                         0,
                         REG_SZ,
                         exampleLongDataCasted,
                         sizeof(exampleLongData));
        ::RegSetValueExW(hKey,
                         L"ExampleDataExpand",
                         0,
                         REG_EXPAND_SZ,
                         exampleDataCasted,
                         sizeof(exampleData));
        ::RegSetValueExW(hKey,
                         L"ExampleLongDataExpand",
                         0,
                         REG_EXPAND_SZ,
                         exampleLongDataCasted,
                         sizeof(exampleLongData));
        ::RegSetValueExW(hKey,
                         L"ExampleMultiSz",
                         0,
                         REG_MULTI_SZ,
                         exampleMultiSzCasted,
                         sizeof(exampleMultiSz));
        auto dwordPtr = reinterpret_cast<unsigned char const*>(&exampleDword);
        std::array<unsigned char, 4> dwordArray;
        std::copy(dwordPtr, dwordPtr + 4, dwordArray.begin());
        ::RegSetValueExW(
            hKey, L"ExampleDword", 0, REG_DWORD, &dwordArray[0], sizeof(DWORD));
        std::reverse(dwordArray.begin(), dwordArray.end());
        ::RegSetValueExW(hKey,
                         L"ExampleFDword",
                         0,
                         REG_DWORD_BIG_ENDIAN,
                         &dwordArray[0],
                         sizeof(DWORD));
        ::RegSetValueExW(hKey,
                         L"ExampleQWord",
                         0,
                         REG_QWORD,
                         reinterpret_cast<BYTE const*>(&exampleQWord),
                         sizeof(std::uint64_t));

        HKEY hConversions;
        errorCheck = ::RegCreateKeyExW(
            hKey, L"Conversions", 0, 0, 0, KEY_SET_VALUE, 0, &hConversions, 0);
        ASSERT_EQ(errorCheck, 0);
        ::RegSetValueExW(hConversions,
                         L"SmallQword",
                         0,
                         REG_QWORD,
                         reinterpret_cast<BYTE const*>(&exampleSmallQword),
                         sizeof(std::uint64_t));
        ::RegSetValueExW(hConversions,
                         L"WordData",
                         0,
                         REG_SZ,
                         reinterpret_cast<BYTE const*>(wordData),
                         sizeof(wordData));
        ::RegSetValueExW(hConversions,
                         L"WordDataAppended",
                         0,
                         REG_SZ,
                         reinterpret_cast<BYTE const*>(wordDataAppended),
                         sizeof(wordDataAppended));
        ::RegSetValueExW(hConversions,
                         L"WordDataTooLong",
                         0,
                         REG_SZ,
                         reinterpret_cast<BYTE const*>(wordDataTooLong),
                         sizeof(wordDataTooLong));
        ::RegSetValueExW(hConversions,
                         L"WordDataTooLongQ",
                         0,
                         REG_SZ,
                         reinterpret_cast<BYTE const*>(wordDataTooLongQ),
                         sizeof(wordDataTooLongQ));
        ::RegSetValueExW(hConversions,
                         L"CommaTest",
                         0,
                         REG_SZ,
                         reinterpret_cast<BYTE const*>(commaTest),
                         sizeof(commaTest));
        ::RegSetValueExW(hConversions,
                         L"CommaTestExpand",
                         0,
                         REG_EXPAND_SZ,
                         reinterpret_cast<BYTE const*>(commaTest),
                         sizeof(commaTest));

        ::RegCloseKey(hConversions);
        ::RegCloseKey(hKey);
        keyUnderTest = RegistryKey::Open(
            GetCurrentUserRelativeKeyPath("\\Software\\BillyONeal"),
            KEY_QUERY_VALUE);
        conversionsKey =
            RegistryKey::Open(GetCurrentUserRelativeKeyPath(
                                  "\\Software\\BillyONeal\\Conversions"),
                              KEY_QUERY_VALUE);
        ASSERT_TRUE(keyUnderTest.Valid());
        ASSERT_TRUE(conversionsKey.Valid());
    }
    void TearDown()
    {
        RegistryKey::Open(GetCurrentUserRelativeKeyPath(
                              "\\Software\\BillyONeal\\Conversions"),
                          DELETE).Delete();
        RegistryKey::Open(
            GetCurrentUserRelativeKeyPath("\\Software\\BillyONeal"), DELETE)
            .Delete();
    }

    std::vector<RegistryValueAndData> GetAndSort()
    {
        std::vector<RegistryValueAndData> resultValues(
            keyUnderTest.EnumerateValues());
        std::random_shuffle(resultValues.begin(), resultValues.end());
        std::sort(resultValues.begin(), resultValues.end());
        return resultValues;
    }
};

TEST_F(RegistryValueTest, CanGetValueData)
{
    auto data = keyUnderTest.GetValue("ExampleData");
    ASSERT_EQ(REG_SZ, data.GetType());
    ASSERT_EQ(sizeof(exampleData), data.size());
    ASSERT_TRUE(std::equal(data.cbegin(), data.cend(), exampleDataCasted));
}

TEST_F(RegistryValueTest, CanGetDWordRawData)
{
    auto data = keyUnderTest.GetValue("ExampleDword");
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
    auto data = keyUnderTest.GetValue("ExampleLongData");
    ASSERT_EQ(REG_SZ, data.GetType());
    ASSERT_EQ(sizeof(exampleLongData), data.size());
    ASSERT_TRUE(std::equal(data.cbegin(), data.cend(), exampleLongDataCasted));
}

TEST_F(RegistryValueTest, CanEnumerateValueNames)
{
    auto names = keyUnderTest.EnumerateValueNames();
    ASSERT_EQ(10, names.size());
    std::sort(names.begin(), names.end());
    std::array<std::string, 10> answers;
    answers[0] = "ExampleData";
    answers[1] = "ExampleDataBinary";
    answers[2] = "ExampleDataExpand";
    answers[3] = "ExampleDataNone";
    answers[4] = "ExampleDword";
    answers[5] = "ExampleFDword";
    answers[6] = "ExampleLongData";
    answers[7] = "ExampleLongDataExpand";
    answers[8] = "ExampleMultiSz";
    answers[9] = "ExampleQWord";
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
    ASSERT_EQ("ExampleData", namesData[0].GetName());
    ASSERT_EQ(REG_SZ, namesData[0].GetType());
    ASSERT_EQ("ExampleDword", namesData[4].GetName());
    ASSERT_EQ(REG_DWORD, namesData[4].GetType());
    ASSERT_EQ("ExampleLongData", namesData[6].GetName());
    ASSERT_EQ(REG_SZ, namesData[6].GetType());
    ASSERT_TRUE(std::equal(
        namesData[0].cbegin(), namesData[0].cend(), exampleDataCasted));
    DWORD const* dwordData =
        reinterpret_cast<DWORD const*>(&*namesData[4].cbegin());
    ASSERT_EQ(0xDEADBEEF, *dwordData);
    ASSERT_TRUE(std::equal(
        namesData[6].cbegin(), namesData[6].cend(), exampleLongDataCasted));
}

TEST_F(RegistryValueTest, CanSortValuesAndData)
{
    auto namesData = keyUnderTest.EnumerateValues();
    ASSERT_EQ(10, namesData.size());
    std::random_shuffle(namesData.begin(), namesData.end());
    std::sort(namesData.begin(), namesData.end());
    std::vector<std::string> out;
    out.resize(10);
    std::transform(namesData.begin(),
                   namesData.end(),
                   out.begin(),
                   std::mem_fun_ref(&RegistryValueAndData::GetName));
    std::vector<std::string> outSorted(out);
    std::sort(outSorted.begin(), outSorted.end());
    ASSERT_EQ(outSorted, out);
}

TEST_F(RegistryValueTest, StringizeWorks)
{
    auto underTest = GetAndSort();
    std::vector<std::string> stringized(underTest.size());
    std::transform(underTest.begin(),
                   underTest.end(),
                   stringized.begin(),
                   std::mem_fun_ref(&BasicRegistryValue::GetString));
    ASSERT_LE(stringized[0].size(), _countof(exampleData));
    EXPECT_TRUE(
        std::equal(stringized[0].cbegin(), stringized[0].cend(), exampleData));
    EXPECT_EQ(
        stringized[1], "hex:65,00,78,00,61,00,6D,00,70,00,6C,00,65,00,20"
        ",00,65,00,78,00,61,00,6D,00,70,00,6C,00,65,00,20,00,65,00,78,00,61,00"
        ",6D,00,70,00,6C,00,65,00,20,00,74,00,65,00,73,00,74,00,20,00,74,00,65"
        ",00,73,00,74,00,20,00,65,00,78,00,61,00,6D,00,70,00,6C,00,65,00,20,00"
        ",00,00,20,00,65,00,6D,00,62,00,65,00,64,00,64,00,65,00,64,00,00,00");
    ASSERT_LE(stringized[2].size(), _countof(exampleData));
    EXPECT_TRUE(
        std::equal(stringized[2].cbegin(), stringized[2].cend(), exampleData));
    EXPECT_EQ(
        stringized[3], "hex(0):65,00,78,00,61,00,6D,00,70,00,6C,00,65,00"
        ",20,00,65,00,78,00,61,00,6D,00,70,00,6C,00,65,00,20,00,65,00,78,00,61"
        ",00,6D,00,70,00,6C,00,65,00,20,00,74,00,65,00,73,00,74,00,20,00,74,00"
        ",65,00,73,00,74,00,20,00,65,00,78,00,61,00,6D,00,70,00,6C,00,65,00,20"
        ",00,00,00,20,00,65,00,6D,00,62,00,65,00,64,00,64,00,65,00,64,00,00,00"
        );
    EXPECT_EQ(stringized[4], "dword:DEADBEEF");
    EXPECT_EQ(stringized[5], "dword-be:DEADBEEF");
    ASSERT_LE(stringized[6].size(), _countof(exampleLongData));
    EXPECT_TRUE(std::equal(
        stringized[6].cbegin(), stringized[6].cend(), exampleLongData));
    ASSERT_LE(stringized[7].size(), _countof(exampleLongData));
    EXPECT_TRUE(std::equal(
        stringized[7].cbegin(), stringized[7].cend(), exampleLongData));
    EXPECT_EQ(
        stringized[8],
        "hex(7):46,00,6F,00,6F,00,00,00,62,00,61,00,72,00,00,00,62,00,61,00,7A,00,00,00,00,00,00,00");
    EXPECT_EQ(stringized[9], "qword:BADC0FFEEBADBAD1");
}

TEST_F(RegistryValueTest, StrictStringize)
{
    auto underTest = GetAndSort();
    EXPECT_THROW(underTest[1].GetStringStrict(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(underTest[3].GetStringStrict(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(underTest[4].GetStringStrict(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(underTest[5].GetStringStrict(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(underTest[8].GetStringStrict(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(underTest[9].GetStringStrict(),
                 InvalidRegistryDataTypeException);
}

TEST_F(RegistryValueTest, DwordGet)
{
    auto underTest = GetAndSort();
    EXPECT_EQ(0xDEADBEEF, underTest[4].GetDWord());
}

TEST_F(RegistryValueTest, DwordGetBe)
{
    auto underTest = GetAndSort();
    EXPECT_EQ(0xDEADBEEF, underTest[5].GetDWord());
}

TEST_F(RegistryValueTest, DwordGetStrictRejectsBe)
{
    auto underTest = GetAndSort();
    EXPECT_THROW(underTest[5].GetDWordStrict(),
                 InvalidRegistryDataTypeException);
}

TEST_F(RegistryValueTest, DwordGetSmallQword)
{
    EXPECT_EQ(0xDEADBEEF, conversionsKey["SmallQWord"].GetDWord());
}

TEST_F(RegistryValueTest, DwordGetSmallQwordStrict)
{
    EXPECT_THROW(conversionsKey["SmallQWord"].GetDWordStrict(),
                 InvalidRegistryDataTypeException);
}

TEST_F(RegistryValueTest, DwordGetLargeQword)
{
    EXPECT_THROW(keyUnderTest["ExampleQWord"].GetDWord(),
                 InvalidRegistryDataTypeException);
}

TEST_F(RegistryValueTest, DwordGetFailsForInvalidInputs)
{
    EXPECT_THROW(keyUnderTest["ExampleData"].GetDWord(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleDataBinary"].GetDWord(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleDataExpand"].GetDWord(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleDataNone"].GetDWord(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleLongData"].GetDWord(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleLongDataExpand"].GetDWord(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleMultiSz"].GetDWord(),
                 InvalidRegistryDataTypeException);
}

TEST_F(RegistryValueTest, DwordGetTriesStringConversions)
{
    EXPECT_EQ(42, conversionsKey["WordData"].GetDWord());
}

TEST_F(RegistryValueTest, DwordGetFailsForInvalidStringConversions)
{
    EXPECT_THROW(conversionsKey["WordDataAppended"].GetDWord(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(conversionsKey["WordDataTooLong"].GetDWord(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(conversionsKey["WordDataTooLongQ"].GetDWord(),
                 InvalidRegistryDataTypeException);
}

TEST_F(RegistryValueTest, QwordGet)
{
    auto underTest = GetAndSort();
    EXPECT_EQ(0xBADC0FFEEBADBAD1ull, underTest[9].GetQWord());
}

TEST_F(RegistryValueTest, QwordGetDwordBe)
{
    auto underTest = GetAndSort();
    EXPECT_EQ(0xDEADBEEFull, underTest[5].GetQWord());
}

TEST_F(RegistryValueTest, QwordGetStrictRejectsBe)
{
    auto underTest = GetAndSort();
    EXPECT_THROW(underTest[5].GetQWordStrict(),
                 InvalidRegistryDataTypeException);
}

TEST_F(RegistryValueTest, QwordGetStrictRejectsDword)
{
    auto underTest = GetAndSort();
    EXPECT_THROW(underTest[4].GetQWordStrict(),
                 InvalidRegistryDataTypeException);
}

TEST_F(RegistryValueTest, QwordGetDword)
{
    EXPECT_EQ(0xDEADBEEFull, keyUnderTest["ExampleDword"].GetQWord());
}

TEST_F(RegistryValueTest, QwordGetFailsForInvalidInputs)
{
    EXPECT_THROW(keyUnderTest["ExampleData"].GetQWord(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleDataBinary"].GetQWord(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleDataExpand"].GetQWord(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleDataNone"].GetQWord(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleLongData"].GetQWord(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleLongDataExpand"].GetQWord(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleMultiSz"].GetQWord(),
                 InvalidRegistryDataTypeException);
}

TEST_F(RegistryValueTest, QwordGetTriesStringConversions)
{
    EXPECT_EQ(42, conversionsKey["WordData"].GetQWord());
    EXPECT_EQ(4294967296ull, conversionsKey["WordDataTooLong"].GetQWord());
}

TEST_F(RegistryValueTest, QwordGetFailsForInvalidStringConversions)
{
    EXPECT_THROW(conversionsKey["WordDataAppended"].GetQWord(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(conversionsKey["WordDataTooLongQ"].GetQWord(),
                 InvalidRegistryDataTypeException);
}

TEST_F(RegistryValueTest, CanGetMultiStringArray)
{
    std::vector<std::string> expected;
    expected.emplace_back("Foo");
    expected.emplace_back("bar");
    expected.emplace_back("baz");
    std::vector<std::string> enumerated(
        keyUnderTest["ExampleMultiSz"].GetMultiStringArray());
    EXPECT_EQ(expected, enumerated);
}

TEST_F(RegistryValueTest, MultiStringGetFailsForInvalidInputs)
{
    EXPECT_THROW(keyUnderTest["ExampleDword"].GetMultiStringArray(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleFDword"].GetMultiStringArray(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleQWord"].GetMultiStringArray(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleData"].GetMultiStringArray(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleDataBinary"].GetMultiStringArray(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleDataExpand"].GetMultiStringArray(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleDataNone"].GetMultiStringArray(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleLongData"].GetMultiStringArray(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleLongDataExpand"].GetMultiStringArray(),
                 InvalidRegistryDataTypeException);
}

TEST_F(RegistryValueTest, CommaStringGetFailsForInvalidInputs)
{
    EXPECT_THROW(keyUnderTest["ExampleDword"].GetCommaStringArray(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleFDword"].GetCommaStringArray(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleQWord"].GetCommaStringArray(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleDataBinary"].GetCommaStringArray(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleDataNone"].GetCommaStringArray(),
                 InvalidRegistryDataTypeException);
    EXPECT_THROW(keyUnderTest["ExampleMultiSz"].GetCommaStringArray(),
                 InvalidRegistryDataTypeException);
}

TEST_F(RegistryValueTest, CanGetCommaString)
{
    std::vector<std::string> expected;
    expected.emplace_back("Foo");
    expected.emplace_back("bar");
    expected.emplace_back("baz");
    std::vector<std::string> enumerated(
        conversionsKey["CommaTest"].GetCommaStringArray());
    EXPECT_EQ(expected, enumerated);
    std::vector<std::string> enumeratedExpand(
        conversionsKey["CommaTestExpand"].GetCommaStringArray());
    EXPECT_EQ(expected, enumeratedExpand);
}
