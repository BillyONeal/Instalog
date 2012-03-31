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
		DWORD timeWritten;
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