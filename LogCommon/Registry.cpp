// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <strsafe.h>
#include <cassert>
#include <type_traits>
#include <limits>
#include <iterator>
#include <array>
#include <functional>
#include <boost/lexical_cast.hpp>
#include "Win32Exception.hpp"
#include "StringUtilities.hpp"
#include "Library.hpp"
#include "Registry.hpp"

namespace Instalog { namespace SystemFacades {

	static NtOpenKeyFunc PNtOpenKey = GetNtDll().GetProcAddress<NtOpenKeyFunc>("NtOpenKey");
	static NtCreateKeyFunc PNtCreateKey = GetNtDll().GetProcAddress<NtCreateKeyFunc>("NtCreateKey");
	static NtCloseFunc PNtClose = GetNtDll().GetProcAddress<NtCloseFunc>("NtClose");
	static NtDeleteKeyFunc PNtDeleteKey = GetNtDll().GetProcAddress<NtCloseFunc>("NtDeleteKey");
	static NtQueryKeyFunc PNtQueryKey = GetNtDll().GetProcAddress<NtQueryKeyFunc>("NtQueryKey");
	static NtEnumerateKeyFunc PNtEnumerateKey = GetNtDll().GetProcAddress<NtEnumerateKeyFunc>("NtEnumerateKey");
	static NtEnumerateValueKeyFunc PNtEnumerateValueKeyFunc = GetNtDll().GetProcAddress<NtEnumerateValueKeyFunc>("NtEnumerateValueKey");
	static NtQueryValueKeyFunc PNtQueryValueKeyFunc = GetNtDll().GetProcAddress<NtQueryValueKeyFunc>("NtQueryValueKey");

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
		: hKey_(other.hKey_)
	{
		other.hKey_ = INVALID_HANDLE_VALUE;
	}

	RegistryKey::RegistryKey()
		: hKey_(INVALID_HANDLE_VALUE)
	{ }

	RegistryValue const RegistryKey::operator[]( std::wstring name ) const
	{
		return GetValue(std::move(name));
	}

	RegistryValue const RegistryKey::GetValue( std::wstring name ) const
	{
		UNICODE_STRING valueName(WstringToUnicodeString(name));
		std::vector<unsigned char> buff(MAX_PATH);
		NTSTATUS errorCheck;
		do 
		{
			ULONG resultLength = 0;
			errorCheck = PNtQueryValueKeyFunc(
				hKey_,
				&valueName,
				KeyValuePartialInformation,
				buff.data(),
				static_cast<ULONG>(buff.size()),
				&resultLength
				);
			if ((errorCheck == STATUS_BUFFER_TOO_SMALL || errorCheck == STATUS_BUFFER_OVERFLOW) && resultLength != 0)
			{
				buff.resize(resultLength);
			}
		} while (errorCheck == STATUS_BUFFER_TOO_SMALL || errorCheck == STATUS_BUFFER_OVERFLOW);
		if (!NT_SUCCESS(errorCheck))
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		auto partialInfo = reinterpret_cast<KEY_VALUE_PARTIAL_INFORMATION const*>(buff.data());
		DWORD type = partialInfo->Type;
		ULONG len = partialInfo->DataLength;
		buff.erase(buff.begin(), buff.begin() + 3*sizeof(ULONG));
		buff.resize(len);
		return RegistryValue(type, std::move(buff));
	}

	static RegistryKey RegistryKeyOpen( HANDLE hRoot, UNICODE_STRING& key, REGSAM samDesired )
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
		if (!NT_SUCCESS(errorCheck))
		{
			::SetLastError(errorCheck);
			hOpened = INVALID_HANDLE_VALUE;
		}
		return RegistryKey(hOpened);
	}

	static RegistryKey RegistryKeyOpen( HANDLE hRoot, std::wstring const& key, REGSAM samDesired )
	{
		UNICODE_STRING ustrKey = WstringToUnicodeString(key);
		return RegistryKeyOpen(hRoot, ustrKey, samDesired);
	}

