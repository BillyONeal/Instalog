#pragma once
#include <boost/noncopyable.hpp>
#include <string>
#include <windows.h>

namespace Instalog { namespace SystemFacades {

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
	}

}}