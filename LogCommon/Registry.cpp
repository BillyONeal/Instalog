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
	static NtQueryKeyFunc PNtQueryKey = GetNtDll().GetProcAddress<NtQueryKeyFunc>("NtQueryKey");
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

	RegistryKeySizeInformation RegistryKey::GetSizeInformation() const
	{
		auto const buffSize = 32768ul;
		unsigned char buffer[buffSize];
		auto keyFullInformation = reinterpret_cast<KEY_FULL_INFORMATION const*>(&buffer[0]);
		auto bufferPtr = reinterpret_cast<void *>(&buffer[0]);
		ULONG resultLength = 0;
		NTSTATUS errorCheck = PNtQueryKey(GetHkey(), KeyFullInformation, bufferPtr, buffSize, &resultLength);
		if (NT_ERROR(errorCheck))
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		return RegistryKeySizeInformation(
			keyFullInformation->LastWriteTime.QuadPart,
			keyFullInformation->SubKeys,
			keyFullInformation->Values
			);
	}

	std::vector<std::wstring> RegistryKey::EnumerateSubKeyNames() const
	{
		const auto bufferLength = 32768;
		std::vector<std::wstring> subkeys;
		NTSTATUS errorCheck;
		ULONG index = 0;
		ULONG resultLength = 0;
		unsigned char buff[bufferLength];
		auto basicInformation = reinterpret_cast<KEY_BASIC_INFORMATION const*>(buff);
		for(;;)
		{
			errorCheck = PNtEnumerateKey(
				GetHkey(),
				index++,
				KeyBasicInformation,
				buff,
				bufferLength,
				&resultLength
			);
			if (!NT_SUCCESS(errorCheck))
			{
				break;
			}
			subkeys.emplace_back(
				std::wstring(basicInformation->Name,
				             basicInformation->NameLength / sizeof(wchar_t)
							)
			);
		}
		if (errorCheck != STATUS_NO_MORE_ENTRIES)
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		std::sort(subkeys.begin(), subkeys.end());
		return std::move(subkeys);
	}

	RegistryKey& RegistryKey::operator=( RegistryKey other )
	{
		std::swap(hKey_, other.hKey_);
		return *this;
	}

	RegistryValue::RegistryValue( HANDLE hKey, std::wstring && name )
		: hKey_(hKey)
		, name_(std::move(name))
	{ }

	RegistryValue::RegistryValue( RegistryValue && other )
		: hKey_(other.hKey_)
		, name_(std::move(other.name_))
	{ }


	RegistryKeySizeInformation::RegistryKeySizeInformation( unsigned __int64 lastWriteTime, unsigned __int32 numberOfSubkeys, unsigned __int32 numberOfValues ) : lastWriteTime_(lastWriteTime)
		, numberOfSubkeys_(numberOfSubkeys)
		, numberOfValues_(numberOfValues)
	{ }

	unsigned __int32 RegistryKeySizeInformation::GetNumberOfSubkeys() const
	{
		return numberOfSubkeys_;
	}

	unsigned __int32 RegistryKeySizeInformation::GetNumberOfValues() const
	{
		return numberOfValues_;
	}

	unsigned __int64 RegistryKeySizeInformation::GetLastWriteTime() const
	{
		return lastWriteTime_;
	}

}}
