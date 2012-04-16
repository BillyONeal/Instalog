// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <vector>
#include <sstream>
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
		: eventIdWithExtras(pRecord->EventID)
		, dataString(reinterpret_cast<const wchar_t*>(reinterpret_cast<char*>(pRecord) + pRecord->DataOffset))
	{
		date = FiletimeFromSecondsSince1970(pRecord->TimeGenerated);
		level = pRecord->EventType;
		source = reinterpret_cast<const wchar_t*>(reinterpret_cast<char*>(pRecord) + sizeof(*pRecord));
		eventId = eventIdWithExtras & 0x0000FFFF;

		strings.reserve(pRecord->NumStrings);
		for (wchar_t* stringPtr = reinterpret_cast<wchar_t*>(reinterpret_cast<char*>(pRecord) + pRecord->StringOffset); strings.size() < pRecord->NumStrings; stringPtr += strings.back().length() + 1)
		{
			strings.push_back(std::wstring(stringPtr));
		}
	}

	OldEventLogEntry::OldEventLogEntry( OldEventLogEntry && e )
		: eventIdWithExtras(e.eventIdWithExtras)
		, source(e.source)
		, strings(std::move(e.strings))
		, dataString(std::move(e.dataString))
	{
		e.eventIdWithExtras = 0;
		e.source = L"";
	}

	std::wstring OldEventLogEntry::GetDescription() 
	{
		RegistryKey eventKey = RegistryKey::Open(std::wstring(L"\\Registry\\Machine\\System\\CurrentControlSet\\services\\eventlog\\System\\" + source), KEY_QUERY_VALUE);
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
			return eventMessageFile.GetFormattedMessage(eventIdWithExtras, strings);
		}
		catch (ErrorFileNotFoundException const&)
		{
			// We don't know what library to use so just return the short data string
			return dataString;
		}
	}

	std::wstring OldEventLogEntry::GetSource()
	{
		return source;
	}

