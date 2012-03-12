#pragma once
#include <string>

namespace Instalog
{
	void Escape(std::wstring &target, wchar_t escapeCharacter, wchar_t rightCharacter = L'\0');
	void UrlEscape(std::wstring &target, wchar_t escapeCharacter, wchar_t rightCharacter = L'\0');
	template<typename InIter, typename OutIter>
	InIter Unescape(InIter begin, InIter end, OutIter target, wchar_t escapeCharacter, wchar_t rightCharacter = L'\0')
	{
		for (; begin != end; ++begin)
		{
		}
		return begin;
	}
	template<typename InIter, typename OutIter>
	InIter CmdLineToArgvWUnEscape(InIter begin, InIter end, OutIter target)
	{
	}
	void Header(std::wstring &headerText, std::size_t headerWidth = 50);
}
