// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "Win32Exception.hpp"
#include "Library.hpp"
#include "Utf8.hpp"

namespace Instalog
{
namespace SystemFacades
{

Library::Library(std::string const& filename, DWORD flags)
    : hModule(::LoadLibraryExW(utf8::ToUtf16(filename).c_str(), NULL, flags))
{
    if (hModule == NULL)
    {
        Win32Exception::ThrowFromLastError();
    }
}

Library::~Library()
{
    ::FreeLibrary(hModule);
}

RuntimeDynamicLinker& GetNtDll()
{
    static RuntimeDynamicLinker ntdll("ntdll.dll");
    return ntdll;
}

RuntimeDynamicLinker::RuntimeDynamicLinker(std::string const& filename)
    : Library(filename, 0)
{
}

static bool IsVistaLater()
{
    OSVERSIONINFOW info;
    info.dwOSVersionInfoSize = sizeof(info);
    ::GetVersionExW(&info);
    return info.dwMajorVersion >= 6;
}

static bool IsVistaLaterCache()
{
    static bool isVistaLater = IsVistaLater();
    return isVistaLater;
}

static std::string FormatMessageU(HMODULE hModule, DWORD messageId, va_list* argPtr)
{
    wchar_t* messagePtr = nullptr;
    if (FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE |
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        hModule,
        messageId,
        LANG_SYSTEM_DEFAULT,
        reinterpret_cast<LPWSTR>(&messagePtr),
        0,
        argPtr) == 0)
    {
        Win32Exception::ThrowFromLastError();
    }
    std::string answer(utf8::ToUtf8(messagePtr));
    LocalFree(messagePtr);
    return answer;
}

FormattedMessageLoader::FormattedMessageLoader(std::string const& filename)
    : Library(filename,
              (IsVistaLaterCache() ? LOAD_LIBRARY_AS_IMAGE_RESOURCE : 0) |
                  LOAD_LIBRARY_AS_DATAFILE)
{
}

std::string FormattedMessageLoader::GetFormattedMessage(
    DWORD messageId)
{
    return FormatMessageU(this->hModule, messageId, nullptr);
}

std::string FormattedMessageLoader::GetFormattedMessage(
    DWORD messageId,
    std::vector<std::string> const& argumentsSource)
{
    std::vector<std::wstring> arguments;
    arguments.reserve(argumentsSource.size());
    std::transform(argumentsSource.cbegin(), argumentsSource.cend(), std::back_inserter(arguments), [](std::string const& s) { return utf8::ToUtf16(s); });
    return GetFormattedMessage(messageId, arguments);
}

std::string FormattedMessageLoader::GetFormattedMessage(
    DWORD messageId,
    std::vector<std::wstring> const& arguments)
{
    std::vector<DWORD_PTR> argumentPtrs;
    argumentPtrs.reserve(arguments.size());
    std::transform(arguments.cbegin(), arguments.cend(), std::back_inserter(argumentPtrs),
        [](std::wstring const& str) { return reinterpret_cast<DWORD_PTR>(str.c_str()); });

    auto argPtr = reinterpret_cast<va_list*>(argumentPtrs.data());
    if (arguments.empty())
    {
        argPtr = nullptr;
    }

    return FormatMessageU(this->hModule, messageId, argPtr);
}
}
}