// 	void OldEventLogEntry::OutputToLog( std::wostream& logOutput )
// 	{
// 		// Print the Date
// 		FILETIME filetime = FiletimeFromSecondsSince1970(timeGenerated);
// 		WriteDefaultDateFormat(logOutput, FiletimeToInteger(filetime));
// 
// 		// Print the Type
// 		switch (eventType)
// 		{
// 		case EVENTLOG_ERROR_TYPE: logOutput << L", Error: "; break;
// 		case EVENTLOG_WARNING_TYPE: logOutput << L", Warning: "; break;
// 		case EVENTLOG_INFORMATION_TYPE: logOutput << L", Information: "; break;
// 		default: logOutput << L", Unknown: "; break;
// 		}
// 
// 		// Print the Source
// 		logOutput << sourceName << L" [";
// 
// 		// Print the EventID
// 		logOutput << GetEventIdCode() << L"] ";
// 
// 		std::wstring description = GetDescription();
// 		GeneralEscape(description);
// 		if (boost::algorithm::ends_with(description, "#r#n"))
// 		{
// 			logOutput << description.substr(0, description.size() - 4) << std::endl;
// 		}
// 		else
// 		{
// 			logOutput << description << std::endl;		
// 		}
// 	}

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

	std::vector<std::unique_ptr<EventLogEntry>> OldEventLog::ReadEvents()
	{
		std::vector<std::unique_ptr<EventLogEntry>> eventLogEntries;
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
					eventLogEntries.emplace_back(std::unique_ptr<EventLogEntry>(new OldEventLogEntry(pRecord)));
				}
			}
		}

		return eventLogEntries;

	}	

	typedef enum _EVT_QUERY_FLAGS {
		EvtQueryChannelPath           = 0x1,
		EvtQueryFilePath              = 0x2,
		EvtQueryForwardDirection      = 0x100,
		EvtQueryReverseDirection      = 0x200,
		EvtQueryTolerateQueryErrors   = 0x1000 
	} EVT_QUERY_FLAGS;

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

	typedef enum _EVT_RENDER_CONTEXT_FLAGS {
		EvtRenderContextValues   = 0,
		EvtRenderContextSystem   = 1,
		EvtRenderContextUser     = 2  
	} EVT_RENDER_CONTEXT_FLAGS;

	typedef enum _EVT_RENDER_FLAGS {
		EvtRenderEventValues   = 0,
		EvtRenderEventXml      = 1,
		EvtRenderBookmark      = 2 
	} EVT_RENDER_FLAGS;

	typedef enum _EVT_FORMAT_MESSAGE_FLAGS {
		EvtFormatMessageEvent      = 1,
		EvtFormatMessageLevel      = 2,
		EvtFormatMessageTask       = 3,
		EvtFormatMessageOpcode     = 4,
		EvtFormatMessageKeyword    = 5,
		EvtFormatMessageChannel    = 6,
		EvtFormatMessageProvider   = 7,
		EvtFormatMessageId         = 8,
		EvtFormatMessageXml        = 9 
	} EVT_FORMAT_MESSAGE_FLAGS;

	/// @brief	Function handles for the new XML API
	class EvtFunctionHandles : boost::noncopyable
	{
		RuntimeDynamicLinker wevtapi;

	public:
		typedef HANDLE (WINAPI *EvtQuery_t)(HANDLE, LPCWSTR, LPCWSTR, DWORD);
		EvtQuery_t EvtQuery;

		typedef BOOL (WINAPI *EvtClose_t)(HANDLE);
		EvtClose_t EvtClose;

		typedef BOOL (WINAPI *EvtNext_t)(HANDLE, DWORD, HANDLE*, DWORD, DWORD, PDWORD);
		EvtNext_t EvtNext;

		typedef HANDLE (WINAPI *EvtCreateRenderContext_t)(DWORD, LPCWSTR*, DWORD);
		EvtCreateRenderContext_t EvtCreateRenderContext;

		typedef BOOL (WINAPI *EvtRender_t)(HANDLE, HANDLE, DWORD, DWORD, PVOID, PDWORD, PDWORD);
		EvtRender_t EvtRender;

		typedef HANDLE (WINAPI *EvtOpenPublisherMetadata_t)(HANDLE, LPCWSTR, LPCWSTR, LCID, DWORD);
		EvtOpenPublisherMetadata_t EvtOpenPublisherMetadata;

		typedef HANDLE (WINAPI *EvtFormatMessage_t)(HANDLE, HANDLE, DWORD, DWORD, PEVT_VARIANT, DWORD, DWORD, LPWSTR, PDWORD);
		EvtFormatMessage_t EvtFormatMessage;

		typedef HANDLE (WINAPI *EvtOpenPublisherEnum_t)(HANDLE, DWORD); // TODO: Don't think I need this
		EvtOpenPublisherEnum_t EvtOpenPublisherEnum;

		typedef BOOL (WINAPI *EvtNextPublisherId_t)(HANDLE, DWORD, LPWSTR, PDWORD); // TODO: Don't think I need this
		EvtNextPublisherId_t EvtNextPublisherId;

		EvtFunctionHandles()
			: wevtapi(L"wevtapi.dll")
			, EvtQuery(wevtapi.GetProcAddress<EvtQuery_t>("EvtQuery"))
			, EvtClose(wevtapi.GetProcAddress<EvtClose_t>("EvtClose"))
			, EvtNext(wevtapi.GetProcAddress<EvtNext_t>("EvtNext"))
			, EvtCreateRenderContext(wevtapi.GetProcAddress<EvtCreateRenderContext_t>("EvtCreateRenderContext"))
			, EvtRender(wevtapi.GetProcAddress<EvtRender_t>("EvtRender"))
			, EvtOpenPublisherMetadata(wevtapi.GetProcAddress<EvtOpenPublisherMetadata_t>("EvtOpenPublisherMetadata"))
			, EvtFormatMessage(wevtapi.GetProcAddress<EvtFormatMessage_t>("EvtFormatMessage"))
			, EvtOpenPublisherEnum(wevtapi.GetProcAddress<EvtOpenPublisherEnum_t>("EvtOpenPublisherEnum"))
			, EvtNextPublisherId(wevtapi.GetProcAddress<EvtNextPublisherId_t>("EvtNextPublisherId"))
		{
		}
	};

	/// @brief	Returns a static copy of the event handles
	///
	/// @return	Event API handles
	/// 
	/// @throws Exception if loading any of the handles failed
	static EvtFunctionHandles const& EvtFunctions()
	{
		static EvtFunctionHandles evtFunctionHandles;

		return evtFunctionHandles;
	}

	std::wstring XmlEventLogEntry::FormatMessage( DWORD messageFlag )
	{
		HANDLE publisherHandle = EvtFunctions().EvtOpenPublisherMetadata(NULL, providerName.c_str(), NULL, 0, 0);
		if (publisherHandle == NULL)
		{
			Win32Exception::ThrowFromLastError();
		}

		std::vector<wchar_t> buffer;
		DWORD bufferUsed = 0;
		while (EvtFunctions().EvtFormatMessage(publisherHandle, eventHandle, 0, 0, NULL, messageFlag, static_cast<DWORD>(buffer.capacity()), static_cast<LPWSTR>(buffer.data()), &bufferUsed) == false)
		{
			DWORD status = GetLastError();
			switch(status)
			{
			case ERROR_INSUFFICIENT_BUFFER: 
				buffer.reserve(bufferUsed);
				break;
			case ERROR_EVT_MESSAGE_NOT_FOUND:
			case ERROR_EVT_MESSAGE_ID_NOT_FOUND:
				return L"";
				break;
			default:
				Win32Exception::Throw(status);
			}			
		}

		EvtFunctions().EvtClose(publisherHandle);

		return buffer.data();
	}

	XmlEventLogEntry::XmlEventLogEntry( HANDLE handle )
		: eventHandle(handle)
	{
		HANDLE contextHandle = EvtFunctions().EvtCreateRenderContext(0, NULL, EvtRenderContextSystem);
		if (contextHandle == NULL)
		{
			Win32Exception::ThrowFromLastError();
		}

		std::vector<char> buffer;
		DWORD bufferUsed = 0;
		DWORD propertyCount = 0;
		while (EvtFunctions().EvtRender(contextHandle, eventHandle, EvtRenderEventValues, static_cast<DWORD>(buffer.capacity()), buffer.data(), &bufferUsed, &propertyCount) == false)
		{
			DWORD status = GetLastError();
			if (status == ERROR_INSUFFICIENT_BUFFER)
			{
				buffer.reserve(bufferUsed);
			}
			else
			{
				Win32Exception::Throw(status);
			}
		}		
		PEVT_VARIANT renderedValues = reinterpret_cast<PEVT_VARIANT>(buffer.data());

		providerName = std::wstring(renderedValues[EvtSystemProviderName].StringVal);
		
		eventId = renderedValues[EvtSystemEventID].UInt16Val;
		if (renderedValues[EvtSystemQualifiers].Type != EvtVarTypeNull)
		{
			eventId = MAKELONG(renderedValues[EvtSystemEventID].UInt16Val, renderedValues[EvtSystemQualifiers].UInt16Val);
		}

		level = renderedValues[EvtSystemLevel].ByteVal;

		ULONGLONG timeStamp = renderedValues[EvtSystemTimeCreated].FileTimeVal;
		date.dwHighDateTime = static_cast<DWORD>((timeStamp >> 32) & 0xFFFFFFFF);
		date.dwLowDateTime  = static_cast<DWORD>(timeStamp & 0xFFFFFFFF);
				
		EvtFunctions().EvtClose(contextHandle);
	}

	XmlEventLogEntry::XmlEventLogEntry( XmlEventLogEntry && x )
		: eventHandle(x.eventHandle)
	{
		x.eventHandle = NULL;
	}

	XmlEventLogEntry& XmlEventLogEntry::operator=( XmlEventLogEntry && x )
	{
		eventHandle = x.eventHandle;

		x.eventHandle = NULL;

		return *this;
	}

	XmlEventLogEntry::~XmlEventLogEntry()
	{
		EvtFunctions().EvtClose(eventHandle);
	}

	std::wstring XmlEventLogEntry::GetDescription()
	{
		return FormatMessage(EvtFormatMessageEvent);
	}

	std::wstring XmlEventLogEntry::GetSource()
	{
		std::wstring response = FormatMessage(EvtFormatMessageProvider);
		if (response.size() == 0)
		{
			return providerName;
		}
		else
		{
 			if (boost::starts_with(response, L"Microsoft-Windows-")) 
			{ 
				response.erase(response.begin(), response.begin() + 18);
			}
			return response;
		}
	}

	XmlEventLog::XmlEventLog( wchar_t* logPath /*= L"System"*/, wchar_t* query /*= L"Event/System"*/ )
		: queryHandle(EvtFunctions().EvtQuery(NULL, logPath, query, EvtQueryChannelPath | EvtQueryReverseDirection))
	{
		if (queryHandle == NULL)
		{
			Win32Exception::ThrowFromLastError();
		}
	}

	XmlEventLog::~XmlEventLog()
	{
		EvtFunctions().EvtClose(queryHandle);
	}

	std::vector<std::unique_ptr<EventLogEntry>> XmlEventLog::ReadEvents()
	{
		std::vector<std::unique_ptr<EventLogEntry>> eventLogEntries;
		HANDLE eventHandles[10];
		DWORD numReturned = 0;

		while (EvtFunctions().EvtNext(queryHandle, 10, eventHandles, INFINITE, 0, &numReturned))
		{
			for (DWORD i = 0; i < numReturned; ++i)
			{
				eventLogEntries.emplace_back(std::unique_ptr<EventLogEntry>(new XmlEventLogEntry(eventHandles[i])));
			}
		}

		DWORD errorStatus = GetLastError();
		if (errorStatus != ERROR_NO_MORE_ITEMS)
		{
			Win32Exception::Throw(errorStatus);
		}

		return eventLogEntries;
	}
	
	EventLogEntry::EventLogEntry( EventLogEntry && e )
		: date(e.date)
		, level(e.level)
		, eventId(e.eventId)
	{
		e.date.dwLowDateTime = 0;
		e.date.dwHighDateTime = 0;
		e.level = 0;
		e.eventId = 0;
	}

	EventLogEntry::EventLogEntry()
	{

	}

	EventLogEntry::~EventLogEntry()
	{

	}


	EventLog::~EventLog()
	{

	}

}}