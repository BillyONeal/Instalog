#include "pch.hpp"
#include <cassert>
#include <limits>
#include "Win32Exception.hpp"
#include "RuntimeDynamicLinker.hpp"
#include "Registry.hpp"
#include "DdkStructures.h"

namespace Instalog { namespace SystemFacades {

	static NtOpenKeyFunc PNtOpenKey = GetNtDll().GetProcAddress<NtOpenKeyFunc>("NtOpenKey");
	static NtCreateKeyFunc PNtCreateKey = GetNtDll().GetProcAddress<NtCreateKeyFunc>("NtCreateKey");
	static NtCloseFunc PNtClose = GetNtDll().GetProcAddress<NtCloseFunc>("NtClose");

	RegistryKey::~RegistryKey()
	{
		if (hKey_ != INVALID_HANDLE_VALUE)
		{
			PNtClose(hKey_);
		}
	}

	HANDLE RegistryKey::GetHkey() const
	{
		return hKey_;
	}

	RegistryKey::RegistryKey( HANDLE hKey )
	{
		hKey_ = hKey;
	}

	RegistryKey::RegistryKey( RegistryKey && other )
	{
		hKey_ = other.hKey_;
		other.hKey_ = INVALID_HANDLE_VALUE;
	}

	RegistryValue RegistryKey::operator[]( std::wstring const& name )
	{
		return GetValue(name);
	}

	RegistryValue RegistryKey::operator[]( std::wstring && name )
	{
		return GetValue(name);
	}

	RegistryValue RegistryKey::GetValue( std::wstring const& name )
	{
		return RegistryValue(hKey_, name);
	}

	RegistryValue RegistryKey::GetValue( std::wstring && name )
	{
		return RegistryValue(hKey_, name);
	}

	std::unique_ptr<RegistryKey> RegistryKey::Open( std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/ )
	{
		HANDLE hOpened;
		OBJECT_ATTRIBUTES attribs;
		UNICODE_STRING ustrKey = WstringToUnicodeString(key);
		attribs.Length = sizeof(attribs);
		attribs.RootDirectory = NULL;
		attribs.ObjectName = &ustrKey;
		attribs.Attributes = OBJ_CASE_INSENSITIVE;
		attribs.SecurityDescriptor = NULL;
		attribs.SecurityQualityOfService = NULL;
		NTSTATUS errorCheck = PNtOpenKey(&hOpened, samDesired, &attribs);
		if (NT_SUCCESS(errorCheck))
		{
			return std::unique_ptr<RegistryKey>(new RegistryKey(hOpened));
		}
		else
		{
			::SetLastError(errorCheck);
			return std::unique_ptr<RegistryKey>(nullptr);
		}
	}

	std::unique_ptr<RegistryKey> RegistryKey::Create( std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/, DWORD options /*= REG_OPTION_NON_VOLATILE */ )
	{
		HANDLE hOpened;
		OBJECT_ATTRIBUTES attribs;
		UNICODE_STRING ustrKey = WstringToUnicodeString(key);
		attribs.Length = sizeof(attribs);
		attribs.RootDirectory = NULL;
		attribs.ObjectName = &ustrKey;
		attribs.Attributes = OBJ_CASE_INSENSITIVE;
		attribs.SecurityDescriptor = NULL;
		attribs.SecurityQualityOfService = NULL;
		NTSTATUS errorCheck = PNtCreateKey(&hOpened, samDesired, &attribs, NULL, NULL, options, NULL);
		if (NT_SUCCESS(errorCheck))
		{
			return std::unique_ptr<RegistryKey>(new RegistryKey(hOpened));
		}
		else
		{
			::SetLastError(errorCheck);
			return std::unique_ptr<RegistryKey>(nullptr);
		}
	}

	RegistryValue::RegistryValue( HANDLE hKey, std::wstring const& name )
		: hKey_(hKey)
		, name_(name)
	{ }

	RegistryValue::RegistryValue( HANDLE hKey, std::wstring && name )
		: hKey_(hKey)
		, name_(std::move(name))
	{ }

	RegistryValue::RegistryValue( RegistryValue const& other )
		: hKey_(other.hKey_)
		, name_(other.name_)
	{ }

	RegistryValue::RegistryValue( RegistryValue && other )
		: hKey_(other.hKey_)
		, name_(std::move(other.name_))
	{ }

}}
