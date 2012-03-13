#include "pch.hpp"
#include <string>
#include <limits>
#include <LogCommon/StringUtilities.hpp>

using namespace Instalog;

TEST(StringUtilities, General_EscapeCharacter)
{
	std::wstring str = L"#";
	GeneralEscape(str);
	EXPECT_EQ(L"##", str);
}

TEST(StringUtilities, General_RightDelimiter)
{
	std::wstring str;

	str = L"[]";
	GeneralEscape(str);
	EXPECT_EQ(L"[]", str);

	str = L"[]";
	GeneralEscape(str, L'#', L']');
	EXPECT_EQ(L"[#]", str);
}

TEST(StringUtilities, General_NullCharacter)
{
	std::wstring str;
	str.push_back(0x00);
	GeneralEscape(str);
	EXPECT_EQ(L"#0", str);
}

TEST(StringUtilities, General_BackspaceCharacter)
{
	std::wstring str;
	str.push_back(0x08);
	GeneralEscape(str);
	EXPECT_EQ(L"#b", str);
}

TEST(StringUtilities, General_FormFeed)
{
	std::wstring str;
	str.push_back(0x0c);
	GeneralEscape(str);
	EXPECT_EQ(L"#f", str);
}

TEST(StringUtilities, General_Newline)
{
	std::wstring str;
	str.push_back(0x0A);
	GeneralEscape(str);
	EXPECT_EQ(L"#n", str);
}

TEST(StringUtilities, General_CarriageReturn)
{
	std::wstring str;
	str.push_back(0x0D);
	GeneralEscape(str);
	EXPECT_EQ(L"#r", str);
}

TEST(StringUtilities, General_HorizontalTab)
{
	std::wstring str;
	str.push_back(0x09);
	GeneralEscape(str);
	EXPECT_EQ(L"#t", str);
}

TEST(StringUtilities, General_VerticalTab)
{
	std::wstring str;
	str.push_back(0x0B);
	GeneralEscape(str);
	EXPECT_EQ(L"#v", str);
}

TEST(StringUtilities, General_OtherASCII)
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

TEST(StringUtilities, General_Unicode)
{
	srand((int)time(NULL));
	int increment = rand() % 10000;
	for (wchar_t c = 0x0080; c < std::numeric_limits<wchar_t>::max() - 10000; c += increment)
	{
		std::wstring str(1, c);
		GeneralEscape(str);
		wchar_t expected[7];
		swprintf_s(expected, 7, L"#x%04X", c);
		EXPECT_EQ(std::wstring(expected), str);
		increment = rand() % 10000;
	}

	// Check the last 
	std::wstring str(1, std::numeric_limits<wchar_t>::max());
	GeneralEscape(str);
	wchar_t expected[7];
	swprintf_s(expected, 7, L"#x%04X", std::numeric_limits<wchar_t>::max());
	EXPECT_EQ(std::wstring(expected), str);
}

TEST(StringUtilities, General_Whitespace)
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

TEST(StringUtilities, General_NonEscaped)
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

TEST(StringUtilities, Url_EscapeCharacter)
{
	std::wstring str = L"#";
	UrlEscape(str);
	EXPECT_EQ(L"##", str);
}

TEST(StringUtilities, Url_RightDelimiter)
{
	std::wstring str;
	
	str = L"[]";
	UrlEscape(str);
	EXPECT_EQ(L"[]", str);

	str = L"[]";
	UrlEscape(str, L'#', L']');
	EXPECT_EQ(L"[#]", str);
}

TEST(StringUtilities, Url_NullCharacter)
{
	std::wstring str;
	str.push_back(0x00);
	UrlEscape(str);
	EXPECT_EQ(L"#0", str);
}

TEST(StringUtilities, Url_BackspaceCharacter)
{
	std::wstring str;
	str.push_back(0x08);
	UrlEscape(str);
	EXPECT_EQ(L"#b", str);
}

