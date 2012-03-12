#include "pch.hpp"
#include <ntdef.h>
#include "Win32Exception.hpp"
#include "Registry.hpp"
#include "DdkStructures.h"

namespace Instalog { namespace SystemFacades {


	RegistryKey::~RegistryKey()
	{
		if (hKey_ != INVALID_HANDLE_VALUE)
		{
			::RegCloseKey(hKey_);
		}
	}

	HKEY RegistryKey::GetHkey() const
	{
		return hKey_;
	}

	RegistryKey::RegistryKey( HKEY hKey )
	{
		hKey_ = hKey;
	}

	RegistryKey::RegistryKey( RegistryKey && other )
	{
		hKey_ = other.hKey_;
		other.hKey_ = reinterpret_cast<HKEY>(INVALID_HANDLE_VALUE);
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

	RegistryValue::RegistryValue( HKEY hKey, std::wstring const& name )
		: hKey_(hKey)
		, name_(name)
	{ }

	RegistryValue::RegistryValue( HKEY hKey, std::wstring && name )
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
