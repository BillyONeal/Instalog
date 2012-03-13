#pragma once
#include <string>
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
