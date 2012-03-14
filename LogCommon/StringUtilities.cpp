#include "pch.hpp"
#include "StringUtilities.hpp"

namespace Instalog
{
	static wchar_t http[] = L"http";
	static wchar_t HTTP[] = L"HTTP";
	static void EscapeHelper(std::wstring &target, wchar_t escapeCharacter, wchar_t rightDelimiter, bool escapeHTTP)
	{
		int httpState = 0;
		for (std::wstring::size_type i = 0; i < target.size(); ++i)
		{
			switch (target[i])
			{
			case 0x00: target[i] = L'0'; target.insert(i, 1, escapeCharacter); ++i; break;
			case 0x08: target[i] = L'b'; target.insert(i, 1, escapeCharacter); ++i; break;
			case 0x0C: target[i] = L'f'; target.insert(i, 1, escapeCharacter); ++i; break;
			case 0x0A: target[i] = L'n'; target.insert(i, 1, escapeCharacter); ++i; break;
			case 0x0D: target[i] = L'r'; target.insert(i, 1, escapeCharacter); ++i; break;
			case 0x09: target[i] = L't'; target.insert(i, 1, escapeCharacter); ++i; break;
			case 0x0B: target[i] = L'v'; target.insert(i, 1, escapeCharacter); ++i; break;
			case ' ':
				if (i == 0)
				{
					target.insert(i, 1, escapeCharacter); ++i;
				}
				else if (target[i - 1] == ' ')
				{
					target.insert(i, 1, escapeCharacter); ++i;
				}
				break;
			default:
				if (target[i] == escapeCharacter)
				{
					target.insert(i, 1, escapeCharacter);
					++i;
				}
				else if (target[i] == rightDelimiter)
				{
					target.insert(i, 1, escapeCharacter);
					++i;
				}
				else if (target[i] <= 0x1F || target[i] == 0x7F)
				{
					wchar_t hex[5];
					swprintf_s(hex, 5, L"%cx%02X", escapeCharacter, target[i]);
					target.replace(i, 1, hex);
				}
				else if (target[i] >= 0x0080)
				{
					wchar_t hex[7];
					swprintf_s(hex, 7, L"%cu%04X", escapeCharacter, target[i]);
					target.replace(i, 1, hex);
				}
			}

			if (escapeHTTP)
			{
				if (target[i] == http[httpState] || target[i] == HTTP[httpState])
				{
					httpState++;
					if (httpState == 4)
					{
						target.replace(i - 3, 4, L"htt#p");
						++i;

						httpState = 0;
					}
				}
				else if (target[i] == http[0] || target[i] == HTTP[0])
				{
					httpState = 1;
				}
				else 
				{
					httpState = 0;
				}
			}
		}
	}

	void GeneralEscape(std::wstring &target, wchar_t escapeCharacter, wchar_t rightDelimiter)
	{
		EscapeHelper(target, escapeCharacter, rightDelimiter, false);
	}

	void UrlEscape( std::wstring &target, wchar_t escapeCharacter /*= L'#'*/, wchar_t rightCharacter /*= L'\0'*/ )
	{
		EscapeHelper(target, escapeCharacter, rightCharacter, true);
	}

	std::string ConvertUnicode(const std::wstring &uni)
	{
		if (uni.empty())
		{
			return std::string();
		}
		INT length;
		BOOL blank;
		length = WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,uni.c_str(),static_cast<int>(uni.length()),NULL,NULL,"?",&blank);
		std::vector<char> resultRaw(length);
		WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,uni.c_str(),static_cast<int>(uni.length()),&resultRaw[0],length,"?",&blank);
		std::string result(resultRaw.begin(), resultRaw.end());
		return result;
	}
	std::wstring ConvertUnicode(const std::string &uni)
	{
		if (uni.empty())
		{
			return std::wstring();
		}
		INT length;
		length = MultiByteToWideChar(CP_ACP,MB_COMPOSITE,uni.c_str(),static_cast<int>(uni.length()),NULL,NULL);
		std::vector<wchar_t> resultRaw(length);
		MultiByteToWideChar(CP_ACP,MB_COMPOSITE,uni.c_str(),static_cast<int>(uni.length()),&resultRaw[0],length);
		std::wstring result(resultRaw.begin(), resultRaw.end());
		return result;
	}

	char const* InvalidHexCharacter::what() const
	{
		return "Invalid hex character in supplied string";
	}


	char const* MalformedEscapedSequence::what() const
	{
		return "Malformed escaped sequence in supplied string";
	}

}
