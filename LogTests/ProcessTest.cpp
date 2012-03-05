#include <algorithm>
#include <string>
#include <windows.h>
#define PSAPI_VERSION 1
#include <Psapi.h>
#include "gtest/gtest.h"
#include <LogCommon/Win32Exception.hpp>
#include <LogCommon/Process.hpp>

#pragma comment(lib, "psapi.lib")

using Instalog::SystemFacades::ProcessEnumerator;
using Instalog::SystemFacades::ErrorAccessDeniedException;

TEST(Process, CanEnumerateAndCompareToProcessIds)
{
	std::size_t currentProcess = ::GetCurrentProcessId();
	ProcessEnumerator enumerator;
	ASSERT_NE(std::find(enumerator.begin(), enumerator.end(), currentProcess), enumerator.end());
}

TEST(Process, CanGetModuleBaseName)
{
	wchar_t currentProcessExecutable[MAX_PATH];
	::GetModuleFileName(NULL, currentProcessExecutable, MAX_PATH);
	std::wstring baseName = currentProcessExecutable;
	ProcessEnumerator enumerator;
	bool couldFindMyOwnProcess = false;
	for (ProcessEnumerator::iterator it = enumerator.begin(); it != enumerator.end(); ++it)
	{
		try
		{
			if (it->GetExecutablePath() == baseName) 
			{
				couldFindMyOwnProcess = true;
			}
		}
		catch (ErrorAccessDeniedException const&)
		{ } //Not much we can really do about these.
	}
	ASSERT_TRUE(couldFindMyOwnProcess);
}
