#include "pch.hpp"
#include <iomanip>
#include <boost/io/ios_state.hpp>
#include <windows.h>
#include "Win32Exception.hpp"
#include "StockOutputFormats.hpp"

namespace Instalog {
	
	void WriteDefaultDateFormat(std::wostream &str, unsigned __int64 time)
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
	}
	void WriteMillisecondDateFormat(std::wostream &str, unsigned __int64 time)
	{

	}
	void WriteFileAttributes( std::wostream &str, unsigned __int32 attributes )
	{
		if (attributes & FILE_ATTRIBUTE_DIRECTORY)
			str << L'd';
		else
			str << L'-';

		if (attributes & FILE_ATTRIBUTE_COMPRESSED)
			str << L'c';
		else
			str << L'-';

		if (attributes & FILE_ATTRIBUTE_SYSTEM)
			str << L's';
		else
			str << L'-';

		if (attributes & FILE_ATTRIBUTE_HIDDEN)
			str << L'h';
		else
			str << L'-';

		if (attributes & FILE_ATTRIBUTE_ARCHIVE)
			str << L'a';
		else
			str << L'-';

		if (attributes & FILE_ATTRIBUTE_TEMPORARY)
			str << L't';
		else
			str << L'-';

		if (attributes & FILE_ATTRIBUTE_READONLY)
			str << L'r';
		else
			str << L'w';

		if (attributes & FILE_ATTRIBUTE_REPARSE_POINT)
			str << L'r';
		else
			str << L'-';
	}
	void WriteDefaultFileOutput(std::wostream &str, std::wstring const& targetFile)
	{

	}
	void WriteFileListingFile( std::wostream &str, std::wstring const& targetFile )
	{

	}

}
