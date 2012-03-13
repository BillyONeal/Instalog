#pragma once
#include <string>
#include <exception>
#include <windows.h>

namespace Instalog
{

	inline std::string ConvertUnicode(const std::wstring &uni)
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
	inline std::wstring ConvertUnicode(const std::string &uni)
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

	void GeneralEscape(std::wstring &target, wchar_t escapeCharacter = L'#', wchar_t rightDelimiter = L'\0');

	void UrlEscape(std::wstring &target, wchar_t escapeCharacter = L'#', wchar_t rightCharacter = L'\0');

	class MalformedEscapedSequence : std::exception 
	{
		inline const char *what() const
		{
			return "Malformed escaped sequence in supplied string";
		}
	};

	class InvalidHexCharacter : std::exception 
	{
		inline const char *what() const
		{
			return "Invalid hex character in supplied string";
		}
	};

	template<typename InIter, typename OutIter>
	inline InIter Unescape(InIter begin, InIter end, OutIter target, wchar_t escapeCharacter = L'#', wchar_t endDelimiter = L'\0')
	{
		for (; begin != end && *begin != endDelimiter; ++begin)
		{
			if (*begin != escapeCharacter)
			{
				*target = *begin;
				++target;
			}
			else
			{
				begin++;
				if (begin == end)
				{
					throw MalformedEscapedSequence();
				}

				switch (*begin)
				{
				case L'0': *target = 0x00; ++target; break;
				case L'b': *target = 0x08; ++target; break;
				case L'f': *target = 0x0C; ++target; break;
				case L'n': *target = 0x0A; ++target; break;
				case L'r': *target = 0x0D; ++target; break;
				case L't': *target = 0x09; ++target; break;
				case L'v': *target = 0x0B; ++target; break;
				default:
					if (*begin == escapeCharacter)
					{
						*target = *begin;
						++target;
					}
					else if (*begin == L'x')
					{
						++begin;
						if (begin == end)
						{
							throw MalformedEscapedSequence();
						}

						wchar_t hexString[] = L"0xXX";
						for (int i = 2; i < 3; ++i)
						{
							if (!(*begin >= L'0' && *begin <= L'9') &&
								!(*begin >= L'A' && *begin <= L'F') &&
								!(*begin >= L'a' && *begin <= L'f'))
							{
								throw InvalidHexCharacter();
							}

							hexString[i] = *begin;
							begin++;

							if (begin == end)
							{
								throw MalformedEscapedSequence();
							}
						}
						wchar_t hexValue;
						int count = swscanf_s(hexString, L"%x", &hexValue, sizeof(wchar_t));
						if (count != 1)
						{
							throw MalformedEscapedSequence();
						}
						*target = hexValue;
						++target;
					}
				}
			}
		}
		return begin;
	}
	template<typename InIter, typename OutIter>
	inline InIter CmdLineToArgvWUnEscape(InIter begin, InIter end, OutIter target)
	{
	}
	void Header(std::wstring &headerText, std::size_t headerWidth = 50);
}
