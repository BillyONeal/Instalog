#include "pch.hpp"
#include "resource.h"
#include <functional>
#include <algorithm>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <windows.h>
#include "Win32Exception.hpp"
#include "Whitelist.hpp"

using Instalog::SystemFacades::Win32Exception;

namespace Instalog {

	Whitelist::Whitelist( __int32 whitelistId )
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
		boost::algorithm::split(innards, boost::make_iterator_range(resourceDataCasted, resourceDataCasted + (resourceLen / sizeof(wchar_t))),
			std::bind(std::equal_to<wchar_t>(), _1, L'\n'));
		std::for_each(innards.begin(), innards.end(), std::bind(boost::algorithm::to_lower<std::wstring>, _1, std::locale()));
		std::sort(innards.begin(), innards.end());
	}

	bool Whitelist::IsOnWhitelist( std::wstring const& checked ) const
	{
		std::wstring lowercased(boost::algorithm::to_lower_copy(checked));
		return std::binary_search(innards.begin(), innards.end(), lowercased);
	}

}
