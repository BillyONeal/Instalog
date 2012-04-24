// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <LogCommon/Win32Exception.hpp>
#include <LogCommon/ScopedPrivilege.hpp>

using Instalog::SystemFacades::ScopedPrivilege;
using Instalog::SystemFacades::Win32Exception;

static bool HasPrivilege(LPCWSTR privilegeName)
{
	HANDLE procToken;
	::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &procToken);
	DWORD bufferLength = 1024;
	std::vector<unsigned char> buff;
	DWORD lastError = ERROR_INSUFFICIENT_BUFFER;
	while(lastError == ERROR_INSUFFICIENT_BUFFER)
	{
		buff.resize(static_cast<std::vector<unsigned char>::size_type>(bufferLength));
		if (::GetTokenInformation(procToken, TokenPrivileges, &buff[0], bufferLength, &bufferLength) == 0)
		{
			lastError = ::GetLastError();
		}
		else
		{
			lastError = ERROR_SUCCESS;
		}
	}
	if (lastError != ERROR_SUCCESS)
	{
		Win32Exception::Throw(lastError);
	}
	TOKEN_PRIVILEGES const* privStruct = reinterpret_cast<TOKEN_PRIVILEGES*>(&buff[0]);
	LUID privValue;
	if (::LookupPrivilegeValueW(nullptr, privilegeName, &privValue) == 0)
	{
		Win32Exception::ThrowFromLastError();
	}
	for (std::size_t idx = 0; idx < privStruct->PrivilegeCount; ++idx)
	{
		LUID_AND_ATTRIBUTES const& currentPrivilege = privStruct->Privileges[idx];
		if (currentPrivilege.Attributes != SE_PRIVILEGE_ENABLED && currentPrivilege.Attributes != SE_PRIVILEGE_ENABLED_BY_DEFAULT)
		{
			continue;
		}
		if (currentPrivilege.Luid.HighPart != privValue.HighPart)
		{
			continue;
		}
		if (currentPrivilege.Luid.LowPart != privValue.LowPart)
		{
			continue;
		}
		return true;
	}
	return false;
}

TEST(ScopedPrivilege, PrivilegeGetsTaken)
{
	ScopedPrivilege priv(SE_INC_WORKING_SET_NAME);
	ASSERT_TRUE(::HasPrivilege(SE_INC_WORKING_SET_NAME));
}

TEST(ScopedPrivilege, PrivilegeGetsReleased)
{
	{
		ScopedPrivilege priv(SE_INC_WORKING_SET_NAME);
		ASSERT_TRUE(::HasPrivilege(SE_INC_WORKING_SET_NAME));
	}
	ASSERT_FALSE(::HasPrivilege(SE_INC_WORKING_SET_NAME));
}
