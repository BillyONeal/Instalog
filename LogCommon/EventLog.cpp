#include "pch.hpp"
#include "EventLog.hpp"
#include "Win32Exception.hpp"

namespace Instalog { namespace SystemFacades {

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

}}