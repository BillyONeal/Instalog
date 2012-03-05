#include <algorithm>
#include <string>
#include <windows.h>
#define PSAPI_VERSION 1
#include <Psapi.h>
#include "gtest/gtest.h"
#include <LogCommon/Process.hpp>

#pragma comment(lib, "psapi.lib")

using Instalog::SystemFacades::ProcessEnumerator;

TEST(Process, CanEnumerateAndCompareToProcessIds)
{
	std::size_t currentProcess = ::GetCurrentProcessId();
	ProcessEnumerator enumerator;
	ASSERT_NE(std::find(enumerator.begin(), enumerator.end(), currentProcess), enumerator.end());
}

TEST(Process, CanGetModuleBaseName)
{
	wchar_t baseNameBuffer[MAX_PATH];
	::GetModuleBaseNameW(::GetCurrentProcess(), NULL, baseNameBuffer, MAX_PATH);
	std::wstring baseName = baseNameBuffer;
	ProcessEnumerator enumerator;
	bool couldFindModuleBaseName = false;
	for (ProcessEnumerator::iterator it = enumerator.begin(); it != enumerator.end(); ++it)
	{
		if (it->GetExecutablePath() == baseName) 
		{
			couldFindModuleBaseName = true;
		}
	}
	ASSERT_TRUE(couldFindModuleBaseName);
}