TEST(StringUtilities, Url_FormFeed)
{
	std::wstring str;
	str.push_back(0x0c);
	UrlEscape(str);
	EXPECT_EQ(L"#f", str);
}

TEST(StringUtilities, Url_Newline)
{
	std::wstring str;
	str.push_back(0x0A);
	UrlEscape(str);
	EXPECT_EQ(L"#n", str);
}

TEST(StringUtilities, Url_CarriageReturn)
{
	std::wstring str;
	str.push_back(0x0D);
	UrlEscape(str);
	EXPECT_EQ(L"#r", str);
}

TEST(StringUtilities, Url_HorizontalTab)
{
	std::wstring str;
	str.push_back(0x09);
	UrlEscape(str);
	EXPECT_EQ(L"#t", str);
}

TEST(StringUtilities, Url_VerticalTab)
{
	std::wstring str;
	str.push_back(0x0B);
	UrlEscape(str);
	EXPECT_EQ(L"#v", str);
}

TEST(StringUtilities, Url_OtherASCII)
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
		UrlEscape(str);
		wchar_t expected[5];
		swprintf_s(expected, 5, L"#x%02X", c);
		EXPECT_EQ(std::wstring(expected), str);
	}

	{
		std::wstring str(1, 0x7F);
		UrlEscape(str);
		EXPECT_EQ(L"#x7F", str);
	}
	
	{
		std::wstring str(1, 0x7F);
		str.append(L"end");
		UrlEscape(str);
		EXPECT_EQ(L"#x7Fend", str);
	}
}

TEST(StringUtilities, Url_Unicode)
{
	srand((int)time(NULL));
	int increment = rand() % 10000;
	for (wchar_t c = 0x0080; c < std::numeric_limits<wchar_t>::max() - 10000; c += increment)
	{
		std::wstring str(1, c);
		UrlEscape(str);
		wchar_t expected[7];
		swprintf_s(expected, 7, L"#x%04X", c);
		EXPECT_EQ(std::wstring(expected), str);
		increment = rand() % 10000;
	}

	// Check the last 
	std::wstring str(1, std::numeric_limits<wchar_t>::max());
	UrlEscape(str);
	wchar_t expected[7];
	swprintf_s(expected, 7, L"#x%04X", std::numeric_limits<wchar_t>::max());
	EXPECT_EQ(std::wstring(expected), str);
}

TEST(StringUtilities, Url_Whitespace)
{
	std::wstring str;

	str = L"start  end";
	UrlEscape(str);
	EXPECT_EQ(L"start # end", str);

	str = L"start   end";
	UrlEscape(str);
	EXPECT_EQ(L"start # # end", str);

	str = L" end";
	UrlEscape(str);
	EXPECT_EQ(L"# end", str);

	str = L"  end";
	UrlEscape(str);
	EXPECT_EQ(L"# # end", str);

	str = L"start  ";
	UrlEscape(str);
	EXPECT_EQ(L"start # ", str);
}

TEST(StringUtilities, Url_NonEscaped)
{
	for (wchar_t c = 0x1F + 1; c < 0x79; ++c)
	{
		if (c == L'#' || c == L' ')
		{
			continue;
		}
		std::wstring str(1, c);
		UrlEscape(str);
		EXPECT_EQ(std::wstring(1, c), str);
	}
}

TEST(StringUtilities, Url_UrlEscape)
{
	std::wstring str;

	str = L"http";
	UrlEscape(str);
	EXPECT_EQ(L"htt#p", str);

	str = L"HTTP";
	UrlEscape(str);
	EXPECT_EQ(L"htt#p", str);

	str = L"hTtP";
	UrlEscape(str);
	EXPECT_EQ(L"htt#p", str);
	
	str = L"hthttptp";
	UrlEscape(str);
	EXPECT_EQ(L"hthtt#ptp", str);

	str = L"htt#p";
	UrlEscape(str);
	EXPECT_EQ(L"htt##p", str);
}