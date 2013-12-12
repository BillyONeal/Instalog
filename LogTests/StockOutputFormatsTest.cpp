// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include "TestSupport.hpp"
#include "../LogCommon/StockOutputFormats.hpp"
#include "../LogCommon/File.hpp"
#include "../LogCommon/Path.hpp"
#include "../LogCommon/Win32Exception.hpp"

using namespace testing;
using namespace Instalog;

TEST(StockFormats, DateIsCorrect)
{
    std::uint64_t ft = 123412341234ull;
    string_sink ss;
    WriteDefaultDateFormat(ss, ft);
    EXPECT_EQ("1601-01-01 03:25:41", ss.get());
}

TEST(StockFormats, InvalidDateThrows)
{
    std::uint64_t ft = 0xF000000000000000ull;
    string_sink ss;
    EXPECT_THROW(WriteDefaultDateFormat(ss, ft),
                 SystemFacades::ErrorInvalidParameterException);
}

TEST(StockFormats, DateMsIsCorrect)
{
    std::uint64_t ft = 123412341234ull;
    string_sink ss;
    WriteMillisecondDateFormat(ss, ft);
    EXPECT_EQ("1601-01-01 03:25:41.0234", ss.get());
}

TEST(StockFormats, InvalidMsDateThrows)
{
    std::uint64_t ft = 0xF000000000000000ull;
    string_sink ss;
    EXPECT_THROW(WriteMillisecondDateFormat(ss, ft),
                 SystemFacades::ErrorInvalidParameterException);
}

static void testWriteFileAttributes(std::uint32_t attributes,
                                    std::string expected)
{
    string_sink ss;
    WriteFileAttributes(ss, attributes);
    EXPECT_EQ(expected, ss.get());
}

TEST(StockFormats, WriteFileAttributes_nothing)
{
    testWriteFileAttributes(0, "------w-");
}

TEST(StockFormats, WriteFileAttributes_d)
{
    testWriteFileAttributes(FILE_ATTRIBUTE_DIRECTORY, "d-----w-");
}

TEST(StockFormats, WriteFileAttributes_c)
{
    testWriteFileAttributes(FILE_ATTRIBUTE_COMPRESSED, "-c----w-");
}

TEST(StockFormats, WriteFileAttributes_s)
{
    testWriteFileAttributes(FILE_ATTRIBUTE_SYSTEM, "--s---w-");
}

TEST(StockFormats, WriteFileAttributes_h)
{
    testWriteFileAttributes(FILE_ATTRIBUTE_HIDDEN, "---h--w-");
}

TEST(StockFormats, WriteFileAttributes_a)
{
    testWriteFileAttributes(FILE_ATTRIBUTE_ARCHIVE, "----a-w-");
}

TEST(StockFormats, WriteFileAttributes_t)
{
    testWriteFileAttributes(FILE_ATTRIBUTE_TEMPORARY, "-----tw-");
}

TEST(StockFormats, WriteFileAttributes_readonly)
{
    testWriteFileAttributes(FILE_ATTRIBUTE_READONLY, "------r-");
}

TEST(StockFormats, WriteFileAttributes_r)
{
    testWriteFileAttributes(FILE_ATTRIBUTE_REPARSE_POINT, "------wr");
}

TEST(StockFormats, WriteFileAttributes_everything)
{
    testWriteFileAttributes(
        FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_COMPRESSED |
            FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN |
            FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_TEMPORARY |
            FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_REPARSE_POINT,
        "dcshatrr");
}

TEST(StockFormats, DefaultFileNonexistent)
{
    string_sink ss;
    WriteDefaultFileOutput(ss, "C:\\Does\\Not\\Exist.exe");
    EXPECT_STREQ("C:\\Does\\Not\\Exist.exe [x]", ss.get().c_str());
}

TEST(StockFormats, DefaultFileWithCompany)
{
    using Instalog::SystemFacades::File;
    std::string inputFile(GetTestFilePath("TestVerInfoApp.exe"));
    Instalog::Path::Prettify(inputFile.begin(), inputFile.end());
    string_sink ss;
    WriteDefaultFileOutput(ss, inputFile);
    WIN32_FILE_ATTRIBUTE_DATA fad =
        File::GetExtendedAttributes(inputFile);
    string_sink expected;
    write(expected, inputFile, " [", fad.nFileSizeLow, ' ');
    std::uint64_t ctime =
        static_cast<std::uint64_t>(fad.ftCreationTime.dwHighDateTime) << 32 |
        fad.ftCreationTime.dwLowDateTime;
    WriteDefaultDateFormat(expected, ctime);
    write(expected, " Expected Company Name]");
    EXPECT_EQ(expected, ss);
}

TEST(StockFormats, Listing)
{
    using Instalog::SystemFacades::File;
    string_sink ss;
    WriteFileListingFile(ss, "C:\\Windows\\Explorer.exe");
    WIN32_FILE_ATTRIBUTE_DATA fad =
        File::GetExtendedAttributes("C:\\Windows\\Explorer.exe");
    string_sink expected;
    std::uint64_t ctime =
        static_cast<std::uint64_t>(fad.ftCreationTime.dwHighDateTime) << 32 |
        fad.ftCreationTime.dwLowDateTime;
    std::uint64_t mtime =
        static_cast<std::uint64_t>(fad.ftLastWriteTime.dwHighDateTime) << 32 |
        fad.ftLastWriteTime.dwLowDateTime;
    WriteDefaultDateFormat(expected, ctime);
    write(expected, " . ");
    WriteDefaultDateFormat(expected, mtime);
    write(expected, ' ', padded_number<DWORD>(10, ' ', fad.nFileSizeLow), ' ');
    WriteFileAttributes(expected, fad.dwFileAttributes);
    write(expected, " C:\\Windows\\Explorer.exe");
    EXPECT_EQ(expected, ss);
}
