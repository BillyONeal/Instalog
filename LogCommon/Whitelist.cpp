// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "resource.h"
#include <functional>
#include <cstdint>
#include <algorithm>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <windows.h>
#include "Win32Exception.hpp"
#include "Whitelist.hpp"
#pragma warning(push)
#pragma warning(disable : 4512)
#pragma warning(disable : 4244)
#include "Whitelist.pb.h"
#pragma warning(pop)

using Instalog::SystemFacades::Win32Exception;

namespace Instalog
{
static HMODULE GetCurrentModule()
{
    HMODULE hModule = NULL;
    BOOL errorCheck =
        ::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCTSTR)GetCurrentModule,
                            &hModule);
    if (errorCheck == 0)
    {
        Win32Exception::ThrowFromLastError();
    }
    return hModule;
}

Whitelist::Whitelist(
    std::int32_t whitelistId,
    std::vector<std::pair<std::string, std::string>> const& replacements)
{
    using namespace std::placeholders;
    HMODULE hMod = GetCurrentModule();
    HRSRC resourceHandle =
        ::FindResource(hMod, MAKEINTRESOURCEW(whitelistId), L"WHITELIST");
    if (resourceHandle == 0)
    {
        Win32Exception::ThrowFromLastError();
    }
    HGLOBAL resourceGlobal = ::LoadResource(hMod, resourceHandle);
    if (resourceGlobal == 0)
    {
        Win32Exception::ThrowFromLastError();
    }
    void* resourceData = ::LockResource(resourceGlobal);
    if (resourceData == 0)
    {
        Win32Exception::ThrowFromLastError();
    }
    char const* resourceDataCasted =
        static_cast<char const*>(resourceData);
    DWORD resourceLen = ::SizeofResource(hMod, resourceHandle);
    auto sourceRange = boost::make_iterator_range(
        resourceDataCasted,
        resourceDataCasted + (resourceLen / sizeof(char)));
    boost::algorithm::split(
        innards, sourceRange, std::bind1st(std::equal_to<char>(), '\n'));
    std::locale loc;
    std::for_each(innards.begin(), innards.end(), [&](std::string & x) {
        boost::algorithm::to_lower(x, loc);
    });
    std::for_each(innards.begin(), innards.end(), [&](std::string & a) {
        std::for_each(replacements.begin(),
                      replacements.end(),
                      [&](std::pair<std::string, std::string> const & b) {
            if (boost::algorithm::istarts_with(a, b.first, loc))
            {
                a.replace(a.begin(), a.begin() + b.first.size(), b.second);
            }
        });
    });
    std::sort(innards.begin(), innards.end());
}

bool Whitelist::IsOnWhitelist(std::string checked) const
{
    boost::algorithm::to_lower(checked);
    return std::binary_search(innards.begin(), innards.end(), checked);
}
}
