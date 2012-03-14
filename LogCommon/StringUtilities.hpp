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

	inline char UnHexCharacter(wchar_t hexCharacter)
	{
		if (hexCharacter >= L'0' && hexCharacter <= L'9')
		{
			return static_cast<char>(hexCharacter - L'0');
		}
		if (hexCharacter >= L'A' && hexCharacter <= L'F')
		{
			return static_cast<char>(hexCharacter - L'A' + 10);
		}
		if (hexCharacter >= L'a' && hexCharacter <= L'a')
		{
			return static_cast<char>(hexCharacter - L'a' + 10);
		}
		throw InvalidHexCharacter();
	}

	template<typename InIter, typename OutIter>
	inline InIter Unescape(InIter begin, InIter end, OutIter target, wchar_t escapeCharacter = L'#', wchar_t endDelimiter = L'\0')
	{
		wchar_t temp;
		for (; begin != end && *begin != endDelimiter; ++begin)
		{
			if (*begin == escapeCharacter)
			{
				begin++;
				if (begin == end)
				{
					throw MalformedEscapedSequence();
				}

				switch (*begin)
				{
				case L'0': *target = 0x00; break;
				case L'b': *target = 0x08; break;
				case L'f': *target = 0x0C; break;
				case L'n': *target = 0x0A; break;
				case L'r': *target = 0x0D; break;
				case L't': *target = 0x09; break;
				case L'v': *target = 0x0B; break;
				case L'x':
					if (std::distance(begin, end) < 3) throw MalformedEscapedSequence();
					temp = 0;
					for (unsigned int idx = 0; idx < 2; ++idx)
					{
						temp <<= 4;
						temp |= UnHexCharacter(*++begin);
					}
					*target = temp;
					break;
				case L'u':
					if (std::distance(begin, end) < 5) throw MalformedEscapedSequence();
					temp = 0;
					for (unsigned int idx = 0; idx < 4; ++idx)
					{
						temp <<= 4;
						temp |= UnHexCharacter(*++begin);
					}
					*target = temp;
					break;
				default:   *target = *begin; break;
				}
			}
			else
			{
				*target = *begin;
			}
			++target;
		}
		return begin;
	}

	template<typename InIter, typename OutIter>
	inline InIter CmdLineToArgvWEscape(InIter begin, InIter end, OutIter target)
	{
		return begin;
	}

	template<typename InIter, typename OutIter>
	inline InIter CmdLineToArgvWUnescape(InIter begin, InIter end, OutIter target)
	{
		return begin;
	}
	void Header(std::wstring &headerText, std::size_t headerWidth = 50);
}
