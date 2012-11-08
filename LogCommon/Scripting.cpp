// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <clocale>
#include <functional>
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

    void ScriptParser::AddSectionDefinition( std::unique_ptr<ISectionDefinition> sectionTypeToAdd )
    {
        std::wstring scriptCommand(sectionTypeToAdd->GetScriptCommand());
        to_lower(scriptCommand);
        sectionTypes.emplace(std::make_pair(std::move(scriptCommand), std::move(sectionTypeToAdd)));
    }

    Script ScriptParser::Parse( std::wstring const& script ) const
    {
        std::size_t index = 0;
        Script result(this);
        std::vector<std::wstring> scriptLines;
        split(scriptLines, script, is_any_of(L"\r\n"), token_compress_on);

        scriptLines.erase(std::remove_if(scriptLines.begin(), scriptLines.end(), [](std::wstring const& a) -> bool {
            return std::find_if(a.begin(), a.end(), std::not1(std::ptr_fun(iswspace))) == a.end();
        }), scriptLines.end());

        std::vector<std::wstring>::iterator begin, end;
        begin = scriptLines.begin();
        end = scriptLines.end();
        while (begin != end)
        {
            if (!starts_with(*begin, L":"))
            {
                ++begin;
                continue;
            }
            std::wstring::iterator sectionStart = begin->begin();
            if (sectionStart != begin->end())
            {
                ++sectionStart;
            }
            std::wstring::iterator argumentStart = std::find_if(sectionStart, begin->end(), iswspace);
            std::wstring scriptTarget(sectionStart, argumentStart);
            to_lower(scriptTarget);

            auto sectionIterator = sectionTypes.find(scriptTarget);
            if (sectionIterator == sectionTypes.end())
            {
                throw UnknownScriptSectionException(scriptTarget); 
            }
            ISectionDefinition const *def = sectionIterator->second.get();

            if (argumentStart != begin->end())
            {
                ++argumentStart;
            }
            std::wstring argument(argumentStart, begin->end());
            ++begin;
            std::vector<std::wstring>::iterator endOfOptions = begin;
            endOfOptions = std::find_if(begin, end, [](std::wstring const& a) { return starts_with(a, L":"); });
            result.Add(def, argument, std::vector<std::wstring>(begin, endOfOptions), index++);
        }
        return result;
    }

    bool ScriptSection::operator<( const ScriptSection& rhs ) const
    {
        if (targetSection->GetName() != rhs.targetSection->GetName())
        {
            return targetSection->GetName() < rhs.targetSection->GetName();
        }
        return argument < rhs.argument;
    }


    Script::Script( ScriptParser const* parent )
        : parent_(parent)
    { }

    void Script::Add( ISectionDefinition const* def, std::wstring const& arg, std::vector<std::wstring> const& options, std::size_t index )
    {
        ScriptSection ss(def, arg, 0);
        auto insertPoint = sections.find(ss);
        if (insertPoint == sections.end())
        {
            sections.insert(std::make_pair(ScriptSection(def, arg, index), options));
        }
        else
        {
            insertPoint->second.insert(insertPoint->second.end(), options.begin(), options.end());
        }
    }

    std::map<ScriptSection, std::vector<std::wstring>> const& Script::GetSections() const
    {
        return sections;
    }

    void Script::Run( std::wostream& logOutput, IUserInterface *ui ) const
    {
        ui->LogMessage(L"Starting Execution");
        auto startTime = Instalog::GetLocalTime();
        WriteScriptHeader(logOutput, startTime);
        typedef std::pair<ScriptSection, std::vector<std::wstring> > contained;
        auto cmp = [](contained const& lhs, contained const& rhs) -> bool {
            auto const& lhsDef = lhs.first.GetDefinition();
            auto const& rhsDef = rhs.first.GetDefinition();
            if (lhsDef.GetPriority() != rhsDef.GetPriority())
            {
                return lhsDef.GetPriority() < rhsDef.GetPriority();
            }
            return lhs.first.GetParseIndex() < rhs.first.GetParseIndex();
        };
        std::vector<contained> sectionVec(sections.cbegin(), sections.cend());
        std::stable_sort(sectionVec.begin(), sectionVec.end(), cmp);
        for (auto begin = sectionVec.cbegin(), end = sectionVec.cend(); begin != end; ++begin)
        {
            auto header = begin->first.GetDefinition().GetName();
            std::wstring message(L"Executing " + header);
            ui->LogMessage(message);
            Instalog::Header(header);
            logOutput << L"\n" << header << L"\n\n";
            begin->first.GetDefinition().Execute(logOutput, begin->first, begin->second);
        }
        logOutput << L'\n';
        WriteScriptFooter(logOutput, startTime);
        ui->ReportFinished();
    }

}
