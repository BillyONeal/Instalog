#include "gtest/gtest.h"
#include "LogCommon/RuntimeDynamicLinker.hpp"
#include "LogCommon/Win32Exception.hpp"

using Instalog::SystemFacades::RuntimeDynamicLinker;
using Instalog::SystemFacades::ErrorModuleNotFoundException;

TEST(RuntimeDynamicLinker, NonexistentDllThrows)
{
	ASSERT_THROW(RuntimeDynamicLinker ntdll(L"IDoNotExist.dll"), ErrorModuleNotFoundException);
}
