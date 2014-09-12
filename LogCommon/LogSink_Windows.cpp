// Copyright © Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "LogSink.hpp"
#include <stdexcept>
#include <vector>
#include <limits>
#include <windows.h>
#include "Utf8.hpp"
#include "Win32Exception.hpp"

namespace Instalog
{
    file_sink::file_sink(std::string const& filePath)
        : handleValue(reinterpret_cast<std::uintptr_t>(INVALID_HANDLE_VALUE))
    {
        std::wstring widePathSource = utf8::ToUtf16(filePath);
        std::vector<wchar_t> widePath;
        DWORD actualLength;
        while ((actualLength = ::ExpandEnvironmentStringsW(widePathSource.c_str(), widePath.data(), static_cast<DWORD>(widePath.size()))) > widePath.size())
        {
            widePath.resize(actualLength);
        }

        HANDLE hFile =
            ::CreateFileW(widePath.data(),
                          FILE_WRITE_DATA | FILE_APPEND_DATA,
                          0,
                          nullptr,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                          nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            SystemFacades::Win32Exception::ThrowFromLastError();
        }

        this->handleValue = reinterpret_cast<std::uintptr_t>(hFile);
    }

    file_sink::file_sink(file_sink&& other) BOOST_NOEXCEPT_OR_NOTHROW
        : handleValue(other.handleValue)
    {
        other.handleValue = reinterpret_cast<std::uintptr_t>(INVALID_HANDLE_VALUE);
    }

    file_sink::~file_sink() BOOST_NOEXCEPT_OR_NOTHROW
    {
        HANDLE asHandle = reinterpret_cast<HANDLE>(this->handleValue);
        if (asHandle != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(asHandle);
        }
    }

    void file_sink::append(char const* data, std::size_t dataLength)
    {
        HANDLE asHandle = reinterpret_cast<HANDLE>(this->handleValue);
        if (asHandle == INVALID_HANDLE_VALUE)
        {
            throw std::logic_error("Attempted to use a moved-from file sink.");
        }

        std::size_t maxLength = std::numeric_limits<DWORD>::max();
        if (dataLength > maxLength)
        {
            throw std::overflow_error("This append can only write the number of bytes in a DWORD at a time.");
        }

        DWORD actualLength = static_cast<DWORD>(dataLength);
        DWORD writtenLength;
        if (::WriteFile(asHandle, data, actualLength, &writtenLength, nullptr) == FALSE)
        {
            SystemFacades::Win32Exception::ThrowFromLastError();
        }

        if (writtenLength != actualLength)
        {
            throw std::length_error("Unexpected number of bytes written.");
        }
    }
}
