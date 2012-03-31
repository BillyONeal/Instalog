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
		std::wstring strings; // TODO
		std::wstring dataString;

		EventLogEntry(PEVENTLOGRECORD pRecord)
			: timeGenerated(pRecord->TimeGenerated)
			, timeWritten(pRecord->TimeWritten)
			, eventId(pRecord->EventID & 0x0000FFFF)
			, eventType(eventType)
			, eventCategory(eventCategory)
			, sourceName(reinterpret_cast<const wchar_t*>(reinterpret_cast<char*>(pRecord) + sizeof(*pRecord)))
			, computerName(reinterpret_cast<const wchar_t*>(reinterpret_cast<char*>(pRecord) + sizeof(*pRecord) + sourceName.size() * sizeof(wchar_t) + sizeof(wchar_t)))
			, strings(reinterpret_cast<const wchar_t*>(reinterpret_cast<char*>(pRecord) + pRecord->StringOffset))
			, dataString(reinterpret_cast<const wchar_t*>(reinterpret_cast<char*>(pRecord) + pRecord->DataOffset)) 
		{
		
		}
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

		std::vector<EventLogEntry> ReadEvents()
		{
			std::vector<EventLogEntry> eventLogEntries;

			DWORD lastError = ERROR_SUCCESS;
			std::vector<char> buffer;
			buffer.resize(1024);
			DWORD bytesRead;
			DWORD minNumberOfBytesNeeded;

			while (lastError == ERROR_SUCCESS)
			{
				BOOL status = ::ReadEventLogW(handle, 
					EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ, 
					0,
					buffer.data(),
					static_cast<DWORD>(buffer.size()),
					&bytesRead,
					&minNumberOfBytesNeeded);

				if (status == 0)
				{
					lastError = ::GetLastError();

					if (lastError == ERROR_INSUFFICIENT_BUFFER)
					{
						lastError = ERROR_SUCCESS;

						buffer.resize(minNumberOfBytesNeeded);
					}
					else if (lastError != ERROR_HANDLE_EOF)
					{
						Win32Exception::Throw(lastError);
					}
				}
				else
				{
					PEVENTLOGRECORD pRecord = reinterpret_cast<PEVENTLOGRECORD>(buffer.data());

					while (reinterpret_cast<char*>(pRecord) < buffer.data() + bytesRead)
					{
						eventLogEntries.push_back(EventLogEntry(pRecord));

						pRecord = reinterpret_cast<PEVENTLOGRECORD>(reinterpret_cast<char*>(pRecord) + pRecord->Length);
					}
				}
			}

			return eventLogEntries;
		}
	};

}}