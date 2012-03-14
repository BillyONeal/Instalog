#pragma once
#include <string>
#include <exception>
#include <windows.h>

namespace Instalog
{

	std::string ConvertUnicode(const std::wstring &uni);
	std::wstring ConvertUnicode(const std::string &uni);
	void GeneralEscape(std::wstring &target, wchar_t escapeCharacter = L'#', wchar_t rightDelimiter = L'\0');
	void UrlEscape(std::wstring &target, wchar_t escapeCharacter = L'#', wchar_t rightCharacter = L'\0');

	class MalformedEscapedSequence : std::exception 
	{
		virtual char const* what() const;
	};

	class InvalidHexCharacter : std::exception 
	{
		virtual char const* what() const;
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
	inline InIter CmdLineToArgvWUnescape(InIter begin, InIter end, OutIter target)
	{
		if (std::distance(begin, end) < 2 || *begin != L'"')
		{
			// ""s are required
			throw MalformedEscapedSequence();
		}
		++begin; //Skip "
		std::size_t backslashCount = 0;
		for(; begin != end; ++begin)
		{
			switch(*begin)
			{
			case L'\\':
				backslashCount++;
				break;
			case L'"':
				if (backslashCount)
				{
					std::fill_n(target, backslashCount / 2, L'\\');
					*target++ = L'"';
					backslashCount = 0;
				}
				else
				{
					return ++begin;
				}
				break;
			default:
				if (backslashCount)
				{
					std::fill_n(target, backslashCount, L'\\');
					backslashCount = 0;
				}
				*target++ = *begin;
			}
		}
		throw MalformedEscapedSequence();
	}
	void Header(std::wstring &headerText, std::size_t headerWidth = 50);
}
