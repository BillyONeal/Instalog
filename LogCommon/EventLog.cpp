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
		, EvtCreateRenderContext(wevtapi.GetProcAddress<EvtCreateRenderContext_t>("EvtCreateRenderContext"))
		, EvtRender(wevtapi.GetProcAddress<EvtRender_t>("EvtRender"))
		, EvtOpenPublisherEnum(wevtapi.GetProcAddress<EvtOpenPublisherEnum_t>("EvtOpenPublisherEnum"))
		, EvtNextPublisherId(wevtapi.GetProcAddress<EvtNextPublisherId_t>("EvtNextPublisherId"))
		, handle(EvtQuery(NULL, logPath, query, 0x1 | 0x200 /*EvtQueryChannelPath | EvtQueryReverseDirection*/))
	{
		if (handle == NULL)
		{
			Win32Exception::ThrowFromLastError();
		}

		HANDLE hProviders = NULL;
		LPWSTR pwcsProviderName = NULL;
		LPWSTR pTemp = NULL;
		DWORD dwBufferSize = 0;
		DWORD dwBufferUsed = 0;
		DWORD status = ERROR_SUCCESS;

		// Get a handle to the list of providers.
		hProviders = EvtOpenPublisherEnum(NULL, 0);
		if (NULL == hProviders)
		{
			wprintf(L"EvtOpenPublisherEnum failed with %lu\n", GetLastError());
			goto cleanup;
		}

		wprintf(L"List of registered providers:\n\n");

		// Enumerate the providers in the list.
		int x = 1;
		while (x > 0)
		{
			// Get a provider from the list. If the buffer is not big enough
			// to contain the provider's name, reallocate the buffer to the required size.
			if  (!EvtNextPublisherId(hProviders, dwBufferSize, pwcsProviderName, &dwBufferUsed))
			{
				status = GetLastError();
				if (ERROR_NO_MORE_ITEMS == status)
				{
					break;
				}
				else if (ERROR_INSUFFICIENT_BUFFER == status)
				{
					dwBufferSize = dwBufferUsed;
					pTemp = (LPWSTR)realloc(pwcsProviderName, dwBufferSize * sizeof(WCHAR));
					if (pTemp)
					{
						pwcsProviderName = pTemp;
						pTemp = NULL;
						EvtNextPublisherId(hProviders, dwBufferSize, pwcsProviderName, &dwBufferUsed);
					}
					else
					{
						wprintf(L"realloc failed\n");
						goto cleanup;
					}
				}

				if (ERROR_SUCCESS != (status = GetLastError()))
				{
					wprintf(L"EvtNextPublisherId failed with %d\n", status);
					goto cleanup;
				}
			}

			wprintf(L"%s\n", pwcsProviderName);

			RtlZeroMemory(pwcsProviderName, dwBufferUsed * sizeof(WCHAR));
		}

cleanup:

		if (pwcsProviderName)
			free(pwcsProviderName);

		if (hProviders)
			EvtClose(hProviders);
	}

	XmlEventLog::~XmlEventLog()
	{
		EvtClose(handle);
	}

	typedef struct _EVT_VARIANT
	{
		union
		{
			BOOL        BooleanVal;
			INT8        SByteVal;
			INT16       Int16Val;
			INT32       Int32Val;
			INT64       Int64Val;
			UINT8       ByteVal;
			UINT16      UInt16Val;
			UINT32      UInt32Val;
			UINT64      UInt64Val;
			float       SingleVal;
			double      DoubleVal;
			ULONGLONG   FileTimeVal;
			SYSTEMTIME* SysTimeVal;
			GUID*       GuidVal;
			LPCWSTR     StringVal;
			LPCSTR      AnsiStringVal;
			PBYTE       BinaryVal;
			PSID        SidVal;
			size_t      SizeTVal;

			// array fields
			BOOL*       BooleanArr;
			INT8*       SByteArr;
			INT16*      Int16Arr;
			INT32*      Int32Arr;
			INT64*      Int64Arr;
			UINT8*      ByteArr;
			UINT16*     UInt16Arr;
			UINT32*     UInt32Arr;
			UINT64*     UInt64Arr;
			float*      SingleArr;
			double*     DoubleArr;
			FILETIME*   FileTimeArr;
			SYSTEMTIME* SysTimeArr;
			GUID*       GuidArr;
			LPWSTR*     StringArr;
			LPSTR*      AnsiStringArr;
			PSID*       SidArr;
			size_t*     SizeTArr;

			// internal fields
			HANDLE  EvtHandleVal;
			LPCWSTR     XmlVal;
			LPCWSTR*    XmlValArr;
		};

		DWORD Count;   // number of elements (not length) in bytes.
		DWORD Type;

	} EVT_VARIANT, *PEVT_VARIANT;

	typedef enum  {
		EvtSystemProviderName        = 0,
		EvtSystemProviderGuid,
		EvtSystemEventID,
		EvtSystemQualifiers,
		EvtSystemLevel,
		EvtSystemTask,
		EvtSystemOpcode,
		EvtSystemKeywords,
		EvtSystemTimeCreated,
		EvtSystemEventRecordId,
		EvtSystemActivityID,
		EvtSystemRelatedActivityID,
		EvtSystemProcessID,
		EvtSystemThreadID,
		EvtSystemChannel,
		EvtSystemComputer,
		EvtSystemUserID,
		EvtSystemVersion,
		EvtSystemPropertyIdEND 
	} EVT_SYSTEM_PROPERTY_ID;

	typedef enum _EVT_VARIANT_TYPE {
		EvtVarTypeNull         = 0,
		EvtVarTypeString       = 1,
		EvtVarTypeAnsiString   = 2,
		EvtVarTypeSByte        = 3,
		EvtVarTypeByte         = 4,
		EvtVarTypeInt16        = 5,
		EvtVarTypeUInt16       = 6,
		EvtVarTypeInt32        = 7,
		EvtVarTypeUInt32       = 8,
		EvtVarTypeInt64        = 9,
		EvtVarTypeUInt64       = 10,
		EvtVarTypeSingle       = 11,
		EvtVarTypeDouble       = 12,
		EvtVarTypeBoolean      = 13,
		EvtVarTypeBinary       = 14,
		EvtVarTypeGuid         = 15,
		EvtVarTypeSizeT        = 16,
		EvtVarTypeFileTime     = 17,
		EvtVarTypeSysTime      = 18,
		EvtVarTypeSid          = 19,
		EvtVarTypeHexInt32     = 20,
		EvtVarTypeHexInt64     = 21,
		EvtVarTypeEvtHandle    = 32,
		EvtVarTypeEvtXml       = 35 
	} EVT_VARIANT_TYPE;

