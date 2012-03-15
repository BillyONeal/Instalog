#pragma once
#include <string>
#include <map>
#include <functional>
#include <vector>
#include <exception>
#include "StringUtilities.hpp"
#include "UserInterface.hpp"

namespace Instalog
{
	enum LogSectionPriorities
	{
		MEMORY,
		DISK_PERSISTENT,
		WMI,
		WHITELISTING,
		SCANNING
	};
	
	struct ISectionDefinition;

	struct ScriptSection
	{
		ISectionDefinition const* targetSection;
		std::wstring argument;
		bool operator<(const ScriptSection&) const;
	};

	struct ISectionDefinition
	{
		virtual ~ISectionDefinition() {}
		virtual std::wstring GetName() const = 0;
		virtual LogSectionPriorities GetPriority() const = 0;
		virtual void Execute(std::wostream& logOutput, IUserInterface *ui, ScriptSection const& sectionData, std::vector<std::wstring> const& options) const = 0;
	};

	class Script;

	class ScriptDispatcher
	{
		std::map<std::wstring, std::unique_ptr<ISectionDefinition>> sectionTypes;
	public:
		void AddSectionType(std::unique_ptr<ISectionDefinition> sectionTypeToAdd);
		Script Parse(std::wstring const& script) const;
	};

	class Script
	{
		ScriptDispatcher const* parent_;
		std::map<ScriptSection, std::vector<std::wstring>> sections;
	public:
		Script(ScriptDispatcher const* parent);
		std::map<ScriptSection, std::vector<std::wstring>> const& GetSections() const;
		void Add(ISectionDefinition const* def, std::wstring const& arg, std::vector<std::wstring> const& options);
		void Run(std::wostream& logOutput, IUserInterface *ui) const;
	};

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
