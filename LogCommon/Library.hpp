// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <string>
#include <vector>
#include "ErrorReporter.hpp"

namespace Instalog
{

enum class load_type
{
    load_all,
    load_data_only
};

class library
{
    void* hModule;
    void destroy();
    typedef void(*get_proc_address_result)();
    static get_proc_address_result get_function_impl(
        void* hMod,
        IErrorReporter& errorReporter,
        char const* functionName
        );
public:
    library();
    explicit library(void* hModule);
    library(library const&) = delete;
    library& operator=(library const&) = delete;

    library(library&& other);
    library& operator=(library&& other);

    ~library();

    /// @brief    Gets a function pointer to the specified function
    ///
    /// @param    functionName    Name of the function.
    ///
    /// @return    Function pointer
    template <typename FuncT> FuncT get_function(
        IErrorReporter& errorReporter,
        char const* functionName
        )
    {
        return reinterpret_cast<FuncT>(get_function_impl(
            this->hModule,
            errorReporter,
            functionName
            ));
    }

    /// @brief    Constructor, opens a handle to the library with the given
    /// flags
    ///
    /// @param    filename    Path of the library.
    /// @param    flags       The flags.
    void open(
        IErrorReporter& errorReporter,
        boost::string_ref filename,
        load_type loadType
        );

    std::string get_formatted_message(
        IErrorReporter& errorReporter,
        std::uint32_t messageId
        );
    
    /// @brief    Gets a formatted message with the given id and arguments to
    /// replace
    ///
    /// @param    messageId    Identifier for the message.
    /// @param    arguments    (optional) the arguments.
    ///
    /// @return    The formatted message.
    std::string get_formatted_message(
        IErrorReporter& errorReporter,
        std::uint32_t messageId,
        std::vector<std::wstring> const& arguments
        );

    /// @brief    Gets a formatted message with the given id and arguments to
    /// replace
    ///
    /// @param    messageId    Identifier for the message.
    /// @param    arguments    (optional) the arguments.
    ///
    /// @return    The formatted message.
    std::string get_formatted_message(
        IErrorReporter& errorReporter,
        std::uint32_t messageId,
        std::vector<std::string> const& arguments
        );

    /// @brief    Gets the Windows NT dll
    ///
    /// @return    The Windows NT dll
    static library& ntdll();

    static library& kernel32();
};

}