// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include <clocale>
#include <algorithm>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "Scripting.hpp"
#include "StockOutputFormats.hpp"
#include "StringUtilities.hpp"

using namespace boost::algorithm;

namespace Instalog
{

void ScriptParser::AddSectionDefinition(
    std::unique_ptr<ISectionDefinition> sectionTypeToAdd)
{
    std::string scriptCommand(sectionTypeToAdd->GetScriptCommand());
    to_lower(scriptCommand);
    sectionTypes.emplace(std::move(scriptCommand), std::move(sectionTypeToAdd));
}

Script ScriptParser::Parse(std::string const& script) const
{
    std::size_t index = 0;
    Script result(this);
    std::vector<std::string> scriptLines;
    split(scriptLines, script, is_any_of("\r\n"), token_compress_on);

    scriptLines.erase(
        std::remove_if(scriptLines.begin(),
                       scriptLines.end(),
                           [](std::string const & a)->bool {
            return std::all_of(a.cbegin(), a.cend(), isspace);
        }),
        scriptLines.end());

    auto begin = scriptLines.begin();
    auto end = scriptLines.end();
    while (begin != end)
    {
        if (!starts_with(*begin, ":"))
        {
            ++begin;
            continue;
        }
        std::string::iterator sectionStart = begin->begin();
        if (sectionStart != begin->end())
        {
            ++sectionStart;
        }
        std::string::iterator argumentStart =
            std::find_if(sectionStart, begin->end(), isspace);
        std::string scriptTarget(sectionStart, argumentStart);
        to_lower(scriptTarget);

        auto sectionIterator = sectionTypes.find(scriptTarget);
        if (sectionIterator == sectionTypes.end())
        {
            throw UnknownScriptSectionException(scriptTarget);
        }
        ISectionDefinition const* def = sectionIterator->second.get();

        if (argumentStart != begin->end())
        {
            ++argumentStart;
        }
        std::string argument(argumentStart, begin->end());
        ++begin;
        std::vector<std::string>::iterator endOfOptions = begin;
        endOfOptions = std::find_if(begin, end, [](std::string const & a) {
            return (!a.empty()) && a[0] == ':';
        });
        result.Add(def,
                   argument,
                   std::vector<std::string>(begin, endOfOptions),
                   index++);
    }
    return result;
}

bool ScriptSection::operator<(const ScriptSection& rhs) const
{
    if (targetSection->GetName() != rhs.targetSection->GetName())
    {
        return targetSection->GetName() < rhs.targetSection->GetName();
    }
    return argument < rhs.argument;
}

Script::Script(ScriptParser const* parent) : parent_(parent)
{
}

void Script::Add(ISectionDefinition const* def,
                 std::string const& arg,
                 std::vector<std::string> const& options,
                 std::size_t index)
{
    ScriptSection ss(def, arg, 0);
    auto insertPoint = sections.find(ss);
    if (insertPoint == sections.end())
    {
        sections.emplace(ScriptSection(def, arg, index), options);
    }
    else
    {
        insertPoint->second.insert(
            insertPoint->second.end(), options.begin(), options.end());
    }
}

std::map<ScriptSection, std::vector<std::string>> const&
Script::GetSections() const
{
    return sections;
}

void Script::Run(log_sink& logOutput, IUserInterface* ui) const
{
    ui->LogMessage("Starting Execution");
    auto startTime = Instalog::GetLocalTime();
    WriteScriptHeader(logOutput, startTime);
    typedef std::pair<ScriptSection, std::vector<std::string>> contained;
    auto cmp = [](contained const & lhs, contained const & rhs)->bool
    {
        auto const& lhsDef = lhs.first.GetDefinition();
        auto const& rhsDef = rhs.first.GetDefinition();
        if (lhsDef.GetPriority() != rhsDef.GetPriority())
        {
            return lhsDef.GetPriority() < rhsDef.GetPriority();
        }
        return lhs.first.GetParseIndex() < rhs.first.GetParseIndex();
    }
    ;
    std::vector<contained> sectionVec(sections.cbegin(), sections.cend());
    std::stable_sort(sectionVec.begin(), sectionVec.end(), cmp);
    for (auto& entry : sectionVec)
    {
        auto header = entry.first.GetDefinition().GetName();
        ui->LogMessage("Executing " + header);
        Instalog::Header(header);
        writeln(logOutput);
        writeln(logOutput, header);
        writeln(logOutput);
        ExecutionOptions options(logOutput, entry.first, entry.second);
        entry.first.GetDefinition().Execute(options);
    }

    writeln(logOutput);
    WriteScriptFooter(logOutput, startTime);
    ui->ReportFinished();
}

ExecutionOptions::ExecutionOptions(log_sink& logOutput_, ScriptSection const& sectionData_, std::vector<std::string> const& options_)
    : logOutput(&logOutput_)
    , sectionData(&sectionData_)
    , options(&options_)
{
}

log_sink& ExecutionOptions::GetOutput()
{
    return *this->logOutput;
}

ScriptSection const& ExecutionOptions::GetSectionData() const
{
    return *this->sectionData;
}

std::vector<std::string> const& ExecutionOptions::GetOptions() const
{
    return *this->options;
}

}
