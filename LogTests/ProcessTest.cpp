// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
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
using Instalog::SystemFacades::Process;
using Instalog::SystemFacades::ErrorAccessDeniedException;

TEST(Process, CanEnumerateAndCompareToProcessIds)
{
	std::size_t currentProcess = ::GetCurrentProcessId();
	ProcessEnumerator enumerator;
	ASSERT_NE(std::find(enumerator.begin(), enumerator.end(), currentProcess), enumerator.end());
}

TEST(Process, CanRunConcurrentSearches)
{
	ProcessEnumerator enumerator;
	std::vector<Process> processesA;
	std::vector<Process> processesB;
	ProcessEnumerator::iterator itDoubled = enumerator.begin();
	for (ProcessEnumerator::iterator it = enumerator.begin(); it != enumerator.end(); ++it)
	{
		processesA.push_back(*it);
		if (itDoubled != enumerator.end())
		{
			processesB.push_back(*itDoubled);
			++itDoubled;
		}
		if (itDoubled != enumerator.end())
		{
			processesB.push_back(*itDoubled);
			++itDoubled;
		}
	}

	ASSERT_EQ(processesA, processesB);
}

TEST(Process, CanGetProcessExecutables)
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

TEST(Process, CanGetProcessCommandLines)
{
	wchar_t const* currentProcessCmdLine = ::GetCommandLineW();
	std::wstring baseName = currentProcessCmdLine;
	ProcessEnumerator enumerator;
	bool couldFindMyOwnProcess = false;
	for (ProcessEnumerator::iterator it = enumerator.begin(); it != enumerator.end(); ++it)
	{
		try
		{
			if (it->GetCmdLine() == baseName) 
			{
				couldFindMyOwnProcess = true;
			}
		}
		catch (ErrorAccessDeniedException const&)
		{ } //Not much we can really do about these.
	}
	ASSERT_TRUE(couldFindMyOwnProcess);
}

TEST(Process, NtoskrnlIsInTheBuilding)
{
	ProcessEnumerator enumerator;
	for (ProcessEnumerator::iterator it = enumerator.begin(); it != enumerator.end(); ++it)
	{
		if (it->GetProcessId() == 4) 
		{
			ASSERT_TRUE(boost::iequals(L"C:\\Windows\\System32\\Ntoskrnl.exe", it->GetExecutablePath()));
		}
	}
}
