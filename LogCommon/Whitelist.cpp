// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "resource.h"
#include <functional>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <windows.h>
#include "Win32Exception.hpp"
#include "Whitelist.hpp"

using Instalog::SystemFacades::Win32Exception;

namespace Instalog {

	Whitelist::Whitelist( __int32 whitelistId , std::vector<std::pair<std::wstring, std::wstring>> const& replacements )
	{
		using namespace std::placeholders;

		HRSRC resourceHandle = ::FindResource(0, MAKEINTRESOURCEW(whitelistId), L"WHITELIST");
		if (resourceHandle == 0)
		{
			Win32Exception::ThrowFromLastError();
		}
		HGLOBAL resourceGlobal = ::LoadResource(0, resourceHandle);
		if (resourceGlobal == 0)
		{
			Win32Exception::ThrowFromLastError();
		}
		void * resourceData = ::LockResource(resourceGlobal);
		if (resourceData == 0)
		{
			Win32Exception::ThrowFromLastError();
		}
		wchar_t const* resourceDataCasted = static_cast<wchar_t const*>(resourceData);
		DWORD resourceLen = ::SizeofResource(0, resourceHandle);
		auto sourceRange = boost::make_iterator_range(resourceDataCasted, resourceDataCasted + (resourceLen / sizeof(wchar_t)));
		boost::algorithm::split(innards, sourceRange,
			std::bind(std::equal_to<wchar_t>(), _1, L'\n'));
		std::for_each(innards.begin(), innards.end(), std::bind(boost::algorithm::to_lower<std::wstring>, _1, std::locale()));
		std::for_each(innards.begin(), innards.end(), [&replacements] (std::wstring &a) {
			std::for_each(replacements.begin(), replacements.end(), [&a] (std::pair<std::wstring, std::wstring> const&b) {
				if (boost::algorithm::starts_with(a, b.first))
				{
					a.replace(a.begin(), a.begin() + b.first.size(), b.second);
				}
			});
		});
		std::sort(innards.begin(), innards.end());
	}

	bool Whitelist::IsOnWhitelist( std::wstring checked ) const
	{
		boost::algorithm::to_lower(checked);
		return std::binary_search(innards.begin(), innards.end(), checked);
	}

	void Whitelist::PrintAll( std::wostream & str ) const
	{
		std::copy(innards.begin(), innards.end(), std::ostream_iterator<std::wstring, wchar_t>(str, L"\n"));
	}

}
