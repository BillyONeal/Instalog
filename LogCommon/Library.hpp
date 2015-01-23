// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include <boost/noncopyable.hpp>
#include "ErrorReporter.hpp"

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
    /// @param    reporter    Error reporting strategy to use. If the error reporting strategy does
    ///                       not throw an exception, then the library will be placed in an invalid state.
    /// @param    filename    Path of the library.
    /// @param    flags       The flags to use in the call to LoadLibraryExW.
    Library(IErrorReporter& reporter, std::string const& filename, DWORD flags);

    /// @brief    Destructor, frees the library.
    ~Library();

    /// @brief    Throw std::invalid_argument if the instance is not valid.
    void RequireValid() const;

public:
    /// @brief    Returns true if this instance of Library is usable. Otherwise; returns false.
    bool Valid() const
    {
        return this->hModule != nullptr;
    }
};

/// @brief    An easy runtime dynamic linker
class RuntimeDynamicLinker : public Library
{
    public:
    /// @brief    Constructor.
    ///
    /// @param    reporter    Error reporting strategy to use. If the error reporting strategy does
    ///                       not throw an exception, then the library will be placed in an invalid state.
    /// @param    filename    Filename of the library.
    RuntimeDynamicLinker(IErrorReporter& reporter, std::string const& filename);

    /// @brief    Gets a function pointer to the specified function
    ///
    /// @param    reporter        The error reporting strategy to use. If the function fails and the reporter
    ///                           does not throw an exception, returns nullptr.
    /// @param    functionName    Name of the function.
    ///
    /// @return    Function pointer requested; or nullptr on failure.
    template <typename FuncT> FuncT GetProcAddress(IErrorReporter& reporter, char const* functionName)
    {
        FuncT answer =
            reinterpret_cast<FuncT>(::GetProcAddress(hModule, functionName));
        if (!answer)
        {
            reporter.ReportLastWinError("GetProcAddress");
        }
        return answer;
    }
};

/// @brief    Gets the Windows NT dll
///
/// @return    The Windows NT dll
RuntimeDynamicLinker& GetNtDll();

/// @brief    Gets the Kernel32 dll
///
/// @return    The Kernel32 dll
RuntimeDynamicLinker& GetKernel32();

class FormattedMessageLoader : public Library
{
    public:
    /// @brief    Constructor.
    ///
    /// @param    reporter    Error reporting strategy to use. If the error reporting strategy does
    ///                       not throw an exception, then the library will be placed in an invalid state.
    /// @param    filename    Filename of the library containing the messages.
    FormattedMessageLoader(IErrorReporter& reporter, std::string const& filename);

    /// @brief    Gets a formatted message with the given id and arguments to
    /// replace
    ///
    /// @param    reporter    Error reporting strategy to use. If the error reporting strategy does
    ///                       not throw an exception, then the library will be placed in an invalid state.
    /// @param    messageId    Identifier for the message.
    ///
    /// @return    The formatted message.
    std::string GetFormattedMessage(IErrorReporter& reporter, DWORD messageId);

    /// @brief    Gets a formatted message with the given id and arguments to
    /// replace
    ///
    /// @param    reporter    Error reporting strategy to use. If the error reporting strategy does
    ///                       not throw an exception, then the library will be placed in an invalid state.
    /// @param    messageId    Identifier for the message.
    /// @param    arguments    (optional) the arguments.
    ///
    /// @return    The formatted message.
    std::string
        GetFormattedMessage(IErrorReporter& reporter, DWORD messageId,
        std::vector<std::wstring> const& arguments);

    /// @brief    Gets a formatted message with the given id and arguments to
    /// replace
    ///
    /// @param    reporter    Error reporting strategy to use. If the error reporting strategy does
    ///                       not throw an exception, then the library will be placed in an invalid state.
    /// @param    messageId    Identifier for the message.
    /// @param    arguments    (optional) the arguments.
    ///
    /// @return    The formatted message.
    std::string
    GetFormattedMessage(IErrorReporter& reporter, DWORD messageId,
                        std::vector<std::string> const& arguments);
};
}
}
