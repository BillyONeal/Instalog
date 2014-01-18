// Copyright © Jacob Snyder, Billy O'Neal III
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
#include "LogSink.hpp"

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
    std::string argument;
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
    std::string const& GetArgument() const
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
    ScriptSection(ISectionDefinition const* section, std::string arg = std::string(), std::size_t index = 0)
            : targetSection(section)
            , argument(std::move(arg))
            , parseIndex(index)
    {
    }

    /**
     * Comparison operator for priority ordering.
     *
     * @param rhs The ScriptSection to compare to.
     *
     * @return true if this should be ordered before rhs.
     */
    bool operator<(const ScriptSection& rhs) const;
};

#pragma warning(push)
#pragma warning(disable: 4512) // Assignment operator could not be generated.
struct ExecutionOptions
{
    log_sink& logOutput;
    ScriptSection const& sectionData;
    std::vector<std::string> const& options;
    ExecutionOptions(log_sink& logOutput_, ScriptSection const& sectionData_, std::vector<std::string> const& options_);
    ExecutionOptions(ExecutionOptions const& toCopy);
};
#pragma warning(pop)

/// @brief    Section definition.
struct ISectionDefinition
{
    /// @brief    Destructor.
    virtual ~ISectionDefinition()
    {
    }

    /// @brief    Gets the script command that is used in a script
    ///
    /// @return    The script command.
    virtual std::string GetScriptCommand() const = 0;

    /// @brief    Gets the human-friendly name of the script section.
    ///
    /// @return    The name.
    virtual std::string GetName() const = 0;

    /// @brief    Gets the priority.
    ///
    /// @return    The priority.
    virtual LogSectionPriorities GetPriority() const = 0;

    virtual void Execute(ExecutionOptions options) const = 0;
};

class Script;

/// @brief    Handles script sections and parses scripts
class ScriptParser : boost::noncopyable
{
    std::map<std::string, std::unique_ptr<ISectionDefinition>> sectionTypes;

    public:
    /// @brief    Adds a section type.
    ///
    /// @param    sectionTypeToAdd    The section type to add.
    void
    AddSectionDefinition(std::unique_ptr<ISectionDefinition> sectionTypeToAdd);

    /// @brief    Parses a given script
    ///
    /// @param    script    The script.
    ///
    /// @return    Script to run
    ///
    /// @exception UnknownScriptSectionException Thrown if an unknown script
    /// section is supplied
    Script Parse(std::string const& script) const;
};

/// @brief    Script that can be run
class Script
{
    ScriptParser const* parent_;
    std::map<ScriptSection, std::vector<std::string>> sections;

    public:
    /// @brief    Constructor.
    ///
    /// @param    parent    The ScriptDispatcher that constructed this Script
    Script(ScriptParser const* parent);

    /// @brief    Gets the sections.
    ///
    /// @return    The sections.
    std::map<ScriptSection, std::vector<std::string>> const&
    GetSections() const;

    /// @brief    Adds a ISectionDefinition with given arguments and options
    ///
    /// @param    def           The section definition to add.
    /// @param    arg           The arguments to the section.
    /// @param    options    Options (lines) supplied to the section
    void Add(ISectionDefinition const* def,
             std::string const& arg,
             std::vector<std::string> const& options,
             std::size_t index);

    /// @brief    Runs the script
    ///
    /// @param [out]    logOutput    Stream to output log to
    /// @param [out]    ui             The UI to send messages to
    void Run(log_sink& logOutput, IUserInterface* ui) const;
};

/// @brief    Thrown when an unknown script section is encountered
class UnknownScriptSectionException : public std::exception
{
    std::string unknown;
    public:
    UnknownScriptSectionException(std::string& sectionTitle)
        : unknown(sectionTitle + " is not a known script section type.")
    {
    }
    virtual char const* what() const
    {
        return unknown.c_str();
    }
};
}
