#include "pch.hpp"
#include <sstream>
#include "gtest/gtest.h"
#include "LogCommon/StockOutputFormats.hpp"
#include "LogCommon/Win32Exception.hpp"

using namespace testing;
using namespace Instalog;

TEST(StockFormats, DateIsCorrect)
{
	unsigned __int64 ft = 123412341234ull;
	std::wostringstream ss;
	WriteDefaultDateFormat(ss, ft);
	EXPECT_EQ(L"1601-01-01 03:25:41", ss.str());
}

TEST(StockFormats, InvalidDateThrows)
{
	unsigned __int64 ft = 0xF000000000000000ull;
	std::wostringstream ss;
	EXPECT_THROW(WriteDefaultDateFormat(ss, ft), SystemFacades::ErrorInvalidParameterException);
}

TEST(StockFormats, DateMsIsCorrect)
{
	unsigned __int64 ft = 123412341234ull;
	std::wostringstream ss;
	WriteMillisecondDateFormat(ss, ft);
	EXPECT_EQ(L"1601-01-01 03:25:41.0234", ss.str());
}

TEST(StockFormats, InvalidMsDateThrows)
{
	unsigned __int64 ft = 0xF000000000000000ull;
	std::wostringstream ss;
	EXPECT_THROW(WriteMillisecondDateFormat(ss, ft), SystemFacades::ErrorInvalidParameterException);
}

static void testWriteFileAttributes(unsigned __int32 attributes, std::wstring expected)
{
	std::wostringstream ss;
	WriteFileAttributes(ss, attributes);
	EXPECT_EQ(expected, ss.str());
}

TEST(StockFormats, WriteFileAttributes_nothing)
{
	testWriteFileAttributes(0, L"------w-");
}

TEST(StockFormats, WriteFileAttributes_d)
{
	testWriteFileAttributes(FILE_ATTRIBUTE_DIRECTORY, L"d-----w-");
}

TEST(StockFormats, WriteFileAttributes_c)
{
	testWriteFileAttributes(FILE_ATTRIBUTE_COMPRESSED, L"-c----w-");
}

TEST(StockFormats, WriteFileAttributes_s)
{
	testWriteFileAttributes(FILE_ATTRIBUTE_SYSTEM, L"--s---w-");
}

TEST(StockFormats, WriteFileAttributes_h)
{
	testWriteFileAttributes(FILE_ATTRIBUTE_HIDDEN, L"---h--w-");
}

TEST(StockFormats, WriteFileAttributes_a)
{
	testWriteFileAttributes(FILE_ATTRIBUTE_ARCHIVE, L"----a-w-");
}

TEST(StockFormats, WriteFileAttributes_t)
{
	testWriteFileAttributes(FILE_ATTRIBUTE_TEMPORARY, L"-----tw-");
}

TEST(StockFormats, WriteFileAttributes_readonly)
{
	testWriteFileAttributes(FILE_ATTRIBUTE_READONLY, L"------r-");
}

TEST(StockFormats, WriteFileAttributes_r)
{
	testWriteFileAttributes(FILE_ATTRIBUTE_REPARSE_POINT, L"------wr");
}

TEST(StockFormats, WriteFileAttributes_everything)
{
	testWriteFileAttributes(
		FILE_ATTRIBUTE_DIRECTORY | 
		FILE_ATTRIBUTE_COMPRESSED | 
		FILE_ATTRIBUTE_SYSTEM | 
		FILE_ATTRIBUTE_HIDDEN | 
		FILE_ATTRIBUTE_ARCHIVE | 
		FILE_ATTRIBUTE_TEMPORARY | 
		FILE_ATTRIBUTE_READONLY | 
		FILE_ATTRIBUTE_REPARSE_POINT, L"dcshatrr");
}

TEST(StockFormats, DefaultFileNonexistent)
{
	std::wstringstream ss;
	WriteDefaultFileOutput(ss, L"C:\\Does\\Not\\Exist.exe");
	EXPECT_EQ(L"C:\\Does\\Not\\Exist.exe [x]", ss.str());
}