	RegistryKey RegistryKey::Open( std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/ )
	{
		return RegistryKeyOpen(0, key, samDesired);
	}

	RegistryKey RegistryKey::Open( RegistryKey const& parent, std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/ )
	{
		return RegistryKeyOpen(parent.GetHkey(), key, samDesired);
	}

	RegistryKey RegistryKey::Open( RegistryKey const& parent, UNICODE_STRING& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/ )
	{
		return RegistryKeyOpen(parent.GetHkey(), key, samDesired);
	}

	static RegistryKey RegistryKeyCreate( HANDLE hRoot, std::wstring const& key, REGSAM samDesired, DWORD options )
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
		if (!NT_SUCCESS(errorCheck))
		{
			::SetLastError(errorCheck);
			hOpened = INVALID_HANDLE_VALUE;
		}
		return RegistryKey(hOpened);
	}

	RegistryKey RegistryKey::Create( std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/, DWORD options /*= REG_OPTION_NON_VOLATILE */ )
	{
		return RegistryKeyCreate(0, key, samDesired, options);
	}

	RegistryKey RegistryKey::Create( RegistryKey const& parent, std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/, DWORD options /*= REG_OPTION_NON_VOLATILE */ )
	{
		return RegistryKeyCreate(parent.GetHkey(), key, samDesired, options);
	}

	void RegistryKey::Delete()
	{
		NTSTATUS errorCheck = PNtDeleteKey(GetHkey());
		if (!NT_SUCCESS(errorCheck))
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
		if (!NT_SUCCESS(errorCheck))
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		return RegistryKeySizeInformation(
			keyFullInformation->LastWriteTime.QuadPart,
			keyFullInformation->SubKeys,
			keyFullInformation->Values
			);
	}

	std::wstring RegistryKey::GetName() const
	{
		auto const buffSize = 32768ul;
		unsigned char buffer[buffSize];
		auto keyBasicInformation = reinterpret_cast<KEY_NAME_INFORMATION const*>(&buffer[0]);
		auto bufferPtr = reinterpret_cast<void *>(&buffer[0]);
		ULONG resultLength = 0;
		NTSTATUS errorCheck = PNtQueryKey(GetHkey(), KeyNameInformation, bufferPtr, buffSize, &resultLength);
		if (!NT_SUCCESS(errorCheck))
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		return std::wstring(keyBasicInformation->Name, keyBasicInformation->NameLength / sizeof(wchar_t));
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
		return std::move(subkeys);
	}

	RegistryKey& RegistryKey::operator=( RegistryKey other )
	{
		std::swap(hKey_, other.hKey_);
		return *this;
	}

	std::vector<RegistryKey> RegistryKey::EnumerateSubKeys(REGSAM samDesired /* = KEY_ALL_ACCESS */) const
	{
		std::vector<std::wstring> names(EnumerateSubKeyNames());
		std::vector<RegistryKey> result(names.size());
		std::transform(names.cbegin(), names.cend(), result.begin(), 
			[this, samDesired] (std::wstring const& name) -> RegistryKey {
			return Open(*this, name, samDesired);
		});
		return std::move(result);
	}

	bool RegistryKey::Valid() const
	{
		return hKey_ != INVALID_HANDLE_VALUE;
	}

	bool RegistryKey::Invalid() const
	{
		return !Valid();
	}

	std::vector<std::wstring> RegistryKey::EnumerateValueNames() const
	{
		std::vector<std::wstring> result;
		ULONG index = 0;
		const ULONG valueNameStructSize = 16384 * sizeof(wchar_t) +
			sizeof(KEY_VALUE_BASIC_INFORMATION);
		std::aligned_storage<valueNameStructSize,
			std::alignment_of<KEY_VALUE_BASIC_INFORMATION>::value>::type buff;
		auto basicValueInformation = reinterpret_cast<KEY_VALUE_BASIC_INFORMATION*>(&buff);
		for(;;)
		{
			ULONG resultLength;
			NTSTATUS errorCheck = PNtEnumerateValueKeyFunc(
				hKey_,
				index++,
				KeyValueBasicInformation,
				basicValueInformation,
				valueNameStructSize,
				&resultLength);
			if (NT_SUCCESS(errorCheck))
			{
				result.emplace_back(std::wstring(basicValueInformation->Name,
					basicValueInformation->NameLength / sizeof(wchar_t)));
			}
			else if (errorCheck == STATUS_NO_MORE_ENTRIES)
			{
				break;
			}
			else
			{
				Win32Exception::ThrowFromNtError(errorCheck);
			}
		}
		return std::move(result);
	}

	std::vector<RegistryValueAndData> RegistryKey::EnumerateValues() const
	{
		std::vector<RegistryValueAndData> result;
		std::vector<unsigned char> buff;
		NTSTATUS errorCheck = 0;
		for (ULONG index = 0; NT_SUCCESS(errorCheck); ++index)
		{
			ULONG elementSize = 260; // MAX_PATH
			do
			{
				buff.resize(elementSize);
				errorCheck = PNtEnumerateValueKeyFunc(
					hKey_,
					index,
					KeyValueFullInformation,
					buff.data(),
					static_cast<ULONG>(buff.size()),
					&elementSize);
			} while (errorCheck == STATUS_BUFFER_OVERFLOW || errorCheck == STATUS_BUFFER_TOO_SMALL);
			if (NT_SUCCESS(errorCheck))
			{
				result.emplace_back(RegistryValueAndData(std::move(buff)));
			}
		}
		if (errorCheck != STATUS_NO_MORE_ENTRIES)
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		return std::move(result);
	}

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

	RegistryValue::RegistryValue( DWORD type, std::vector<unsigned char> && data )
		: type_(type)
		, data_(data)
	{ }

	RegistryValue::RegistryValue( RegistryValue && other )
		: type_(other.type_)
		, data_(std::move(other.data_))
	{ }

	DWORD RegistryValue::GetType() const
	{
		return type_;
	}

	unsigned char const* RegistryValue::cbegin() const
	{
		return data_.data();
	}

	unsigned char const* RegistryValue::cend() const
	{
		return data_.data() + data_.size();
	}

	std::size_t RegistryValue::size() const
	{
		return data_.size();
	}


	RegistryValueAndData::RegistryValueAndData( std::vector<unsigned char> && buff )
		: innerBuffer_(buff)
	{ }

	RegistryValueAndData::RegistryValueAndData( RegistryValueAndData && other )
		: innerBuffer_(other.innerBuffer_)
	{ }

	std::wstring RegistryValueAndData::GetName() const
	{
		auto casted = Cast();
		return std::wstring(casted->Name, casted->NameLength / sizeof(wchar_t));
	}

	KEY_VALUE_FULL_INFORMATION const* RegistryValueAndData::Cast() const
	{
		return reinterpret_cast<KEY_VALUE_FULL_INFORMATION const*>(innerBuffer_.data());
	}

	unsigned char const* RegistryValueAndData::cbegin() const
	{
		return innerBuffer_.data() + Cast()->DataOffset;
	}

	unsigned char const* RegistryValueAndData::cend() const
	{
		auto casted = Cast();
		auto forwardLength = casted->DataOffset + casted->DataLength;
		return innerBuffer_.data() + forwardLength;
	}

	bool RegistryValueAndData::operator<( RegistryValueAndData const& rhs ) const
	{
		auto casted = Cast();
		auto rhsCasted = rhs.Cast();
		return std::lexicographical_compare(casted->Name, casted->Name + (casted->NameLength/sizeof(wchar_t)),
			                                rhsCasted->Name, rhsCasted->Name + (rhsCasted->NameLength/sizeof(wchar_t)));
	}

	DWORD RegistryValueAndData::GetType() const
	{
		return Cast()->Type;
	}

	std::size_t RegistryValueAndData::size() const
	{
		return Cast()->DataLength;
	}

	static unsigned __int32 BytestreamToDword( unsigned char const* first, unsigned char const* last )
	{
		static_assert(sizeof(unsigned __int32) == sizeof(unsigned char) * 4, "This conversion assumes a 32 bit integer is 4 characters.");
		union
		{
			unsigned __int32 converted;
			unsigned char toConvert[4];
		};
		if (std::distance(first, last) < 4)
		{
			throw ErrorInvalidParameterException();
		}
		std::copy(first, first + 4, toConvert);
		return converted;
	}

	static unsigned __int32 BytestreamToDwordBe( unsigned char const* first, unsigned char const* last )
	{
		static_assert(sizeof(unsigned __int32) == sizeof(unsigned char) * 4, "This conversion assumes a 32 bit integer is 4 characters.");
		union
		{
			unsigned __int32 converted;
			unsigned char toConvert[4];
		};
		if (std::distance(first, last) < 4)
		{
			throw ErrorInvalidParameterException();
		}
		std::copy(first, first + 4, toConvert);
		std::reverse(toConvert, toConvert + 4);
		return converted;
	}

	static unsigned __int64 BytestreamToQword( unsigned char const* first, unsigned char const* last )
	{
		static_assert(sizeof(unsigned __int64) == sizeof(unsigned char) * 8, "This conversion assumes a 64 bit integer is 8 characters.");
		union
		{
			unsigned __int64 converted;
			unsigned char toConvert[8];
		};
		if (std::distance(first, last) < 8)
		{
			throw ErrorInvalidParameterException();
		}
		std::copy(first, first + 8, toConvert);
		return converted;
	}


	DWORD BasicRegistryValue::GetDWord() const
	{
		if (GetType() == REG_DWORD)
		{
			if (size() != 4)
			{
				throw InvalidRegistryDataTypeException();
			}
			return BytestreamToDword(cbegin(), cend());
		}
		else if (GetType() == REG_DWORD_BIG_ENDIAN)
		{
			if (size() != 4)
			{
				throw InvalidRegistryDataTypeException();
			}
			return BytestreamToDwordBe(cbegin(), cend());
		}
		else if (GetType() == REG_QWORD)
		{
			if (size() != 8)
			{
				throw InvalidRegistryDataTypeException();
			}
			__int64 tmp = BytestreamToQword(cbegin(), cend());
			if (tmp > std::numeric_limits<DWORD>::max() || tmp < std::numeric_limits<DWORD>::min())
			{
				throw InvalidRegistryDataTypeException();
			}
			return static_cast<DWORD>(tmp);
		}
		else if (GetType() == REG_SZ || GetType() == REG_EXPAND_SZ)
		{
			try
			{
				return boost::lexical_cast<DWORD>(GetStringStrict());
			}
			catch (boost::bad_lexical_cast const&)
			{
				throw InvalidRegistryDataTypeException();
			}
		}
		throw InvalidRegistryDataTypeException();
	}

	std::wstring BasicRegistryValue::GetStringStrict() const
	{
		auto type = GetType();
		if (type != REG_SZ && type != REG_EXPAND_SZ)
		{
			throw InvalidRegistryDataTypeException();
		}
		return GetString();
	}

	DWORD BasicRegistryValue::GetDWordStrict() const
	{
		if (GetType() != REG_DWORD)
		{
			throw InvalidRegistryDataTypeException();
		}
		return GetDWord();
	}

	unsigned __int64 BasicRegistryValue::GetQWord() const
	{
		if (GetType() == REG_QWORD)
		{
			if (size() != 8)
			{
				throw InvalidRegistryDataTypeException();
			}
			return BytestreamToQword(cbegin(), cend());
		}
		else if (GetType() == REG_DWORD)
		{
			if (size() != 4)
			{
				throw InvalidRegistryDataTypeException();
			}
			return BytestreamToDword(cbegin(), cend());
		}
		else if (GetType() == REG_DWORD_BIG_ENDIAN)
		{
			if (size() != 4)
			{
				throw InvalidRegistryDataTypeException();
			}
			return BytestreamToDwordBe(cbegin(), cend());
		}
		else if (GetType() == REG_SZ || GetType() == REG_EXPAND_SZ)
		{
			try
			{
				return boost::lexical_cast<unsigned __int64>(GetStringStrict());
			}
			catch (boost::bad_lexical_cast const&)
			{
				throw InvalidRegistryDataTypeException();
			}
		}
		throw InvalidRegistryDataTypeException();
	}

	unsigned __int64 BasicRegistryValue::GetQWordStrict() const
	{
		if (GetType() != REG_QWORD)
		{
			throw InvalidRegistryDataTypeException();
		}
		return GetQWord();
	}

	std::wstring BasicRegistryValue::GetString( ) const
	{
		std::wstring result;
		switch (GetType())
		{
		case REG_SZ:
		case REG_EXPAND_SZ:
			if (empty())
			{
				break;
			}
			if (*(wcend() - 1) == L'\0')
			{
				result.assign(wcbegin(), wcend() - 1);
			}
			else
			{
				result.assign(wcbegin(), wcend());
			}
			
			break;
		case REG_DWORD:
			if (size() != 4)
			{
				throw InvalidRegistryDataTypeException();
			}
			result.reserve(14);
			result.assign(L"dword:");
			{
				auto it = cbegin() + 3;
				auto itEnd = cbegin() - 1;
				for (; it != itEnd; --it)
				{
					HexCharacter(*it, result);
				}
			}
			break;
		case REG_QWORD:
			if (size() != 8)
			{
				throw InvalidRegistryDataTypeException();
			}
			result.reserve(22);
			result.assign(L"qword:");
			{
				auto it = cbegin() + 7;
				auto itEnd = cbegin() - 1;
				for (; it != itEnd; --it)
				{
					HexCharacter(*it, result);
				}
			}
			break;
		case REG_DWORD_BIG_ENDIAN:
			if (size() != 4)
			{
				throw InvalidRegistryDataTypeException();
			}
			result.reserve(17);
			result.assign(L"dword-be:");
			{
				auto it = cbegin();
				auto itEnd = cbegin() + 4;
				for (; it != itEnd; ++it)
				{
					HexCharacter(*it, result);
				}
			}
			break;
		default:
			result.reserve(4*size() + 7);
			if (GetType() == REG_BINARY)
			{
				result.assign(L"hex:");
			}
			else
			{
				wchar_t buff[16];
				swprintf_s(buff, L"hex(%d):", GetType());
				result.assign(buff);
			}
			if (size() == 0)
			{
				break;
			}
			HexCharacter(*cbegin(), result);
			{
				auto it = cbegin() + 1;
				auto end = cend();
				for (; it != end; ++it)
				{
					result.push_back(L',');
					HexCharacter(*it, result);
				}
			}
		}
		return std::move(result);
	}

	std::vector<std::wstring> BasicRegistryValue::GetMultiStringArray() const
	{
		if (GetType() != REG_MULTI_SZ)
		{
			throw InvalidRegistryDataTypeException();
		}
		std::vector<std::wstring> answers;
		auto first = wcbegin();
		auto middle = first;
		auto last = wcend();
		while (middle = std::find(first, last, L'\0'), first != last && middle != last)
		{
			if (first != middle)
				answers.emplace_back(std::wstring(first, middle));
			first = middle + 1;
		}
		return std::move(answers);
	}

	wchar_t const* BasicRegistryValue::wcbegin() const
	{
		return reinterpret_cast<wchar_t const*>(cbegin());
	}

	wchar_t const* BasicRegistryValue::wcend() const
	{
		return reinterpret_cast<wchar_t const*>(cend());
	}

	std::vector<std::wstring> BasicRegistryValue::GetCommaStringArray() const
	{
		std::vector<std::wstring> answer;
		std::wstring contents(GetStringStrict());
		boost::algorithm::split(answer, contents, 
			std::bind(std::equal_to<wchar_t>(), std::placeholders::_1, L','));
		std::for_each(answer.begin(), answer.end(), [] (std::wstring & a) {
			boost::algorithm::trim_left(a, std::locale()); });
		return std::move(answer);
	}

}}
