#pragma once
#include <string>

namespace Instalog
{
	inline void GeneralEscape(std::wstring &target, wchar_t escapeCharacter = L'#', wchar_t rightDelimiter = L'\0')
	{
		for (int i = 0; i < target.length(); ++i)
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
					swprintf_s(hex, 7, L"%cx%04X", escapeCharacter, target[i]);
					target.replace(i, 1, hex);
				}
			}			
		}
	}
	void UrlEscape(std::wstring &target, wchar_t escapeCharacter, wchar_t rightCharacter = L'\0');
	template<typename InIter, typename OutIter>
	inline InIter Unescape(InIter begin, InIter end, OutIter target, wchar_t escapeCharacter, wchar_t rightCharacter = L'\0')
	{
		for (; begin != end; ++begin)
		{
		}
		return begin;
	}
	template<typename InIter, typename OutIter>
	inline InIter CmdLineToArgvWUnEscape(InIter begin, InIter end, OutIter target)
	{
	}
	void Header(std::wstring &headerText, std::size_t headerWidth = 50);
}
