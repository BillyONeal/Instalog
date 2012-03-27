#pragma once
#include <string>
#include <exception>
#include <windows.h>

namespace Instalog
{
	/// @brief	Convert wide string to narrow string
	///
	/// @param	uni	The string to convert
	///
	/// @return	The narrow string
	std::string ConvertUnicode(const std::wstring &uni);

	/// @brief	Convert narrow string to wide string
	///
	/// @param	uni	The string to convert
	///
	/// @return	The wide string
	std::wstring ConvertUnicode(const std::string &uni);

	/// @brief	Escapes strings according to the general escape scheme
	///
	/// @param [in,out]	target 	String to escape
	/// @param	escapeCharacter	(optional) the escape character.
	/// @param	rightDelimiter 	(optional) the right delimiter.
	void GeneralEscape(std::wstring &target, wchar_t escapeCharacter = L'#', wchar_t rightDelimiter = L'\0');

	/// @brief	Escapes strings according to the url escape scheme
	///
	/// @param [in,out]	target 	String to escape
	/// @param	escapeCharacter	(optional) the escape character.
	/// @param	rightDelimiter 	(optional) the right delimiter.
	void UrlEscape(std::wstring &target, wchar_t escapeCharacter = L'#', wchar_t rightDelimiter = L'\0');

	/// @brief	Malformed escaped sequence 
	class MalformedEscapedSequence : public std::exception 
	{
		virtual char const* what() const;
	};

	/// @brief	Invalid hexadecimal character.
	class InvalidHexCharacter : public std::exception 
	{
		virtual char const* what() const;
	};

	inline void HexCharacter(unsigned char characterToHex, std::wstring &target)
	{
		static const wchar_t chars[] = L"0123456789ABCDEF";
		target.push_back(chars[characterToHex >> 4]);
		target.push_back(chars[characterToHex & 0x0F]);
	}

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

	/// @brief	Unescapes a given string
	///
	/// @exception	MalformedEscapedSequence	Thrown when a malformed escaped sequence is passed in
	///
	/// @param	begin		   	An iterator at the beginning of the string to escape
	/// @param	end			   	An iterator one past the end of the string to escape
	/// @param	target		   	Target iterator to write to.  Should not be overlapping with the escaped string or the behavior is undefined
	/// @param	escapeCharacter	(optional) the escape character.
	/// @param	endDelimiter   	(optional) the end delimiter.
	///
	/// @return	An iterator to where unescaping stopped (end of the string or endDelimiter)
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

	/// @brief	Unescapes a command line argv escaped string
	///
	/// @exception	MalformedEscapedSequence	Thrown when a malformed escaped sequence is supplied
	///
	/// @param	begin		   	An iterator at the beginning of the string to escape
	/// @param	end			   	An iterator one past the end of the string to escape
	/// @param	target		   	Target iterator to write to.  Should not be overlapping with the escaped string or the behavior is undefined
	///
	/// @return	An iterator to where unescaping stopped (end of the string or endDelimiter)
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
