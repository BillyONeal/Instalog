#pragma once
#include <windows.h>
#include "Win32Exception.hpp"

namespace Instalog { namespace SystemFacades {

	class ScopedPrivilege
	{
		HANDLE hThisProcess;
		HANDLE hThisProcessToken;
		TOKEN_PRIVILEGES privileges;
	public:
		ScopedPrivilege(LPCWSTR privilegeName)
			: hThisProcess(::GetCurrentProcess())
		{

			if (::OpenProcessToken(hThisProcess, TOKEN_ADJUST_PRIVILEGES, &hThisProcessToken) == 0)
			{
				Win32Exception::ThrowFromLastError();
			}
			privileges.PrivilegeCount = 1;
			privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			if (::LookupPrivilegeValueW(nullptr, privilegeName, &privileges.Privileges[0].Luid) == 0)
			{
				Win32Exception::ThrowFromLastError();
			}
			if (::AdjustTokenPrivileges(hThisProcessToken, FALSE, &privileges, 0, nullptr, nullptr) == 0)
			{
				Win32Exception::ThrowFromLastError();
			}
		}
		~ScopedPrivilege()
		{
			privileges.Privileges[0].Attributes = 0;
			::AdjustTokenPrivileges(hThisProcessToken, FALSE, &privileges, 0, nullptr, nullptr);
			::CloseHandle(hThisProcessToken);
		}
	};

}}