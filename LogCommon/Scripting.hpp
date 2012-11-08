// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <memory>
#include <string>
#include <map>
#include <functional>
#include <vector>
#include <exception>
#include <boost/noncopyable.hpp>
#include "StringUtilities.hpp"
#include "UserInterface.hpp"

namespace Instalog
{
    /// @brief    The relative priorities of different script actions
    enum LogSectionPriorities
    {
        MEMORY,
        DISK_PERSISTENT,
        WMI,
        WHITELISTING,
        SCANNING
    };
    
    struct ISectionDefinition;

    /// @brief    Script section.
    class ScriptSection
    {
        ISectionDefinition const* targetSection;
        std::wstring argument;
        std::size_t parseIndex;

    public:
        /**
         * Gets the parse index.
         *
         * @return The parse index.
         */
        std::size_t GetParseIndex() const
        {
            return parseIndex;
        }

        /**
         * Gets the argument.
         *
         * @return The argument.
         */
        std::wstring const& GetArgument() const
        {
            return argument;
        }

        /**
         * Gets the section definition.
         *
         * @return The section definition corresponding to this section.
         */
        ISectionDefinition const& GetDefinition() const
        {
            return *targetSection;
        }

        /**
         * Constructor.
         *
         * @param section The section definition to associate with this section.
         * @param arg     The argument to this section definition.
         * @param index   Index of this script section in the source script.
         */
        ScriptSection(ISectionDefinition const* section, std::wstring arg = std::wstring(), std::size_t index = 0)
            : targetSection(section)
            , argument(std::move(arg))
            , parseIndex(index)
        { }

        /**
         * Comparison operator for priority ordering.
         *
         * @param rhs The ScriptSection to compare to.
         *
         * @return true if this should be ordered before rhs.
         */
        bool operator<(const ScriptSection& rhs) const;
    };

    /// @brief    Section definition.
    struct ISectionDefinition
    {
        /// @brief    Destructor.
        virtual ~ISectionDefinition() {}

        /// @brief    Gets the script command that is used in a script
        ///
        /// @return    The script command.
        virtual std::wstring GetScriptCommand() const = 0;

        /// @brief    Gets the human-friendly name of the script section.
        ///
        /// @return    The name.
        virtual std::wstring GetName() const = 0;

        /// @brief    Gets the priority.
        ///
        /// @return    The priority.
        virtual LogSectionPriorities GetPriority() const = 0;

        virtual void Execute(std::wostream& logOutput, ScriptSection const& sectionData, std::vector<std::wstring> const& options) const = 0;
    };

    class Script;

    /// @brief    Handles script sections and parses scripts
    class ScriptParser : boost::noncopyable
    {
        std::map<std::wstring, std::unique_ptr<ISectionDefinition>> sectionTypes;
    public:
        /// @brief    Adds a section type.
        ///
        /// @param    sectionTypeToAdd    The section type to add.
        void AddSectionDefinition(std::unique_ptr<ISectionDefinition> sectionTypeToAdd);

        /// @brief    Parses a given script
        ///
        /// @param    script    The script.
        ///
        /// @return    Script to run
        /// 
        /// @exception UnknownScriptSectionException Thrown if an unknown script section is supplied
        Script Parse(std::wstring const& script) const;
    };

    /// @brief    Script that can be run
    class Script
    {
        ScriptParser const* parent_;
        std::map<ScriptSection, std::vector<std::wstring>> sections;
    public:
        /// @brief    Constructor.
        ///
        /// @param    parent    The ScriptDispatcher that constructed this Script
        Script(ScriptParser const* parent);

        /// @brief    Gets the sections.
        ///
        /// @return    The sections.
        std::map<ScriptSection, std::vector<std::wstring>> const& GetSections() const;

        /// @brief    Adds a ISectionDefinition with given arguments and options
        ///
        /// @param    def           The section definition to add.
        /// @param    arg           The arguments to the section.
        /// @param    options    Options (lines) supplied to the section
        void Add(ISectionDefinition const* def, std::wstring const& arg, std::vector<std::wstring> const& options, std::size_t index);

        /// @brief    Runs the script
        ///
        /// @param [out]    logOutput    Stream to output log to 
        /// @param [out]    ui             The UI to send messages to
        void Run(std::wostream& logOutput, IUserInterface *ui) const;
    };

    /// @brief    Thrown when an unknown script section is encountered
    class UnknownScriptSectionException : public std::exception
    {
        std::string unknown;
    public:
        UnknownScriptSectionException(std::wstring& sectionTitle)
            : unknown(ConvertUnicode(sectionTitle) + " is not a known script section type.")
        { }
        virtual char const* what() const
        {
            return unknown.c_str();
        }
    };
}
