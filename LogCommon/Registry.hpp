#pragma once
#include <string>
#include <memory>
#include <windows.h>
#include <boost/noncopyable.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include "DdkStructures.h"

namespace Instalog { namespace SystemFacades {

	class RegistryValue
	{
		HANDLE hKey_;
		std::wstring name_;
	public:
		RegistryValue(HANDLE hKey, std::wstring && name);
		RegistryValue(RegistryValue && other);
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
		typedef std::unique_ptr<RegistryKey> Ptr;
		explicit RegistryKey(HANDLE hKey);
		RegistryKey(RegistryKey && other);
		RegistryKey& operator=(RegistryKey other);
		~RegistryKey();
		HANDLE GetHkey() const;
		RegistryValue GetValue(std::wstring name);
		RegistryValue operator[](std::wstring name);
		void Delete();

		std::wstring GetName() const;
		RegistryKeySizeInformation GetSizeInformation() const;
		std::vector<std::wstring> EnumerateSubKeyNames() const;
		std::vector<std::unique_ptr<RegistryKey> > EnumerateSubKeys(REGSAM samDesired = KEY_ALL_ACCESS) const;

		static Ptr Open(std::wstring const& key, REGSAM samDesired = KEY_ALL_ACCESS);
		static Ptr Open(Ptr const& parent, std::wstring const& key, REGSAM samDesired = KEY_ALL_ACCESS);
		static Ptr Open(RegistryKey const* parent, std::wstring const& key, REGSAM samDesired = KEY_ALL_ACCESS);
		static Ptr Open(RegistryKey const* parent, UNICODE_STRING& key, REGSAM samDesired = KEY_ALL_ACCESS);
		static Ptr Create(
			std::wstring const& key,
			REGSAM samDesired = KEY_ALL_ACCESS,
			DWORD options = REG_OPTION_NON_VOLATILE
		);
		static Ptr Create(
			Ptr const& parent,
			std::wstring const& key,
			REGSAM samDesired = KEY_ALL_ACCESS,
			DWORD options = REG_OPTION_NON_VOLATILE
		);
	};

}}
