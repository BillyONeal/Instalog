// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "EventLog.hpp"
#include "Win32Exception.hpp"
#include "Win32Glue.hpp"
#include "StockOutputFormats.hpp"
#include "Registry.hpp"
#include "Path.hpp"
#include "Library.hpp"

namespace Instalog { namespace SystemFacades {

	EventLogEntry::EventLogEntry( PEVENTLOGRECORD pRecord ) 
		: timeGenerated(pRecord->TimeGenerated)
		, eventId(pRecord->EventID)
		, eventType(pRecord->EventType)
		, eventCategory(eventCategory)
		, sourceName(reinterpret_cast<const wchar_t*>(reinterpret_cast<char*>(pRecord) + sizeof(*pRecord)))
		, computerName(reinterpret_cast<const wchar_t*>(reinterpret_cast<char*>(pRecord) + sizeof(*pRecord) + sourceName.size() * sizeof(wchar_t) + sizeof(wchar_t)))
		, dataString(reinterpret_cast<const wchar_t*>(reinterpret_cast<char*>(pRecord) + pRecord->DataOffset))
	{
		strings.reserve(pRecord->NumStrings);

		for (wchar_t* stringPtr = reinterpret_cast<wchar_t*>(reinterpret_cast<char*>(pRecord) + pRecord->StringOffset); strings.size() < pRecord->NumStrings; stringPtr += strings.back().length() + 1)
		{
			strings.push_back(std::wstring(stringPtr));
		}
	}

	EventLogEntry::EventLogEntry( EventLogEntry && e ) 
		: timeGenerated(e.timeGenerated)
		, eventId(e.eventId)
		, eventType(e.eventType)
		, eventCategory(eventCategory)
		, sourceName(std::move(e.sourceName))
		, computerName(std::move(e.computerName))
		, strings(std::move(e.strings))
		, dataString(std::move(e.dataString))
	{

	}

	DWORD EventLogEntry::GetEventIdCode()
	{
		return eventId & 0x0000FFFF;
	}

	std::wstring EventLogEntry::GetDescription() 
	{
		if (sourceName == L"EventLog")
		{
			return L"SKIPPED\r\n"; // TODO, for some reason the dates in eventlog break everything
		}

		RegistryKey eventKey = RegistryKey::Open(std::wstring(L"\\Registry\\Machine\\System\\CurrentControlSet\\services\\eventlog\\System\\" + sourceName), KEY_QUERY_VALUE);
		if (eventKey.Invalid())
		{
			Win32Exception::ThrowFromNtError(::GetLastError());
		}

		try
		{
			RegistryValue eventMessageFileValue = eventKey.GetValue(L"EventMessageFile");
			std::wstring eventMessageFilePath = eventMessageFileValue.GetStringStrict();
			Path::ResolveFromCommandLine(eventMessageFilePath);

			FormattedMessageLoader eventMessageFile(eventMessageFilePath);
			return eventMessageFile.GetFormattedMessage(eventId, strings);
		}
		catch (ErrorFileNotFoundException const&)
		{
			// We don't know what library to use so just return the short data string
			return dataString;
		}
	}

	void EventLogEntry::OutputToLog( std::wostream& logOutput )
	{
		// Print the Date
		FILETIME filetime = FiletimeFromSecondsSince1970(timeGenerated);
		WriteDefaultDateFormat(logOutput, FiletimeToInteger(filetime));

		// Print the Type
		switch (eventType)
		{
		case EVENTLOG_ERROR_TYPE: logOutput << L", Error: "; break;
		case EVENTLOG_WARNING_TYPE: logOutput << L", Warning: "; break;
		case EVENTLOG_INFORMATION_TYPE: logOutput << L", Information: "; break;
		default: logOutput << L", Unknown: "; break;
		}

		// Print the Source
		logOutput << sourceName << L" [";

		// Print the EventID
		logOutput << GetEventIdCode() << L"] ";

		logOutput << GetDescription();
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
				for (PEVENTLOGRECORD pRecord = reinterpret_cast<PEVENTLOGRECORD>(buffer.data()); reinterpret_cast<char*>(pRecord) < buffer.data() + bytesRead; pRecord = reinterpret_cast<PEVENTLOGRECORD>(reinterpret_cast<char*>(pRecord) + pRecord->Length))
				{
					eventLogEntries.emplace_back(EventLogEntry(pRecord));
				}
			}
		}

		return eventLogEntries;
	}


}}