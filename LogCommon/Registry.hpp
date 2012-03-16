#pragma once
#include <string>
#include <memory>
#include <windows.h>

namespace Instalog { namespace SystemFacades {

	class RegistryValue
	{
		HANDLE hKey_;
		std::wstring name_;
	public:
		RegistryValue(HANDLE hKey, std::wstring const& name);
		RegistryValue(HANDLE hKey, std::wstring && name);
		RegistryValue(RegistryValue const& other);
		RegistryValue(RegistryValue && other);
	};

	class RegistryKey
	{
		HANDLE hKey_;
		RegistryKey(RegistryKey const& other); //Noncopyable
		RegistryKey& operator=(RegistryKey const& other);
	public:
		typedef std::unique_ptr<RegistryKey> Ptr;
		explicit RegistryKey(HANDLE hKey);
		RegistryKey(RegistryKey && other);
		~RegistryKey();
		HANDLE GetHkey() const;
		RegistryValue GetValue(std::wstring const& name);
		RegistryValue GetValue(std::wstring && name);
		RegistryValue operator[](std::wstring const& name);
		RegistryValue operator[](std::wstring && name);
		void Delete();

		static Ptr Open(std::wstring const& key, REGSAM samDesired = KEY_ALL_ACCESS);
		static Ptr Open(Ptr const& parent, std::wstring const& key, REGSAM samDesired = KEY_ALL_ACCESS);
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
