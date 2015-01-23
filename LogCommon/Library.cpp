// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include <algorithm>
#include <stdexcept>
#include "Library.hpp"
#include "ScopeExit.hpp"
#include "Utf8.hpp"

namespace Instalog
{
namespace SystemFacades
{

Library::Library(IErrorReporter& reporter, std::string const& filename, DWORD flags)
    : hModule(::LoadLibraryExW(utf8::ToUtf16(filename).c_str(), nullptr, flags))
{
    if (!this->Valid())
    {
        reporter.ReportLastWinError("LoadLibraryExW");
    }
}

Library::~Library()
{
    if (this->Valid())
    {
        ::FreeLibrary(hModule);
    }
}

void Library::RequireValid() const
{
    if (!this->Valid())
    {
        throw std::invalid_argument("Invalid library state.");
    }
}

RuntimeDynamicLinker& GetNtDll()
{
    static RuntimeDynamicLinker ntdll(GetThrowingErrorReporter(), "ntdll.dll");
    return ntdll;
}

RuntimeDynamicLinker& GetKernel32()
{
    static RuntimeDynamicLinker kernel32(GetThrowingErrorReporter(), "kernel32.dll");
    return kernel32;
}

RuntimeDynamicLinker::RuntimeDynamicLinker(IErrorReporter& reporter, std::string const& filename)
    : Library(reporter, filename, 0)
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

static std::string FormatMessageU(IErrorReporter& reporter, HMODULE hModule, DWORD messageId, va_list* argPtr)
{
    wchar_t* messagePtr = nullptr;
    ScopeExit onExit([&]() {
        if (messagePtr)
        {
            LocalFree(messagePtr);
        }}
    );
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
        reporter.ReportLastWinError("FormatMessageW");
        messagePtr = nullptr;
    }

    return utf8::ToUtf8(messagePtr);
}

FormattedMessageLoader::FormattedMessageLoader(IErrorReporter& reporter, std::string const& filename)
    : Library(reporter, filename,
              (IsVistaLaterCache() ? LOAD_LIBRARY_AS_IMAGE_RESOURCE : 0) |
                  LOAD_LIBRARY_AS_DATAFILE)
{
}

std::string FormattedMessageLoader::GetFormattedMessage(
    IErrorReporter& reporter,
    DWORD messageId)
{
    this->RequireValid();
    return FormatMessageU(reporter, this->hModule, messageId, nullptr);
}

std::string FormattedMessageLoader::GetFormattedMessage(
    IErrorReporter& reporter,
    DWORD messageId,
    std::vector<std::string> const& argumentsSource)
{
    this->RequireValid();
    std::vector<std::wstring> arguments;
    arguments.reserve(argumentsSource.size());
    std::transform(argumentsSource.cbegin(), argumentsSource.cend(), std::back_inserter(arguments), [](std::string const& s) { return utf8::ToUtf16(s); });
    return GetFormattedMessage(reporter, messageId, arguments);
}

std::string FormattedMessageLoader::GetFormattedMessage(
    IErrorReporter& reporter,
    DWORD messageId,
    std::vector<std::wstring> const& arguments)
{
    this->RequireValid();
    std::vector<DWORD_PTR> argumentPtrs;
    argumentPtrs.reserve(arguments.size());
    std::transform(arguments.cbegin(), arguments.cend(), std::back_inserter(argumentPtrs),
        [](std::wstring const& str) { return reinterpret_cast<DWORD_PTR>(str.c_str()); });

    auto argPtr = reinterpret_cast<va_list*>(argumentPtrs.data());
    if (arguments.empty())
    {
        argPtr = nullptr;
    }

    return FormatMessageU(reporter, this->hModule, messageId, argPtr);
}
}
}
