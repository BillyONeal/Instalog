#include "pch.hpp"
#include <iomanip>
#include <boost/io/ios_state.hpp>
#include <windows.h>
#include "Win32Exception.hpp"
#include "StockOutputFormats.hpp"

namespace Instalog {

	static void DateFormatImpl(std::wostream &str, unsigned __int64 time, bool ms)
	{
		using namespace boost::io;
		SYSTEMTIME st;
		if (FileTimeToSystemTime(reinterpret_cast<FILETIME const*>(&time), &st) == 0)
		{
			SystemFacades::Win32Exception::ThrowFromLastError();
		}
		wios_fill_saver saveFill(str);
		ios_flags_saver saveFlags(str);
		str.fill(L'0');
		str << std::setw(4) << st.wYear << L'-'
			<< std::setw(2) << st.wMonth << L'-'
			<< std::setw(2) << st.wDay << L' '
			<< std::setw(2) << st.wHour << L':'
			<< std::setw(2) << st.wMinute << L':'
			<< std::setw(2) << st.wSecond;
		if (ms)
		{
			str << L'.' << std::setw(4) << st.wMilliseconds;
		}
	}
	
	void WriteDefaultDateFormat(std::wostream &str, unsigned __int64 time)
	{
		DateFormatImpl(str, time, false);
	}
	void WriteMillisecondDateFormat(std::wostream &str, unsigned __int64 time)
	{
		DateFormatImpl(str, time, true);
	}
	void WriteFileAttributes(std::wostream &str, unsigned __int64 time)
	{

	}
	void WriteDefaultFileOutput(std::wostream &str, std::wstring const& targetFile)
	{

	}
	void WriteFileListingFile( std::wostream &str, std::wstring const& targetFile )
	{

	}

}
