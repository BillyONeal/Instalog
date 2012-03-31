// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <iomanip>
#include <boost/io/ios_state.hpp>
#include <windows.h>
#include "Win32Exception.hpp"
#include "RuntimeDynamicLinker.hpp"
#include "Path.hpp"
#include "File.hpp"
#include "StockOutputFormats.hpp"
#include "InstalogVersion.hpp"

using Instalog::SystemFacades::Win32Exception;
using Instalog::SystemFacades::File;

namespace Instalog {

	static void DateFormatImpl(std::wostream &str, unsigned __int64 time, bool ms)
	{
		using namespace boost::io;
		SYSTEMTIME st;
		if (FileTimeToSystemTime(reinterpret_cast<FILETIME const*>(&time), &st) == 0)
		{
			Win32Exception::ThrowFromLastError();
		}
		wios_fill_saver saveFill(str);
		ios_flags_saver saveFlags(str);
		str.fill(L'0');
		str << std::setw(4) << st.wYear << L'-'
			<< std::setw(2) << st.wMonth << L'-'<< std::setw(2) << st.wDay << L' '
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
	void WriteDefaultFileOutput( std::wostream &str, std::wstring targetFile )
	{
		if (Path::ResolveFromCommandLine(targetFile) == false)
		{
			str << targetFile << L" [x]";
			return;
		}
		std::wstring companyInfo(L" ");
		try
		{
			companyInfo.append(File::GetCompany(targetFile));
		}
		catch (Win32Exception const&)
		{
			companyInfo = L"";
		}
		WIN32_FILE_ATTRIBUTE_DATA fad = File::GetExtendedAttributes(targetFile);
		unsigned __int64 size = 
			static_cast<unsigned __int64>(fad.nFileSizeHigh) << 32
			| fad.nFileSizeLow;
		str << targetFile << L" [" << size << L' ';
		unsigned __int64 ctime = 
			static_cast<unsigned __int64>(fad.ftCreationTime.dwHighDateTime) << 32
			| fad.ftCreationTime.dwLowDateTime;
		WriteDefaultDateFormat(str, ctime);
		str << companyInfo << L"]";
	}

	void WriteFileListingFile( std::wostream &str, std::wstring const& targetFile )
	{
		WIN32_FILE_ATTRIBUTE_DATA fad = File::GetExtendedAttributes(targetFile);
		unsigned __int64 size = 
			static_cast<unsigned __int64>(fad.nFileSizeHigh) << 32
			| fad.nFileSizeLow;
		unsigned __int64 ctime = 
			static_cast<unsigned __int64>(fad.ftCreationTime.dwHighDateTime) << 32
			| fad.ftCreationTime.dwLowDateTime;
		unsigned __int64 mtime = 
			static_cast<unsigned __int64>(fad.ftLastWriteTime.dwHighDateTime) << 32
			| fad.ftLastWriteTime.dwLowDateTime;
		WriteDefaultDateFormat(str, ctime);
		str << L" . ";
		WriteDefaultDateFormat(str, mtime);
		str << L' ' << size << L' ';
		WriteFileAttributes(str, fad.dwFileAttributes);
		str << L' ' << targetFile;
	}

	void WriteMemoryInformation( std::wostream &log )
	{
		unsigned __int64 availableRam = 0;
		unsigned __int64 totalRam = 0;
		MEMORYSTATUSEX memStatus;
		memStatus.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&memStatus);

		availableRam = memStatus.ullAvailPhys;

		Instalog::SystemFacades::RuntimeDynamicLinker kernel32(L"kernel32.dll");
		typedef BOOL (WINAPI *GetPhysicallyInstalledFunc)(PULONGLONG TotalMemoryInKilobytes);
		try 
		{
			auto gpFunc = kernel32.GetProcAddress<GetPhysicallyInstalledFunc>("GetPhysicallyInstalledSystemMemory");
			gpFunc(&totalRam);
			totalRam *= 1024;
		}
		catch (SystemFacades::ErrorProcedureNotFoundException &)
		{
			totalRam = memStatus.ullTotalPhys;
		}
		totalRam /= 1024*1024;
		availableRam /= 1024*1024;

		log << availableRam << L'/' << totalRam << L" MB Free";
	}

