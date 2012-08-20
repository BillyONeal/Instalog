// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include <sstream>
#include "../LogCommon/StockOutputFormats.hpp"
#include "../LogCommon/File.hpp"
#include "../LogCommon/Win32Exception.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Instalog;

static void testWriteFileAttributes(unsigned __int32 attributes, std::wstring expected)
{
	std::wostringstream ss;
	WriteFileAttributes(ss, attributes);
	Assert::AreEqual(expected, ss.str());
}

TEST_CLASS(StockFormatsTest)
{
	TEST_METHOD(DateIsCorrect)
	{
		unsigned __int64 ft = 123412341234ull;
		std::wostringstream ss;
		WriteDefaultDateFormat(ss, ft);
		Assert::AreEqual<std::wstring>(L"1601-01-01 03:25:41", ss.str());
	}

	TEST_METHOD(InvalidDateThrows)
	{
		unsigned __int64 ft = 0xF000000000000000ull;
		std::wostringstream ss;
		Assert::ExpectException<SystemFacades::ErrorInvalidParameterException>([&] { WriteDefaultDateFormat(ss, ft); } );
	}

	TEST_METHOD(DateMsIsCorrect)
	{
		unsigned __int64 ft = 123412341234ull;
		std::wostringstream ss;
		WriteMillisecondDateFormat(ss, ft);
		Assert::AreEqual<std::wstring>(L"1601-01-01 03:25:41.0234", ss.str());
	}

	TEST_METHOD(InvalidMsDateThrows)
	{
		unsigned __int64 ft = 0xF000000000000000ull;
		std::wostringstream ss;
		Assert::ExpectException<SystemFacades::ErrorInvalidParameterException>([&] { WriteMillisecondDateFormat(ss, ft); });
	}

	TEST_METHOD(WriteFileAttributes_nothing)
	{
		testWriteFileAttributes(0, L"------w-");
	}

	TEST_METHOD(WriteFileAttributes_d)
	{
		testWriteFileAttributes(FILE_ATTRIBUTE_DIRECTORY, L"d-----w-");
	}

	TEST_METHOD(WriteFileAttributes_c)
	{
		testWriteFileAttributes(FILE_ATTRIBUTE_COMPRESSED, L"-c----w-");
	}

	TEST_METHOD(WriteFileAttributes_s)
	{
		testWriteFileAttributes(FILE_ATTRIBUTE_SYSTEM, L"--s---w-");
	}

	TEST_METHOD(WriteFileAttributes_h)
	{
		testWriteFileAttributes(FILE_ATTRIBUTE_HIDDEN, L"---h--w-");
	}

	TEST_METHOD(WriteFileAttributes_a)
	{
		testWriteFileAttributes(FILE_ATTRIBUTE_ARCHIVE, L"----a-w-");
	}

	TEST_METHOD(WriteFileAttributes_t)
	{
		testWriteFileAttributes(FILE_ATTRIBUTE_TEMPORARY, L"-----tw-");
	}

	TEST_METHOD(WriteFileAttributes_readonly)
	{
		testWriteFileAttributes(FILE_ATTRIBUTE_READONLY, L"------r-");
	}

	TEST_METHOD(WriteFileAttributes_r)
	{
		testWriteFileAttributes(FILE_ATTRIBUTE_REPARSE_POINT, L"------wr");
	}

	TEST_METHOD(WriteFileAttributes_everything)
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

	TEST_METHOD(DefaultFileNonexistent)
	{
		std::wstringstream ss;
		WriteDefaultFileOutput(ss, L"C:\\Does\\Not\\Exist.exe");
		Assert::AreEqual<std::wstring>(L"C:\\Does\\Not\\Exist.exe [x]", ss.str());
	}

	TEST_METHOD(DefaultFileWithCompany)
	{
		using Instalog::SystemFacades::File;
		std::wstringstream ss;
		WriteDefaultFileOutput(ss, L"Explorer");
		WIN32_FILE_ATTRIBUTE_DATA fad = File::GetExtendedAttributes(L"C:\\Windows\\Explorer.exe");
		std::wstringstream expected;
		expected << L"C:\\Windows\\Explorer.exe [" << fad.nFileSizeLow << L' ';
		unsigned __int64 ctime = 
			static_cast<unsigned __int64>(fad.ftCreationTime.dwHighDateTime) << 32
			| fad.ftCreationTime.dwLowDateTime;
		WriteDefaultDateFormat(expected, ctime);
		expected << L" Microsoft Corporation]";
		Assert::AreEqual(expected.str(), ss.str());
	}

	TEST_METHOD(Listing)
	{
		using Instalog::SystemFacades::File;
		std::wstringstream ss;
		WriteFileListingFile(ss, L"C:\\Windows\\Explorer.exe");
		WIN32_FILE_ATTRIBUTE_DATA fad = File::GetExtendedAttributes(L"C:\\Windows\\Explorer.exe");
		std::wstringstream expected;
		unsigned __int64 ctime = 
			static_cast<unsigned __int64>(fad.ftCreationTime.dwHighDateTime) << 32
			| fad.ftCreationTime.dwLowDateTime;
		unsigned __int64 mtime = 
			static_cast<unsigned __int64>(fad.ftLastWriteTime.dwHighDateTime) << 32
			| fad.ftLastWriteTime.dwLowDateTime;
		WriteDefaultDateFormat(expected, ctime);
		expected << L" . ";
		WriteDefaultDateFormat(expected, mtime);
		expected << L' ' << std::setw(10) << std::setfill(L' ') << fad.nFileSizeLow << L' ';
		WriteFileAttributes(expected, fad.dwFileAttributes);
		expected << L" C:\\Windows\\Explorer.exe";
		Assert::AreEqual(expected.str(), ss.str());
	}
};
