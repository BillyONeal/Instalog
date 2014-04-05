// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "Library.hpp"
#include <Windows.h>
#include "Utf8.hpp"

namespace Instalog
{

void library::destroy()
{
    if (this->hModule != nullptr)
    {
        ::FreeLibrary(static_cast<HMODULE>(this->hModule));
    }
}

library::library() : hModule(nullptr)
{}

library::library(void* hModule) : hModule(hModule)
{}

library::library(library&& other)
    : hModule(other.hModule)
{
    other.hModule = nullptr;
}

library& library::operator=(library&& other)
{
    this->destroy();
    this->hModule = other.hModule;
    other.hModule = nullptr;
    return *this;
}

library::~library()
{
    this->destroy();
}

library::get_proc_address_result library::get_function_impl(
    void* hMod,
    IErrorReporter& errorReporter,
    char const* functionName
    )
{
    get_proc_address_result result = reinterpret_cast<get_proc_address_result>(
        ::GetProcAddress(static_cast<HMODULE>(hMod), functionName));
    if (result == nullptr)
    {
        errorReporter.ReportWinError(::GetLastError(), "GetProcAddress");
    }

    return result;
}

void library::open(
    IErrorReporter& errorReporter,
    boost::string_ref filename,
    load_type loadType
    )
{
    DWORD flags;
    switch (loadType)
    {
    case load_type::load_all:
        flags = 0;
        break;
    case load_type::load_data_only:
        flags = LOAD_LIBRARY_AS_DATAFILE;
        break;
    }

    std::wstring expanded(utf8::ToUtf16(filename));
    HMODULE loadedModule = ::LoadLibraryExW(expanded.c_str(), NULL, flags);
    if (loadedModule == NULL)
    {
        errorReporter.ReportWinError(::GetLastError(), "LoadLibraryExW");
    }
    else
    {
        this->destroy();
        this->hModule = loadedModule;
    }
}

static library create_module_library(wchar_t const* lib)
{
    return static_cast<library>(static_cast<void*>(
        ::GetModuleHandleW(lib)));
}

library& library::ntdll()
{
    static library nt = create_module_library(L"ntdll.dll");
    return nt;
}

library& library::kernel32()
{
    static library kernel32 = create_module_library(L"kernel32.dll");
    return kernel32;
}

struct local_free
{
    void operator()(void* hLocal)
    {
        LocalFree(static_cast<HLOCAL>(hLocal));
    }
};

static std::string FormatMessageU(
    IErrorReporter& errorReporter,
    void* hModule,
    DWORD messageId,
    va_list* argPtr
    )
{
    std::string answer;
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
        errorReporter.ReportWinError(::GetLastError(), "FormatMessageW");
    }
    else
    {
        std::unique_ptr<void, local_free> destroyer(messagePtr);
        answer = utf8::ToUtf8(messagePtr);
    }

    return answer;
}

std::string library::get_formatted_message(
    IErrorReporter& errorReporter,
    std::uint32_t messageId
    )
{
    return FormatMessageU(errorReporter, this->hModule, messageId, nullptr);
}

std::string library::get_formatted_message(
    IErrorReporter& errorReporter,
    std::uint32_t messageId,
    std::vector<std::string> const& argumentsSource)
{
    std::vector<std::wstring> arguments;
    arguments.reserve(argumentsSource.size());
    std::transform(argumentsSource.cbegin(), argumentsSource.cend(), std::back_inserter(arguments), [](std::string const& s) { return utf8::ToUtf16(s); });
    return get_formatted_message(errorReporter, messageId, arguments);
}

std::string library::get_formatted_message(
    IErrorReporter& errorReporter,
    std::uint32_t messageId,
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

    return FormatMessageU(errorReporter, this->hModule, messageId, argPtr);
}
}
