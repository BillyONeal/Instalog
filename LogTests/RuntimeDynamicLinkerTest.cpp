// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include "LogCommon/File.hpp"
#include "LogCommon/RuntimeDynamicLinker.hpp"
#include "LogCommon/Win32Exception.hpp"

using namespace Instalog::SystemFacades;

TEST(RuntimeDynamicLinker, NonexistentDllThrows)
{
	ASSERT_THROW(RuntimeDynamicLinker ntdll(L"IDoNotExist.dll"), ErrorModuleNotFoundException);
}

TEST(RuntimeDynamicLinker, CanLoadFunction)
{
	typedef NTSTATUS (NTAPI *NtCloseT)(HANDLE);
	RuntimeDynamicLinker ntdll(L"ntdll.dll");
	NtCloseT ntClose = ntdll.GetProcAddress<NtCloseT>("NtClose");
	HANDLE h = ::CreateFileW(L"DeleteMe.txt", GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, 0);
	ntClose(h);
	ASSERT_FALSE(File::Exists(L"DeleteMe.txt"));
}
