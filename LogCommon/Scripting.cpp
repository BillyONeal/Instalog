#include "pch.hpp"
#include <clocale>
#include <functional>
#include <algorithm>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "Scripting.hpp"

using namespace boost::algorithm;

namespace Instalog
{

	void ScriptDispatcher::AddSectionType( std::unique_ptr<ISectionDefinition> sectionTypeToAdd )
	{
		std::wstring title(sectionTypeToAdd->GetName());
		title.erase(std::remove(title.begin(), title.end(), L' '), title.end());
		to_lower(title);
		sectionTypes.emplace(std::make_pair(std::move(title), std::move(sectionTypeToAdd)));
	}

	Script ScriptDispatcher::Parse( std::wstring const& script ) const
	{
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
			result.Add(def, argument, std::vector<std::wstring>(begin, endOfOptions));
		}
		return result;
	}


	bool ScriptSection::operator<( const ScriptSection& rhs ) const
	{
		if (targetSection->GetPriority() != rhs.targetSection->GetPriority())
		{
			return targetSection->GetPriority() < rhs.targetSection->GetPriority();
		}
		if (targetSection->GetName() != rhs.targetSection->GetName())
		{
			return targetSection->GetName() < rhs.targetSection->GetName();
		}
		return argument < rhs.argument;
	}


	Script::Script( ScriptDispatcher const* parent )
		: parent_(parent)
	{ }

	void Script::Add( ISectionDefinition const* def, std::wstring const& arg, std::vector<std::wstring> const& options )
	{
		ScriptSection ss;
		ss.targetSection = def;
		ss.argument = arg;
		auto insertPoint = sections.find(ss);
		if (insertPoint == sections.end())
		{
			sections.insert(std::make_pair(ss, options));
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
		for (auto begin = sections.cbegin(), end = sections.cend(); begin != end; ++begin)
		{
			begin->first.targetSection->Execute(logOutput, ui, begin->first, begin->second);
		}
	}

}
