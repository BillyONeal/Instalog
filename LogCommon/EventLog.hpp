// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <boost/noncopyable.hpp>
#include <string>
#include <memory>
#include <vector>
#include <windows.h>
#include "Win32Exception.hpp"
#include "Library.hpp"

namespace Instalog { namespace SystemFacades {
	
	struct EventLogEntry
	{
		FILETIME date;
		WORD level;
		DWORD eventId;

		EventLogEntry();
		EventLogEntry(EventLogEntry && e);
		virtual ~EventLogEntry();

		virtual std::wstring GetSource() = 0;
		virtual std::wstring GetDescription() = 0;
	};

	class OldEventLogEntry : public EventLogEntry
	{
		DWORD eventIdWithExtras;
		std::wstring source;
		std::vector<std::wstring> strings;
		std::wstring dataString; 

	public:
		OldEventLogEntry(PEVENTLOGRECORD pRecord);
		OldEventLogEntry(OldEventLogEntry && e);

		virtual std::wstring GetSource();
		virtual std::wstring GetDescription();
	};

	class XmlEventLogEntry : boost::noncopyable, public EventLogEntry
	{
		HANDLE eventHandle;
		std::wstring providerName;

		std::wstring FormatMessage(DWORD messageFlag);

	public:
		XmlEventLogEntry(HANDLE handle);
		XmlEventLogEntry(XmlEventLogEntry && x);
		XmlEventLogEntry& operator=(XmlEventLogEntry && x);
		~XmlEventLogEntry();

		virtual std::wstring GetSource();
		virtual std::wstring GetDescription();
	};

	struct EventLog : boost::noncopyable
	{
		virtual ~EventLog();

		virtual std::vector<std::unique_ptr<EventLogEntry>> ReadEvents() = 0;
	};

	/// @brief	Wrapper around the old Win32 event log
	class OldEventLog : public EventLog
	{
		HANDLE handle;
	public:
		/// @brief	Constructor.
		///
		/// @param	sourceName	(optional) name of the log source.
		OldEventLog(std::wstring sourceName = L"System");

		/// @brief	Destructor, frees the handle
		~OldEventLog();

		std::vector<std::unique_ptr<EventLogEntry>> ReadEvents();
	};
	
	/// @brief	Wrapper around the new (Vista and later) XML Win32 event log
	class XmlEventLog : public EventLog
	{
		HANDLE queryHandle;
	public:
		/// @brief	Constructor.
		///
		/// @param	logPath	(optional) full pathname of the log file.
		/// @param	query  	(optional) the query.
		/// 
		/// @throws FileNotFoundException on incompatible machines
		XmlEventLog(wchar_t* logPath = L"System", wchar_t* query = L"Event/System");

		/// @brief	Destructor, frees the handle
		~XmlEventLog();

		std::vector<std::unique_ptr<EventLogEntry>> ReadEvents();
	};

}}