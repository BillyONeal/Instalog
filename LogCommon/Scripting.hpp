#pragma once
#include <string>
#include <vector>
#include <functional>
#include "UserInterface.hpp"

namespace Instalog
{
	enum CheckState
	{
		UNCHANGED = 0,
		CHECKED = 1,
		UNCHECKED = -1
	};

	struct ICheckboxDefinition
	{
		virtual ~ICheckboxDefinition() {}
		virtual std::wstring GetName() const = 0;
		virtual std::wstring GetDescription() const = 0;
		virtual CheckState* Checked() const = 0;
		virtual CheckState* UnChecked() const = 0;
	};

	enum LogSectionPriorities
	{
		MEMORY,
		DISK_PERSISTENT,
		WMI,
		WHITELISTING,
		SCANNING
	};

	struct ISectionDefinition
	{
		virtual ~ISectionDefinition() {}
		virtual std::wstring GetName() const = 0;
		virtual LogSectionPriorities GetPriority() const = 0;
		virtual void Execute(std::ostream& logOutput, IUserInterface *ui, std::wstring const& argument, std::vector<std::wstring> const& options) = 0;
		virtual void Build(std::ostream& scriptOutput, std::wstring const& line, std::vector<bool> checkboxStates) = 0;
		virtual std::vector<std::unique_ptr<ICheckboxDefinition>> GetCheckboxes() const = 0;
	};

	struct Line
	{
		std::wstring content;
		std::vector<bool> checkStates;
		void Write(std::wostream&) const;
	};

	struct LogSection
	{
		ISectionDefinition *parent;
		std::wstring title;
		std::vector<Line> lines;
		void Write(std::wostream& target) const;
	};

	class Log
	{
		std::vector<LogSection> sections;
		std::vector<std::function<void(Log const*)>> listeners;
	public:
		void Notify() const;
		void AddSection(LogSection const& section);
		void AddListener(std::function<void(Log const*)> listener);
		void Check(std::size_t sectionIndex, std::size_t lineIndex);
		void Write(std::wostream& target) const;
	};

	class ScriptDispatcher
	{
		
	};
}