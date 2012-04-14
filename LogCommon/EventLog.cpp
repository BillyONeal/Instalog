// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "EventLog.hpp"
#include "Win32Exception.hpp"
#include "Win32Glue.hpp"
#include "StockOutputFormats.hpp"
#include "StringUtilities.hpp"
#include "Registry.hpp"
#include "Path.hpp"
#include "Library.hpp"

namespace Instalog { namespace SystemFacades {

	OldEventLogEntry::OldEventLogEntry( PEVENTLOGRECORD pRecord ) 
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

	OldEventLogEntry::OldEventLogEntry( OldEventLogEntry && e ) 
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

	DWORD OldEventLogEntry::GetEventIdCode()
	{
		return eventId & 0x0000FFFF;
	}

	std::wstring OldEventLogEntry::GetDescription() 
	{
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

	void OldEventLogEntry::OutputToLog( std::wostream& logOutput )
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

		std::wstring description = GetDescription();
		GeneralEscape(description);
		if (boost::algorithm::ends_with(description, "#r#n"))
		{
			logOutput << description.substr(0, description.size() - 4) << std::endl;
		}
		else
		{
			logOutput << description << std::endl;		
		}
	}

	OldEventLog::OldEventLog( std::wstring sourceName /*= L"System"*/ )
		: handle(::OpenEventLogW(NULL, sourceName.c_str()))
	{
		if (handle == NULL)
		{
			Win32Exception::ThrowFromLastError();
		}
	}

	OldEventLog::~OldEventLog()
	{
		::CloseEventLog(handle);
	}

	std::vector<OldEventLogEntry> OldEventLog::ReadEvents()
	{
		std::vector<OldEventLogEntry> eventLogEntries;
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
					eventLogEntries.emplace_back(OldEventLogEntry(pRecord));
				}
			}
		}

		return eventLogEntries;
	}	

	XmlEventLog::XmlEventLog( wchar_t* logPath /*= L"System"*/, wchar_t* query /*= L"Event/System"*/ )
		: wevtapi(L"wevtapi.dll")
		, EvtQuery(wevtapi.GetProcAddress<EvtQuery_t>("EvtQuery"))
		, EvtClose(wevtapi.GetProcAddress<EvtClose_t>("EvtClose"))
		, EvtNext(wevtapi.GetProcAddress<EvtNext_t>("EvtNext"))
		, EvtRender(wevtapi.GetProcAddress<EvtRender_t>("EvtRender"))
		, handle(EvtQuery(NULL, logPath, query, 0x1 | 0x200 /*EvtQueryChannelPath | EvtQueryReverseDirection*/))
	{
		if (handle == NULL)
		{
			Win32Exception::ThrowFromLastError();
		}
	}

	XmlEventLog::~XmlEventLog()
	{
		EvtClose(handle);
	}

	DWORD XmlEventLog::PrintEvent(HANDLE hEvent)
	{
		DWORD status = ERROR_SUCCESS;
		DWORD dwBufferSize = 0;
		DWORD dwBufferUsed = 0;
		DWORD dwPropertyCount = 0;
		LPWSTR pRenderedContent = NULL;

		// The EvtRenderEventXml flag tells EvtRender to render the event as an XML string.
		if (!EvtRender(NULL, hEvent, 1 /*EvtRenderEventXml*/, dwBufferSize, pRenderedContent, &dwBufferUsed, &dwPropertyCount))
		{
			if (ERROR_INSUFFICIENT_BUFFER == (status = GetLastError()))
			{
				dwBufferSize = dwBufferUsed;
				pRenderedContent = (LPWSTR)malloc(dwBufferSize);
				if (pRenderedContent)
				{
					EvtRender(NULL, hEvent, 1 /*EvtRenderEventXml*/, dwBufferSize, pRenderedContent, &dwBufferUsed, &dwPropertyCount);
				}
				else
				{
					wprintf(L"malloc failed\n");
					status = ERROR_OUTOFMEMORY;
					goto cleanup;
				}
			}

			if (ERROR_SUCCESS != (status = GetLastError()))
			{
				wprintf(L"EvtRender failed with %d\n", GetLastError());
				goto cleanup;
			}
		}

		wprintf(L"\n\n%s", pRenderedContent);

cleanup:

		if (pRenderedContent)
			free(pRenderedContent);

		return status;
	}

	std::vector<XmlEventLogEntry> XmlEventLog::ReadEvents()
	{
		DWORD status = ERROR_SUCCESS;
		HANDLE hEvents[10];
		DWORD dwReturned = 0;

		int x = 1;

		while (x > 0)
		{
			// Get a block of events from the result set.
			if (!EvtNext(handle, 10, hEvents, INFINITE, 0, &dwReturned))
			{
				if (ERROR_NO_MORE_ITEMS != (status = GetLastError()))
				{
					wprintf(L"EvtNext failed with %lu\n", status);
				}

				goto cleanup;
			}

			// For each event, call the PrintEvent function which renders the
			// event for display. PrintEvent is shown in RenderingEvents.
			for (DWORD i = 0; i < dwReturned; i++)
			{
				if (ERROR_SUCCESS == (status = PrintEvent(hEvents[i])))
				{
					EvtClose(hEvents[i]);
					hEvents[i] = NULL;
				}
				else
				{
					goto cleanup;
				}
			}
		}

cleanup:

		for (DWORD i = 0; i < dwReturned; i++)
		{
			if (NULL != hEvents[i])
				EvtClose(hEvents[i]);
		}

		return std::vector<XmlEventLogEntry>();
	}

}}