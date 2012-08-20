// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include <algorithm>
#include <string>
#include <windows.h>
#define PSAPI_VERSION 1
#include <Psapi.h>
#include "../LogCommon/Win32Exception.hpp"
#include "../LogCommon/Process.hpp"

#pragma comment(lib, "psapi.lib")

using Instalog::SystemFacades::ProcessEnumerator;
using Instalog::SystemFacades::Process;
using Instalog::SystemFacades::ErrorAccessDeniedException;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_CLASS(ProcessTests)
{
public:
	TEST_METHOD(CanEnumerateAndCompareToProcessIds)
	{
		std::size_t currentProcess = ::GetCurrentProcessId();
		ProcessEnumerator enumerator;
		Assert::IsFalse(std::find(enumerator.begin(), enumerator.end(), currentProcess) == enumerator.end());
	}

	TEST_METHOD(CanRunConcurrentSearches)
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

		Assert::IsTrue(processesA == processesB);
	}

	TEST_METHOD(CanGetProcessExecutables)
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
		Assert::IsTrue(couldFindMyOwnProcess);
	}

	TEST_METHOD(CanGetProcessCommandLines)
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
		Assert::IsTrue(couldFindMyOwnProcess);
	}

	TEST_METHOD(NtoskrnlIsInTheBuilding)
	{
		ProcessEnumerator enumerator;
		for (ProcessEnumerator::iterator it = enumerator.begin(); it != enumerator.end(); ++it)
		{
			if (it->GetProcessId() == 4) 
			{
				Assert::IsTrue(boost::iequals(L"C:\\Windows\\System32\\Ntoskrnl.exe", it->GetExecutablePath()));
			}
		}
	}
};