	void WriteOsVersion( std::wostream &log )
	{
		typedef BOOL (WINAPI *GetProductInfoFunc)(DWORD, DWORD, DWORD, DWORD, PDWORD);
		OSVERSIONINFOEX versionInfo;
		ZeroMemory(&versionInfo, sizeof(OSVERSIONINFOEX));
		versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		GetVersionEx((LPOSVERSIONINFO)&versionInfo);

		DWORD productType = 0;
		Instalog::SystemFacades::RuntimeDynamicLinker kernel32(L"kernel32.dll");

		try
		{
			auto getProductInfo = kernel32.GetProcAddress<GetProductInfoFunc>("GetProductInfo");
			getProductInfo(6, 2, 0, 0, &productType);
		}
		catch (SystemFacades::ErrorProcedureNotFoundException const&)
		{ } // Expected on earlier OSes

		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);

		log << L"Windows® ";

		switch(versionInfo.dwMajorVersion)
		{
		case 5:
			switch(versionInfo.dwMinorVersion)
			{
			case 0:
				log << L"2000 ";
				if (versionInfo.wProductType == VER_NT_WORKSTATION)
					log << L"Professional";
				else
				{
					if (versionInfo.wSuiteMask & VER_SUITE_DATACENTER)
						log << L"Datacenter Server";
					else if (versionInfo.wSuiteMask & VER_SUITE_ENTERPRISE)
						log << L"Enterprise Server";
					else
						log << L"Server";
				}
				break;
			case 1:
				log << L"XP ";
				if (GetSystemMetrics(SM_MEDIACENTER))
					log << L"Media Center Edition";
				else if (GetSystemMetrics(SM_STARTER))
					log << L"Starter Edition";
				else if (GetSystemMetrics(SM_TABLETPC))
					log << L"Tablet PC Edition";
				else if (versionInfo.wSuiteMask & VER_SUITE_PERSONAL)
					log << L"Home Edition";
				else
					log << L"Professional Edition";
				break;
			case 2:
				if( GetSystemMetrics(SM_SERVERR2) )
					log << L"Server 2003 R2 ";
				else if ( versionInfo.wSuiteMask == VER_SUITE_STORAGE_SERVER )
					log << L"Storage Server 2003";
				else if ( versionInfo.wSuiteMask == /*VER_SUITE_WH_SERVER*/ 0x00008000 )
					log << L"Home Server";
				else if ( versionInfo.wProductType == VER_NT_WORKSTATION && systemInfo.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
					log << L"XP Professional x64 Edition";
				else
					log << L"Server 2003 ";

				if ( versionInfo.wProductType != VER_NT_WORKSTATION )
				{
					if ( systemInfo.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
					{
						if( versionInfo.wSuiteMask & VER_SUITE_DATACENTER )
							log << L"Datacenter x64 Edition";
						else if( versionInfo.wSuiteMask & VER_SUITE_ENTERPRISE )
							log << L"Enterprise x64 Edition";
						else
							log << L"Standard x64 Edition" ;
					}
					else
					{
						if ( versionInfo.wSuiteMask & VER_SUITE_COMPUTE_SERVER )
							log << L"Compute Cluster Edition";
						else if( versionInfo.wSuiteMask & VER_SUITE_DATACENTER )
							log << L"Datacenter Edition";
						else if( versionInfo.wSuiteMask & VER_SUITE_ENTERPRISE )
							log << L"Enterprise Edition";
						else if ( versionInfo.wSuiteMask & VER_SUITE_BLADE )
							log << L"Web Edition";
						else
							log << L"Standard Edition";
					}
				}
			}
			break;
		case 6:
			switch(versionInfo.dwMinorVersion)
			{
			case 0:
				if (versionInfo.wProductType == VER_NT_WORKSTATION)
					log << L"Vista ";
				else
					log << L"Server 2008 ";

				break;
			case 1:
				if (versionInfo.wProductType == VER_NT_WORKSTATION)
					log << L"7 ";
				else
					log << L"Server 2008 R2 ";
				break;
			case 2:
				if (versionInfo.wProductType == VER_NT_WORKSTATION)
					log << L"8 ";
				else
					log << L"Server 8 ";
			}
			switch(productType)
			{
			case PRODUCT_BUSINESS:
				log << L"Business Edition";
				break;
			case PRODUCT_BUSINESS_N:
				log << L"Business Edition N";
				break;
			case PRODUCT_CLUSTER_SERVER:
				log << L"HPC Edition";
				break;
			case PRODUCT_DATACENTER_SERVER:
				log << L"Server Datacenter Edition";
				break;
			case PRODUCT_DATACENTER_SERVER_CORE:
				log << L"Server Datacenter Edition (Server Core)";
				break;
			case /*PRODUCT_DATACENTER_SERVER_CORE_V*/ 0x00000027:
				log << L"Server Datacenter Edition without Hyper-V (Server Core)";
				break;
			case /*PRODUCT_DATACENTER_SERVER_V*/ 0x00000025:
				log << L"Server Datacenter Edition without Hyper-V";
				break;
			case PRODUCT_ENTERPRISE:
				log << L"Enterprise Edition";
				break;
			case /*PRODUCT_ENTERPRISE_E*/ 0x00000046:
				log << L"Enterprise Edition E";
				break;
			case /*PRODUCT_ENTERPRISE_N*/ 0x0000001B:
				log << L"Enterprise Edition N";
				break;
			case PRODUCT_ENTERPRISE_SERVER:
				log << L"Server Enterprise Edition";
				break;
			case PRODUCT_ENTERPRISE_SERVER_CORE:
				log << L"Server Enterprise Edition (Server Core)";
				break;
			case /*PRODUCT_ENTERPRISE_SERVER_CORE_V*/ 0x00000029:
				log << L"Server Enterprise Edition without Hyper-V (Server Core)";
				break;
			case PRODUCT_ENTERPRISE_SERVER_IA64:
				log << L"Server Enterprise Edition for Itanium-based Systems";
				break;
			case /*PRODUCT_ENTERPRISE_SERVER_V*/ 0x00000026:
				log << L"Server Enterprise Edition without Hyper-V";
				break;
			case PRODUCT_HOME_BASIC:
				log << L"Home Basic Edition";
				break;
			case /*PRODUCT_HOME_BASIC_E*/ 0x00000043:
				log << L"Home Basic Edition E";
				break;
			case /*PRODUCT_HOME_BASIC_N*/ 0x00000005:
				log << L"Home Basic Edition N";
				break;
			case PRODUCT_HOME_PREMIUM:
				log << L"Home Premium Edition";
				break;
			case /*PRODUCT_HOME_PREMIUM_E*/ 0x00000044:
				log << L"Home Premium Edition E";
				break;
			case /*PRODUCT_HOME_PREMIUM_N*/ 0x0000001A:
				log << L"Home Premium Edition N";
				break;
			case /*PRODUCT_HYPERV*/ 0x0000002A:
				log << L"Hyper-V Server Edition";
				break;
			case /*PRODUCT_MEDIUMBUSINESS_SERVER_MANAGEMENT*/ 0x0000001E:
				log << L"Essential Business Server Management Server";
				break;
			case /*PRODUCT_MEDIUMBUSINESS_SERVER_MESSAGING*/ 0x00000020:
				log << L"Essential Business Server Messaging Server";
				break;
			case /*PRODUCT_MEDIUMBUSINESS_SERVER_SECURITY*/ 0x0000001F:
				log << L"Essential Business Server Security Server";
				break;
			case /*PRODUCT_PROFESSIONAL*/ 0x00000030:
				log << L"Professional Edition";
				break;
			case /*PRODUCT_PROFESSIONAL_E*/ 0x00000045:
				log << L"Professional Edition E";
				break;
			case /*PRODUCT_PROFESSIONAL_N*/ 0x00000031:
				log << L"Professional Edition N";
				break;
			case PRODUCT_SERVER_FOR_SMALLBUSINESS:
				log << L"for Windows Essential Server Solutions";
				break;
			case /*PRODUCT_SERVER_FOR_SMALLBUSINESS_V*/ 0x00000023:
				log << L"without Hyper-V for Windows Essential Server Solutions";
				break;
			case /*PRODUCT_SERVER_FOUNDATION*/ 0x00000021:
				log << L"Server Foundation Edition";
				break;
			case PRODUCT_SMALLBUSINESS_SERVER:
				log << L"Small Business Server";
				break;
			case PRODUCT_STANDARD_SERVER:
				log << L"Server Standard Edition";
				break;
			case PRODUCT_STANDARD_SERVER_CORE:
				log << L"Server Standard Edition (Server Core)";
				break;
			case /*PRODUCT_STANDARD_SERVER_CORE_V*/ 0x00000028:
				log << L"Server Standard Edition without Hyper-V (Server Core)";
				break;
			case /*PRODUCT_STANDARD_SERVER_V*/ 0x00000024:
				log << L"Server Standard Edition without Hyper-V";
				break;
			case PRODUCT_STARTER:
				log << L"Starter Edition";
				break;
			case /*PRODUCT_STARTER_E*/ 0x00000042:
				log << L"Starter Edition E";
				break;
			case /*PRODUCT_STARTER_N*/ 0x0000002F:
				log << L"Starter Edition N";
				break;
			case PRODUCT_STORAGE_ENTERPRISE_SERVER:
				log << L"Storage Server Enterprise Edition";
				break;
			case PRODUCT_STORAGE_EXPRESS_SERVER:
				log << L"Storage Server Express Edition";
				break;
			case PRODUCT_STORAGE_STANDARD_SERVER:
				log << L"Storage Server Standard Edition";
				break;
			case PRODUCT_STORAGE_WORKGROUP_SERVER:
				log << L"Storage Server Workgroup Edition";
				break;
			case PRODUCT_ULTIMATE:
				log << L"Ultimate Edition";
				break;
			case /*PRODUCT_ULTIMATE_E*/ 0x00000047:
				log << L"Ultimate Edition E";
				break;
			case /*PRODUCT_ULTIMATE_N*/ 0x0000001C:
				log << L"Ultimate Edition N";
				break;
			case PRODUCT_WEB_SERVER:
				log << L"Web Edition";
				break;
			case /*PRODUCT_WEB_SERVER_CORE*/ 0x0000001D:
				log << L"Web Edition (Server Core)";
				break;
			case /*PRODUCT_CONSUMER_PREVIEW*/ 0x0000004A:
				log << L"Consumer Preview";
				break;
			default:
				{
					log << L"UNKNOWN EDITION (GetProductInfo -> 0x" << std::hex << productType << L")";
				}
			}
			break;
		}
		if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
		{
			log << L" x86 ";
		}
		else if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		{
			log << L" x64 ";
		}
		log << versionInfo.dwMajorVersion << L'.' << versionInfo.dwMinorVersion << L'.' << versionInfo.dwBuildNumber << L'.' << versionInfo.wServicePackMajor;
	}

	void WriteScriptHeader( std::wostream &log )
	{
		log << L"Instalog " INSTALOG_VERSION;
		switch(GetSystemMetrics(SM_CLEANBOOT))
		{
		case 1:
			log << L" MINIMAL";
			break;
		case 2:
			log << L" NETWORK";
			break;
		}
		log << L"\nRun By ";
		wchar_t userName[257]; //UNLEN + 1
		DWORD userNameLength = 257;
		GetUserName(userName, &userNameLength);
		log << std::wstring(userName, userNameLength - 1);
		log << L" on ";

		TIME_ZONE_INFORMATION tzInfo;
		if (GetTimeZoneInformation(&tzInfo) == TIME_ZONE_ID_DAYLIGHT)
		{
			tzInfo.Bias += tzInfo.DaylightBias;
		}
		WriteCurrentMillisecondDate(log);
		log << L" [GMT " << std::showpos << (-tzInfo.Bias / 60) << L':'
			<< std::noshowpos << std::setw(2) << std::setfill(L'0')
			<< (-tzInfo.Bias % 60) << L"]\n";

		//HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Internet Explorer\\Version
		//HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Runtime Environment\\BrowserJavaVersion
		//HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Macromedia\FlashPlayer\\CurrentVersion (Replace , with .)
		//HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Adobe\Installer\???\\ProductVersion

		WriteOsVersion(log);
		log << L' ';
		WriteMemoryInformation(log);
		log << L"\n\n";
	}

	void WriteScriptFooter( std::wostream &log )
	{
		log << L"Instalog " INSTALOG_VERSION L" finished ";
		WriteCurrentMillisecondDate(log);
		log << L'\n';
	}

	void WriteCurrentMillisecondDate( std::wostream &str )
	{
		SYSTEMTIME st;
		FILETIME ft;
		GetLocalTime(&st);
		SystemTimeToFileTime(&st, &ft);
		unsigned __int64 msDate = Instalog::FiletimeToInteger(ft);
		WriteMillisecondDateFormat(str, msDate);
	}

}
