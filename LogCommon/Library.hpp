// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include <boost/noncopyable.hpp>
#include "Win32Exception.hpp"

namespace Instalog
{
namespace SystemFacades
{

class Library : boost::noncopyable
{
    protected:
    /// @summary    The module handle.
    HMODULE hModule;

    /// @brief    Constructor, opens a handle to the library with the given
    /// flags
    ///
    /// @param    filename    Path of the library.
    /// @param    flags       The flags.
    Library(std::string const& filename, DWORD flags);

    /// @brief    Destructor, frees the library.
    ~Library();
};

/// @brief    An easy runtime dynamic linker
class RuntimeDynamicLinker : public Library
{
    public:
    /// @brief    Constructor.
    ///
    /// @param    filename    Filename of the library.
    RuntimeDynamicLinker(std::string const& filename);

    /// @brief    Gets a function pointer to the specified function
    ///
    /// @param    functionName    Name of the function.
    ///
    /// @return    Function pointer
    template <typename FuncT> FuncT GetProcAddress(char const* functionName)
    {
        FuncT answer =
            reinterpret_cast<FuncT>(::GetProcAddress(hModule, functionName));
        if (!answer)
        {
            Win32Exception::ThrowFromLastError();
        }
        return answer;
    }
};

/// @brief    Gets the Windows NT dll
///
/// @return    The Windows NT dll
RuntimeDynamicLinker& GetNtDll();

class FormattedMessageLoader : public Library
{
    public:
    /// @brief    Constructor.
    ///
    /// @param    filename    Filename of the library containing the messages.
    FormattedMessageLoader(std::string const& filename);

    std::string GetFormattedMessage(DWORD messageId);

    /// @brief    Gets a formatted message with the given id and arguments to
    /// replace
    ///
    /// @param    messageId    Identifier for the message.
    /// @param    arguments    (optional) the arguments.
    ///
    /// @return    The formatted message.
    std::string
        GetFormattedMessage(DWORD messageId,
        std::vector<std::wstring> const& arguments);

    /// @brief    Gets a formatted message with the given id and arguments to
    /// replace
    ///
    /// @param    messageId    Identifier for the message.
    /// @param    arguments    (optional) the arguments.
    ///
    /// @return    The formatted message.
    std::string
    GetFormattedMessage(DWORD messageId,
                        std::vector<std::string> const& arguments);
};
}
}
