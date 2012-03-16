#pragma once
#include <ostream>
#include <string>
#include <vector>
#include "Win32Glue.hpp"

namespace Instalog {

	//time here is a FILETIME structure (though we're passing a plain __int64 to
	//avoid having to include Windows.h here)
	void WriteDefaultDateFormat(std::wostream &str, unsigned __int64 time);
	void WriteMillisecondDateFormat(std::wostream &str, unsigned __int64 time);
	void WriteCurrentMillisecondDate(std::wostream &str);
	void WriteFileAttributes(std::wostream &str, unsigned __int32 attributes);
	void WriteDefaultFileOutput(std::wostream &str, std::wstring targetFile);
	void WriteFileListingFile(std::wostream &str, std::wstring const& targetFile);

	void WriteScriptHeader(std::wostream &log);
	void WriteScriptFooter(std::wostream &log);
	void WriteMemoryInformation(std::wostream &str);
	void WriteOsVersion(std::wostream &log);

}
