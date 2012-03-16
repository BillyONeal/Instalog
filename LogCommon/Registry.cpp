#include "pch.hpp"
#include <cassert>
#include <limits>
#include "Win32Exception.hpp"
#include "RuntimeDynamicLinker.hpp"
#include "Registry.hpp"

namespace Instalog { namespace SystemFacades {

	static NtOpenKeyFunc PNtOpenKey = GetNtDll().GetProcAddress<NtOpenKeyFunc>("NtOpenKey");
	static NtCreateKeyFunc PNtCreateKey = GetNtDll().GetProcAddress<NtCreateKeyFunc>("NtCreateKey");
	static NtCloseFunc PNtClose = GetNtDll().GetProcAddress<NtCloseFunc>("NtClose");
	static NtDeleteKeyFunc PNtDeleteKey = GetNtDll().GetProcAddress<NtCloseFunc>("NtDeleteKey");
	static NtEnumerateKeyFunc PNtEnumerateKey = GetNtDll().GetProcAddress<NtEnumerateKeyFunc>("NtEnumerateKey");

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

	RegistryValue RegistryKey::operator[]( std::wstring name )
	{
		return GetValue(std::move(name));
	}

	RegistryValue RegistryKey::GetValue( std::wstring name )
	{
		return RegistryValue(hKey_, std::move(name));
	}

	static std::unique_ptr<RegistryKey> RegistryKeyOpen( HANDLE hRoot, UNICODE_STRING& key, REGSAM samDesired )
	{
		HANDLE hOpened;
		OBJECT_ATTRIBUTES attribs;
		attribs.Length = sizeof(attribs);
		attribs.RootDirectory = hRoot;
		attribs.ObjectName = &key;
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

	static std::unique_ptr<RegistryKey> RegistryKeyOpen( HANDLE hRoot, std::wstring const& key, REGSAM samDesired )
	{
		UNICODE_STRING ustrKey = WstringToUnicodeString(key);
		return RegistryKeyOpen(hRoot, ustrKey, samDesired);
	}

	std::unique_ptr<RegistryKey> RegistryKey::Open( std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/ )
	{
		return RegistryKeyOpen(0, key, samDesired);
	}

	std::unique_ptr<RegistryKey> RegistryKey::Open( RegistryKey::Ptr const& parent, std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/ )
	{
		return RegistryKeyOpen(parent->GetHkey(), key, samDesired);
	}

	RegistryKey::Ptr RegistryKey::Open( RegistryKey const* parent, UNICODE_STRING& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/ )
	{
		return RegistryKeyOpen(parent->GetHkey(), key, samDesired);
	}

	static std::unique_ptr<RegistryKey> RegistryKeyCreate( HANDLE hRoot, std::wstring const& key, REGSAM samDesired, DWORD options )
	{
		HANDLE hOpened;
		OBJECT_ATTRIBUTES attribs;
		UNICODE_STRING ustrKey = WstringToUnicodeString(key);
		attribs.Length = sizeof(attribs);
		attribs.RootDirectory = hRoot;
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

	std::unique_ptr<RegistryKey> RegistryKey::Create( std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/, DWORD options /*= REG_OPTION_NON_VOLATILE */ )
	{
		return RegistryKeyCreate(0, key, samDesired, options);
	}

	std::unique_ptr<RegistryKey> RegistryKey::Create( RegistryKey::Ptr const& parent, std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/, DWORD options /*= REG_OPTION_NON_VOLATILE */ )
	{
		return RegistryKeyCreate(parent->GetHkey(), key, samDesired, options);
	}

	void RegistryKey::Delete()
	{
		NTSTATUS errorCheck = PNtDeleteKey(GetHkey());
		if (NT_ERROR(errorCheck))
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
	}

	Instalog::SystemFacades::RegistrySubkeyNameIterator RegistryKey::SubKeyNameBegin() const
	{
		return RegistrySubkeyNameIterator(hKey_, 0);
	}

	Instalog::SystemFacades::RegistrySubkeyNameIterator RegistryKey::SubKeyNameEnd() const
	{
		return RegistrySubkeyNameIterator(hKey_, 999);
	}

	RegistryValue::RegistryValue( HANDLE hKey, std::wstring && name )
		: hKey_(hKey)
		, name_(std::move(name))
	{ }

	RegistryValue::RegistryValue( RegistryValue && other )
		: hKey_(other.hKey_)
		, name_(std::move(other.name_))
	{ }

	std::wstring const& RegistrySubkeyNameIterator::dereference() const
	{
		static std::wstring empty(L"");
		return empty;
	}

	void RegistrySubkeyNameIterator::increment()
	{
		currentIndex++;
	}

	void RegistrySubkeyNameIterator::decrement()
	{
		currentIndex--;
	}

	bool RegistrySubkeyNameIterator::equal( RegistrySubkeyNameIterator const& other ) const
	{
		return currentIndex == other.currentIndex;
	}

}}
