// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <windows.h>
#include "Win32Exception.hpp"

namespace Instalog { namespace SystemFacades {

	/// @brief	Scoped privilege. Takes a privilege for a scope, and relenquishes
	/// 		that privilege on scope exit.
	class ScopedPrivilege
	{
		HANDLE hThisProcess;
		HANDLE hThisProcessToken;
		TOKEN_PRIVILEGES privileges;
	public:
		/// @brief	Constructor. Takes the indicated privilege.
		///
		/// @param	privilegeName	Name of the privilege to take.
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
		/// @brief	Destructor. Releases the privilege taken in the constructor.
		~ScopedPrivilege()
		{
			privileges.Privileges[0].Attributes = 0;
			::AdjustTokenPrivileges(hThisProcessToken, FALSE, &privileges, 0, nullptr, nullptr);
			::CloseHandle(hThisProcessToken);
		}
	};

}}