#include <Sddl.h>
	DWORD XmlEventLog::PrintEvent(HANDLE hEvent)
	{
		DWORD status = ERROR_SUCCESS;
		HANDLE hContext = NULL;
		DWORD dwBufferSize = 0;
		DWORD dwBufferUsed = 0;
		DWORD dwPropertyCount = 0;
		PEVT_VARIANT pRenderedValues = NULL;
		WCHAR wsGuid[50];
		LPWSTR pwsSid = NULL;
		ULONGLONG ullTimeStamp = 0;
		ULONGLONG ullNanoseconds = 0;
		SYSTEMTIME st;
		FILETIME ft;

		// Identify the components of the event that you want to render. In this case,
		// render the system section of the event.
		hContext = EvtCreateRenderContext(0, NULL, 1 /*EvtRenderContextSystem*/);
		if (NULL == hContext)
		{
			wprintf(L"EvtCreateRenderContext failed with %lu\n", status = GetLastError());
			goto cleanup;
		}

		// When you render the user data or system section of the event, you must specify
		// the EvtRenderEventValues flag. The function returns an array of variant values 
		// for each element in the user data or system section of the event. For user data
		// or event data, the values are returned in the same order as the elements are 
		// defined in the event. For system data, the values are returned in the order defined
		// in the EVT_SYSTEM_PROPERTY_ID enumeration.
		if (!EvtRender(hContext, hEvent, 0 /*EvtRenderEventValues*/, dwBufferSize, pRenderedValues, &dwBufferUsed, &dwPropertyCount))
		{
			if (ERROR_INSUFFICIENT_BUFFER == (status = GetLastError()))
			{
				dwBufferSize = dwBufferUsed;
				pRenderedValues = (PEVT_VARIANT)malloc(dwBufferSize);
				if (pRenderedValues)
				{
					EvtRender(hContext, hEvent, 0 /*EvtRenderEventValues*/, dwBufferSize, pRenderedValues, &dwBufferUsed, &dwPropertyCount);
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

		// Print the values from the System section of the element.
		wprintf(L"Provider Name: %s\n", pRenderedValues[EvtSystemProviderName].StringVal);
		if (NULL != pRenderedValues[EvtSystemProviderGuid].GuidVal)
		{
			StringFromGUID2(*(pRenderedValues[EvtSystemProviderGuid].GuidVal), wsGuid, sizeof(wsGuid)/sizeof(WCHAR));
			wprintf(L"Provider Guid: %s\n", wsGuid);
		}
		else 
		{
			wprintf(L"Provider Guid: NULL");
		}


		DWORD EventID = pRenderedValues[EvtSystemEventID].UInt16Val;
		if (EvtVarTypeNull != pRenderedValues[EvtSystemQualifiers].Type)
		{
			EventID = MAKELONG(pRenderedValues[EvtSystemEventID].UInt16Val, pRenderedValues[EvtSystemQualifiers].UInt16Val);
		}
		wprintf(L"EventID: %lu\n", EventID);

		wprintf(L"Version: %u\n", (EvtVarTypeNull == pRenderedValues[EvtSystemVersion].Type) ? 0 : pRenderedValues[EvtSystemVersion].ByteVal);
		wprintf(L"Level: %u\n", (EvtVarTypeNull == pRenderedValues[EvtSystemLevel].Type) ? 0 : pRenderedValues[EvtSystemLevel].ByteVal);
		wprintf(L"Task: %hu\n", (EvtVarTypeNull == pRenderedValues[EvtSystemTask].Type) ? 0 : pRenderedValues[EvtSystemTask].UInt16Val);
		wprintf(L"Opcode: %u\n", (EvtVarTypeNull == pRenderedValues[EvtSystemOpcode].Type) ? 0 : pRenderedValues[EvtSystemOpcode].ByteVal);
		wprintf(L"Keywords: 0x%I64x\n", pRenderedValues[EvtSystemKeywords].UInt64Val);

		ullTimeStamp = pRenderedValues[EvtSystemTimeCreated].FileTimeVal;
		ft.dwHighDateTime = (DWORD)((ullTimeStamp >> 32) & 0xFFFFFFFF);
		ft.dwLowDateTime = (DWORD)(ullTimeStamp & 0xFFFFFFFF);

		FileTimeToSystemTime(&ft, &st);
		ullNanoseconds = (ullTimeStamp % 10000000) * 100; // Display nanoseconds instead of milliseconds for higher resolution
		wprintf(L"TimeCreated SystemTime: %02d/%02d/%02d %02d:%02d:%02d.%I64u)\n", 
			st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond, ullNanoseconds);

		wprintf(L"EventRecordID: %I64u\n", pRenderedValues[EvtSystemEventRecordId].UInt64Val);

		if (EvtVarTypeNull != pRenderedValues[EvtSystemActivityID].Type)
		{
			StringFromGUID2(*(pRenderedValues[EvtSystemActivityID].GuidVal), wsGuid, sizeof(wsGuid)/sizeof(WCHAR));
			wprintf(L"Correlation ActivityID: %s\n", wsGuid);
		}

		if (EvtVarTypeNull != pRenderedValues[EvtSystemRelatedActivityID].Type)
		{
			StringFromGUID2(*(pRenderedValues[EvtSystemRelatedActivityID].GuidVal), wsGuid, sizeof(wsGuid)/sizeof(WCHAR));
			wprintf(L"Correlation RelatedActivityID: %s\n", wsGuid);
		}

		wprintf(L"Execution ProcessID: %lu\n", pRenderedValues[EvtSystemProcessID].UInt32Val);
		wprintf(L"Execution ThreadID: %lu\n", pRenderedValues[EvtSystemThreadID].UInt32Val);
		wprintf(L"Channel: %s\n", (EvtVarTypeNull == pRenderedValues[EvtSystemChannel].Type) ? L"" : pRenderedValues[EvtSystemChannel].StringVal);
		wprintf(L"Computer: %s\n", pRenderedValues[EvtSystemComputer].StringVal);

		if (EvtVarTypeNull != pRenderedValues[EvtSystemUserID].Type)
		{
			if (ConvertSidToStringSid(pRenderedValues[EvtSystemUserID].SidVal, &pwsSid))
			{
				wprintf(L"Security UserID: %s\n", pwsSid);
				LocalFree(pwsSid);
			}
		}

cleanup:

		if (hContext)
			EvtClose(hContext);

		if (pRenderedValues)
			free(pRenderedValues);

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