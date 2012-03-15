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
