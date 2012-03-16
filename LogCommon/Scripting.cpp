#include "pch.hpp"
#include <clocale>
#include <functional>
#include <algorithm>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "Scripting.hpp"
#include "RuntimeDynamicLinker.hpp"
#include "StringUtilities.hpp"

using namespace boost::algorithm;
using Instalog::SystemFacades::RuntimeDynamicLinker;

namespace Instalog
{

	void ScriptDispatcher::AddSectionType( std::unique_ptr<ISectionDefinition> sectionTypeToAdd )
	{
		std::wstring title(sectionTypeToAdd->GetName());
		title.erase(std::remove(title.begin(), title.end(), L' '), title.end());
		to_lower(title);
		sectionTypes.emplace(std::make_pair(std::move(title), std::move(sectionTypeToAdd)));
	}

	Script ScriptDispatcher::Parse( std::wstring const& script ) const
	{
		Script result(this);
		std::vector<std::wstring> scriptLines;
		split(scriptLines, script, is_any_of(L"\r\n"), token_compress_on);

		scriptLines.erase(std::remove_if(scriptLines.begin(), scriptLines.end(), [](std::wstring const& a) -> bool {
			return std::find_if(a.begin(), a.end(), std::not1(std::ptr_fun(iswspace))) == a.end();
		}), scriptLines.end());

		std::vector<std::wstring>::iterator begin, end;
		begin = scriptLines.begin();
		end = scriptLines.end();
		while (begin != end)
		{
			if (!starts_with(*begin, L":"))
			{
				++begin;
				continue;
			}
			std::wstring::iterator sectionStart = begin->begin();
			if (sectionStart != begin->end())
			{
				++sectionStart;
			}
			std::wstring::iterator argumentStart = std::find_if(sectionStart, begin->end(), iswspace);
			std::wstring scriptTarget(sectionStart, argumentStart);
			to_lower(scriptTarget);

			auto sectionIterator = sectionTypes.find(scriptTarget);
			if (sectionIterator == sectionTypes.end())
			{
				throw UnknownScriptSectionException(scriptTarget); 
			}
			ISectionDefinition const *def = sectionIterator->second.get();

			if (argumentStart != begin->end())
			{
				++argumentStart;
			}
			std::wstring argument(argumentStart, begin->end());
			++begin;
			std::vector<std::wstring>::iterator endOfOptions = begin;
			endOfOptions = std::find_if(begin, end, [](std::wstring const& a) { return starts_with(a, L":"); });
			result.Add(def, argument, std::vector<std::wstring>(begin, endOfOptions));
		}
		return result;
	}


	bool ScriptSection::operator<( const ScriptSection& rhs ) const
	{
		if (targetSection->GetPriority() != rhs.targetSection->GetPriority())
		{
			return targetSection->GetPriority() < rhs.targetSection->GetPriority();
		}
		if (targetSection->GetName() != rhs.targetSection->GetName())
		{
			return targetSection->GetName() < rhs.targetSection->GetName();
		}
		return argument < rhs.argument;
	}


	Script::Script( ScriptDispatcher const* parent )
		: parent_(parent)
	{ }

	void Script::Add( ISectionDefinition const* def, std::wstring const& arg, std::vector<std::wstring> const& options )
	{
		ScriptSection ss;
		ss.targetSection = def;
		ss.argument = arg;
		auto insertPoint = sections.find(ss);
		if (insertPoint == sections.end())
		{
			sections.insert(std::make_pair(ss, options));
		}
		else
		{
			insertPoint->second.insert(insertPoint->second.end(), options.begin(), options.end());
		}
	}

	std::map<ScriptSection, std::vector<std::wstring>> const& Script::GetSections() const
	{
		return sections;
	}

	static void WriteOsVersionString(std::wostream &log)
	{
		typedef BOOL (WINAPI *GetProductInfoFunc)(DWORD, DWORD, DWORD, DWORD, PDWORD);
		OSVERSIONINFOEX versionInfo;
		ZeroMemory(&versionInfo, sizeof(OSVERSIONINFOEX));
		versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		GetVersionEx((LPOSVERSIONINFO)&versionInfo);

		DWORD productType;
		RuntimeDynamicLinker kernel32(L"kernel32.dll");
		auto getProductInfo = kernel32.GetProcAddress<GetProductInfoFunc>("GetProductInfo");
		if (getProductInfo)
			getProductInfo(6, 2, 0, 0, &productType);
		else
			productType = 0;

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
					if ( systemInfo.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 )
					{
						if( versionInfo.wSuiteMask & VER_SUITE_DATACENTER )
							log << L"Datacenter Edition for Itanium-based Systems";
						else if( versionInfo.wSuiteMask & VER_SUITE_ENTERPRISE )
							log << L"Enterprise Edition for Itanium-based Systems";
					}
					else if ( systemInfo.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
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
					log << L"UNKNOWN EDITION (0x" << std::hex << productType << L")!";
				}
			}
			break;
		}
	}

	void Script::Run( std::wostream& logOutput, IUserInterface *ui ) const
	{
		ui->LogMessage(L"Starting Execution");
		WriteOsVersionString(logOutput);
		for (auto begin = sections.cbegin(), end = sections.cend(); begin != end; ++begin)
		{
			auto header = begin->first.targetSection->GetName();
			std::wstring message(L"Executing " + header);
			ui->LogMessage(message);
			Instalog::Header(header);
			logOutput << header << L"\n";
			begin->first.targetSection->Execute(logOutput, ui, begin->first, begin->second);
		}
		ui->LogMessage(L"Done!");
		ui->ReportFinished();
	}

}
