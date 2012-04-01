// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <boost/noncopyable.hpp>
#include <string>
#include <vector>
#include <windows.h>
#include "Win32Exception.hpp"

namespace Instalog { namespace SystemFacades {

	/// @brief	Event log entry.
	struct EventLogEntry
	{
		DWORD timeGenerated;
		DWORD eventId;
		WORD eventType;
		WORD eventCategory;
		std::wstring sourceName;
		std::wstring computerName;
		std::vector<std::wstring> strings;
		std::wstring dataString; 

		EventLogEntry(PEVENTLOGRECORD pRecord);

		/// @brief	Move constructor.
		///
		/// @param [in,out]	e	The EventLogEntry &amp;&amp; to process.
		EventLogEntry(EventLogEntry && e);

		/// @brief	Gets the event identifier code (strips out severity and other details)
		///
		/// @return	The event identifier code.
		DWORD GetEventIdCode();

		/// @brief	Output to log stream
		///
		/// @param [out]	logOutput	The log stream to output to.
		void OutputToLog(std::wostream& logOutput);
	private:
		/// @brief	Gets the human-readable event description.
		///
		/// @return	The description.
		std::wstring EventLogEntry::GetDescription();
	};

	/// @brief	Wrapper around Win32 event log
	class EventLog : boost::noncopyable
	{
		HANDLE handle;
	public:
		/// @brief	Constructor.
		///
		/// @param	sourceName	(optional) name of the log source.
		EventLog(std::wstring sourceName = L"System");

		/// @brief	Destructor, frees the handle
		~EventLog();

		std::vector<EventLogEntry> ReadEvents();
	};

}}