#include "RuntimeDynamicLinker.hpp"

#pragma once

namespace Instalog { namespace SystemFacades {

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
