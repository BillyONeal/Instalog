#pragma once
#include <string>
#include <memory>
#include <windows.h>
#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include "DdkStructures.h"

namespace Instalog { namespace SystemFacades {

	struct InvalidRegistryDataTypeException : public std::exception
	{
		virtual char const* what() const
		{
			return "Invalid Registry Value Type";
		}
	};

	class BasicRegistryValue
	{
	protected:
		BasicRegistryValue() {} // Not intended for client construction.
		~BasicRegistryValue() {}
	public:
		virtual unsigned char const* cbegin() const = 0;
		virtual unsigned char const* cend() const = 0;
		wchar_t const* wcbegin() const;
		wchar_t const* wcend() const;
		virtual std::size_t size() const = 0;
		bool empty() const { return size() == 0; }
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

	class RegistryValue : public BasicRegistryValue
	{
		DWORD type_;
		std::vector<unsigned char> data_;
	public:
		RegistryValue(DWORD type, std::vector<unsigned char> && data);
		RegistryValue(RegistryValue && other);
		virtual DWORD GetType() const;
		virtual std::size_t size() const;
		virtual unsigned char const* cbegin() const;
		virtual unsigned char const* cend() const;
	};

	class RegistryValueAndData : public BasicRegistryValue
	{
		std::vector<unsigned char> innerBuffer_;
		typedef KEY_VALUE_FULL_INFORMATION valueType;
		KEY_VALUE_FULL_INFORMATION const* Cast() const;
	public:
		RegistryValueAndData(std::vector<unsigned char> && buff);
		RegistryValueAndData(RegistryValueAndData && other);
		RegistryValueAndData& operator=(RegistryValueAndData other)
		{
			innerBuffer_ = std::move(other.innerBuffer_);
			return *this;
		}
		std::wstring GetName() const;
		virtual DWORD GetType() const;
		virtual std::size_t size() const;
		virtual unsigned char const* cbegin() const;
		virtual unsigned char const* cend() const;
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
