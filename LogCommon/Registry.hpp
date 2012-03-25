#pragma once
#include <string>
#include <memory>
#include <windows.h>
#include <boost/noncopyable.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include "DdkStructures.h"

namespace Instalog { namespace SystemFacades {

	class BasicRegistryValue
	{
	protected:
		BasicRegistryValue() {} // Not intended for client construction.
		~BasicRegistryValue() {}
	public:
		virtual std::vector<unsigned char>::const_iterator cbegin() const = 0;
		virtual std::vector<unsigned char>::const_iterator cend() const = 0;
		virtual std::size_t size() const = 0;
		virtual DWORD GetType() const = 0;
		DWORD GetDword() const;
		DWORD GetDwordStrict() const;
		__int64 GetQWord() const;
		__int64 GetQWordStrict() const;
		std::wstring GetString() const;
		std::wstring GetStringStrict() const;
		std::vector<std::wstring> GetMultiStringArray() const;
		std::vector<std::wstring> GetCommaStringArray() const;
	};

	class RegistryValue
	{
		DWORD type_;
		std::vector<unsigned char> data_;
	public:
		RegistryValue(DWORD type, std::vector<unsigned char> && data);
		RegistryValue(RegistryValue && other);
		virtual DWORD GetType() const;
		virtual std::size_t size() const;
		virtual std::vector<unsigned char>::const_iterator cbegin() const;
		virtual std::vector<unsigned char>::const_iterator cend() const;
	};

	class RegistryValueAndData
	{
		std::vector<unsigned char> innerBuffer_;
		typedef KEY_VALUE_FULL_INFORMATION valueType;
		KEY_VALUE_FULL_INFORMATION const* Cast() const;
	public:
		RegistryValueAndData(std::vector<unsigned char> && buff);
		RegistryValueAndData(RegistryValueAndData && other);
		std::wstring GetName() const;
		virtual DWORD GetType() const;
		virtual std::size_t size() const;
		virtual std::vector<unsigned char>::const_iterator cbegin() const;
		virtual std::vector<unsigned char>::const_iterator cend() const;
		bool operator<(RegistryValueAndData const& rhs) const;
	};

	class RegistryKeySizeInformation
	{
		unsigned __int64 lastWriteTime_;
		unsigned __int32 numberOfSubkeys_;
		unsigned __int32 numberOfValues_;
	public:
		RegistryKeySizeInformation(unsigned __int64 lastWriteTime, unsigned __int32 numberOfSubkeys, unsigned __int32 numberOfValues);
		unsigned __int32 GetNumberOfSubkeys() const;
		unsigned __int32 GetNumberOfValues() const;
		unsigned __int64 GetLastWriteTime() const;
	};

	class RegistryKey : boost::noncopyable
	{
		HANDLE hKey_;
	public:
		RegistryKey();
		explicit RegistryKey(HANDLE hKey);
		RegistryKey(RegistryKey && other);
		RegistryKey& operator=(RegistryKey other);
		~RegistryKey();
		HANDLE GetHkey() const;
		RegistryValue const GetValue(std::wstring name) const;
		RegistryValue const operator[](std::wstring name) const;
		void Delete();
		bool Valid() const;
		bool Invalid() const;

		std::vector<std::wstring> EnumerateValueNames() const;
		std::vector<RegistryValueAndData> EnumerateValues() const;

		std::wstring GetName() const;
		RegistryKeySizeInformation GetSizeInformation() const;
		std::vector<std::wstring> EnumerateSubKeyNames() const;
		std::vector<RegistryKey> EnumerateSubKeys(REGSAM samDesired = KEY_ALL_ACCESS) const;

		static RegistryKey Open(std::wstring const& key, REGSAM samDesired = KEY_ALL_ACCESS);
		static RegistryKey Open(RegistryKey const& parent, std::wstring const& key, REGSAM samDesired = KEY_ALL_ACCESS);
		static RegistryKey Open(RegistryKey const& parent, UNICODE_STRING& key, REGSAM samDesired = KEY_ALL_ACCESS);
		static RegistryKey Create(
			std::wstring const& key,
			REGSAM samDesired = KEY_ALL_ACCESS,
			DWORD options = REG_OPTION_NON_VOLATILE
		);
		static RegistryKey Create(
			RegistryKey const& parent,
			std::wstring const& key,
			REGSAM samDesired = KEY_ALL_ACCESS,
			DWORD options = REG_OPTION_NON_VOLATILE
		);
	};

}}
