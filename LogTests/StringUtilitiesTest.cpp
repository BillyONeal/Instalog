#include "pch.hpp"
#include <string>
#include <limits>
#include <LogCommon/StringUtilities.hpp>

using namespace Instalog;

TEST(StringUtilities, EscapeCharacter)
{
	std::wstring str = L"#";
	GeneralEscape(str);
	EXPECT_EQ(L"##", str);
}

TEST(StringUtilities, RightDelimiter)
{
	std::wstring str;
	
	str = L"[]";
	GeneralEscape(str);
	EXPECT_EQ(L"[]", str);

	str = L"[]";
	GeneralEscape(str, L'#', L']');
	EXPECT_EQ(L"[#]", str);
}

TEST(StringUtilities, NullCharacter)
{
	std::wstring str;
	str.push_back(0x00);
	GeneralEscape(str);
	EXPECT_EQ(L"#0", str);
}

TEST(StringUtilities, BackspaceCharacter)
{
	std::wstring str;
	str.push_back(0x08);
	GeneralEscape(str);
	EXPECT_EQ(L"#b", str);
}

TEST(StringUtilities, FormFeed)
{
	std::wstring str;
	str.push_back(0x0c);
	GeneralEscape(str);
	EXPECT_EQ(L"#f", str);
}

TEST(StringUtilities, Newline)
{
	std::wstring str;
	str.push_back(0x0A);
	GeneralEscape(str);
	EXPECT_EQ(L"#n", str);
}

TEST(StringUtilities, CarriageReturn)
{
	std::wstring str;
	str.push_back(0x0D);
	GeneralEscape(str);
	EXPECT_EQ(L"#r", str);
}

TEST(StringUtilities, HorizontalTab)
{
	std::wstring str;
	str.push_back(0x09);
	GeneralEscape(str);
	EXPECT_EQ(L"#t", str);
}

TEST(StringUtilities, VerticalTab)
{
	std::wstring str;
	str.push_back(0x0B);
	GeneralEscape(str);
	EXPECT_EQ(L"#v", str);
}

TEST(StringUtilities, OtherASCII)
{
	for (wchar_t c = 0x00; c < 0x1F; ++c)
	{
		if (c == 0x00 ||
			c == 0x08 ||
			c == 0x0C ||
			c == 0x0A ||
			c == 0x0D ||
			c == 0x09 ||
			c == 0x0B)
		{
			continue;
		}

		std::wstring str(1, c);
		GeneralEscape(str);
		wchar_t expected[5];
		swprintf_s(expected, 5, L"#x%02X", c);
		EXPECT_EQ(std::wstring(expected), str);
	}

	{
		std::wstring str(1, 0x7F);
		GeneralEscape(str);
		EXPECT_EQ(L"#x7F", str);
	}
	
	{
		std::wstring str(1, 0x7F);
		str.append(L"end");
		GeneralEscape(str);
		EXPECT_EQ(L"#x7Fend", str);
	}
}

TEST(StringUtilities, Unicode)
{
	for (wchar_t c = 0x0080; c < std::numeric_limits<wchar_t>::max(); ++c)
	{
		std::wstring str(1, c);
		GeneralEscape(str);
		wchar_t expected[7];
		swprintf_s(expected, 7, L"#x%04X", c);
		EXPECT_EQ(std::wstring(expected), str);
	}
}

TEST(StringUtilities, Whitespace)
{
	std::wstring str;

	str = L"start  end";
	GeneralEscape(str);
	EXPECT_EQ(L"start # end", str);

	str = L"start   end";
	GeneralEscape(str);
	EXPECT_EQ(L"start # # end", str);

	str = L" end";
	GeneralEscape(str);
	EXPECT_EQ(L"# end", str);

	str = L"  end";
	GeneralEscape(str);
	EXPECT_EQ(L"# # end", str);

	str = L"start  ";
	GeneralEscape(str);
	EXPECT_EQ(L"start # ", str);
}

TEST(StringUtilities, NonEscaped)
{
	for (wchar_t c = 0x1F + 1; c < 0x79; ++c)
	{
		if (c == L'#' || c == L' ')
		{
			continue;
		}
		std::wstring str(1, c);
		GeneralEscape(str);
		EXPECT_EQ(std::wstring(1, c), str);
	}
}