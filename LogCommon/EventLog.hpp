// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <boost/noncopyable.hpp>
#include <string>
#include <vector>
#include <windows.h>
#include "Win32Exception.hpp"
#include "Library.hpp"

namespace Instalog { namespace SystemFacades {

	/// @brief	Event log entry.
	struct OldEventLogEntry
	{
		DWORD timeGenerated;
		DWORD eventId;
		WORD eventType;
		WORD eventCategory;
		std::wstring sourceName;
		std::wstring computerName;
		std::vector<std::wstring> strings;
		std::wstring dataString; 

		OldEventLogEntry(PEVENTLOGRECORD pRecord);

		/// @brief	Move constructor.
		///
		/// @param [in,out]	e	The EventLogEntry &amp;&amp; to process.
		OldEventLogEntry(OldEventLogEntry && e);

		/// @brief	Gets the event identifier code (strips out severity and other details)
		///
		/// @return	The event identifier code.
		DWORD GetEventIdCode();

		/// @brief	Gets the human-readable event description.
		///
		/// @return	The description.
		std::wstring GetDescription();

		/// @brief	Output to log stream
		///
		/// @param [out]	logOutput	The log stream to output to.
		void OutputToLog(std::wostream& logOutput);
	};

	/// @brief	Wrapper around the old Win32 event log
	class OldEventLog : boost::noncopyable
	{
		HANDLE handle;
	public:
		/// @brief	Constructor.
		///
		/// @param	sourceName	(optional) name of the log source.
		OldEventLog(std::wstring sourceName = L"System");

		/// @brief	Destructor, frees the handle
		~OldEventLog();

		std::vector<OldEventLogEntry> ReadEvents();
	};

	class EventLogEntry
	{
	public:
		FILETIME date;
		WORD type;
		std::wstring source;
		DWORD eventId;

		virtual std::wstring GetDescription() = 0;
	};

	class XmlEventLogEntry : boost::noncopyable, EventLogEntry
	{
		HANDLE eventHandle;

	public:
		XmlEventLogEntry(HANDLE handle);
		XmlEventLogEntry(XmlEventLogEntry && x);
		XmlEventLogEntry& operator=(XmlEventLogEntry && x);
		~XmlEventLogEntry();

		std::wstring GetDescription();
	};
	
	/// @brief	Wrapper around the new (Vista and later) XML Win32 event log
	class XmlEventLog : boost::noncopyable
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

		std::vector<XmlEventLogEntry> ReadEvents();
	};

}}