#include "pch.hpp"
#include "EventLog.hpp"
#include "Win32Exception.hpp"

namespace Instalog { namespace SystemFacades {

	EventLogEntry::EventLogEntry( PEVENTLOGRECORD pRecord ) : timeGenerated(pRecord->TimeGenerated)
		, timeWritten(pRecord->TimeWritten)
		, eventId(pRecord->EventID & 0x0000FFFF)
		, eventType(pRecord->EventType)
		, eventCategory(eventCategory)
		, sourceName(reinterpret_cast<const wchar_t*>(reinterpret_cast<char*>(pRecord) + sizeof(*pRecord)))
		, computerName(reinterpret_cast<const wchar_t*>(reinterpret_cast<char*>(pRecord) + sizeof(*pRecord) + sourceName.size() * sizeof(wchar_t) + sizeof(wchar_t)))
		, strings(reinterpret_cast<const wchar_t*>(reinterpret_cast<char*>(pRecord) + pRecord->StringOffset))
		, dataString(reinterpret_cast<const wchar_t*>(reinterpret_cast<char*>(pRecord) + pRecord->DataOffset))
	{

	}

	EventLogEntry::EventLogEntry( EventLogEntry && e ) : timeGenerated(e.timeGenerated)
		, timeWritten(e.timeWritten)
		, eventId(e.eventId)
		, eventType(e.eventType)
		, eventCategory(eventCategory)
		, sourceName(std::move(e.sourceName))
		, computerName(std::move(e.computerName))
		, dataString(std::move(e.dataString))
	{

	}

	EventLog::EventLog( std::wstring sourceName /*= L"System"*/ )
		: handle(::OpenEventLogW(NULL, sourceName.c_str()))
	{
		if (handle == NULL)
		{
			Win32Exception::ThrowFromLastError();
		}
	}

	EventLog::~EventLog()
	{
		::CloseEventLog(handle);
	}

	std::vector<EventLogEntry> EventLog::ReadEvents()
	{
		std::vector<EventLogEntry> eventLogEntries;
		eventLogEntries.reserve(16 * 1024 /* approximate based on dev machine */);

		DWORD lastError = ERROR_SUCCESS;
		std::vector<char> buffer(0x7ffff /* (max size of buffer) */); // http://msdn.microsoft.com/en-us/library/aa363674.aspx
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
					eventLogEntries.emplace_back(EventLogEntry(pRecord));

					pRecord = reinterpret_cast<PEVENTLOGRECORD>(reinterpret_cast<char*>(pRecord) + pRecord->Length);
				}
			}
		}

		return eventLogEntries;
	}


}}