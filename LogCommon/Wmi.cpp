// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "Win32Exception.hpp"
#include "Wmi.hpp"

namespace Instalog
{
namespace SystemFacades
{

UniqueComPtr<IWbemServices> GetWbemServices()
{
    auto locator = UniqueComPtr<IWbemLocator>::Create(CLSID_WbemLocator,
                                                      CLSCTX_INPROC_SERVER);
    UniqueComPtr<IWbemServices> wbemServices;
    ThrowIfFailed(locator->ConnectServer(
        BSTR(L"ROOT"), 0, 0, 0, 0, 0, 0, wbemServices.PassAsOutParameter()));
    ThrowIfFailed(CoSetProxyBlanket(wbemServices.Get(),
                                    RPC_C_AUTHN_WINNT,
                                    RPC_C_AUTHZ_NONE,
                                    0,
                                    RPC_C_AUTHN_LEVEL_DEFAULT,
                                    RPC_C_IMP_LEVEL_IMPERSONATE,
                                    0,
                                    EOAC_NONE));
    return wbemServices;
}

FILETIME WmiDateStringToFiletime(std::wstring const& datestring)
{
    SYSTEMTIME systemTime;
    systemTime.wYear =
        static_cast<WORD>(_wtoi(datestring.substr(0, 4).c_str()));
    systemTime.wMonth =
        static_cast<WORD>(_wtoi(datestring.substr(4, 2).c_str()));
    systemTime.wDay = static_cast<WORD>(_wtoi(datestring.substr(6, 2).c_str()));
    systemTime.wHour =
        static_cast<WORD>(_wtoi(datestring.substr(8, 2).c_str()));
    systemTime.wMinute =
        static_cast<WORD>(_wtoi(datestring.substr(10, 2).c_str()));
    systemTime.wSecond =
        static_cast<WORD>(_wtoi(datestring.substr(12, 2).c_str()));
    systemTime.wMilliseconds =
        static_cast<WORD>(_wtoi(datestring.substr(15, 3).c_str()));

    FILETIME fileTime;
    if (SystemTimeToFileTime(&systemTime, &fileTime) == false)
    {
        Win32Exception::ThrowFromLastError();
    }

    ULARGE_INTEGER intTime;
    intTime.LowPart = fileTime.dwLowDateTime;
    intTime.HighPart = fileTime.dwHighDateTime;

    if (datestring[21] == L'-')
    {
        intTime.QuadPart -=
            static_cast<WORD>(_wtoi(datestring.substr(22, 3).c_str()));
    }
    else if (datestring[21] == L'+')
    {
        intTime.QuadPart +=
            static_cast<WORD>(_wtoi(datestring.substr(22, 3).c_str()));
    }

    FILETIME utcFileTime;
    utcFileTime.dwLowDateTime = intTime.LowPart;
    utcFileTime.dwHighDateTime = intTime.HighPart;

    return utcFileTime;
}
}
}