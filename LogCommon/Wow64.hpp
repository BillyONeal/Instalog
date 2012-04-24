// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "Library.hpp"

#pragma once

namespace Instalog { namespace SystemFacades {

	/// @brief	Query if the current process is running under WOW64.
	///
	/// @return	true if running under WOW64, false otherwise
	inline bool IsWow64()
	{
		try
		{
			RuntimeDynamicLinker kernel32(L"kernel32.dll");
			typedef BOOL (WINAPI *IsWow64ProcessT)(
				__in   HANDLE hProcess,
				__out  PBOOL Wow64Process
				);
			auto IsWow64ProcessFunc = kernel32.GetProcAddress<IsWow64ProcessT>("IsWow64Process");
			BOOL answer;
			BOOL errorCheck = IsWow64ProcessFunc(GetCurrentProcess(), &answer);
			if (errorCheck == 0)
			{
				Win32Exception::ThrowFromLastError();
			}
			return answer != 0;
		}
		catch (ErrorProcedureNotFoundException const&)
		{
			return false;
		}
	}

}}
