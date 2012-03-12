#pragma once
#include <string>
#include <windows.h>

namespace Instalog { namespace SystemFacades {

	class RegistryValue
	{
		HKEY hKey_;
		std::wstring name_;
	public:
		RegistryValue(HKEY hKey, std::wstring const& name);
		RegistryValue(HKEY hKey, std::wstring && name);
		RegistryValue(RegistryValue const& other);
		RegistryValue(RegistryValue && other);
	};

	class RegistryKey
	{
		HKEY hKey_;
		RegistryKey(HKEY hKey);
		RegistryKey(RegistryKey const& other); //Noncopyable
		RegistryKey& operator=(RegistryKey const& other);
	public:
		RegistryKey(RegistryKey && other);
		~RegistryKey();
		HKEY GetHkey() const;
		RegistryValue GetValue(std::wstring const& name);
		RegistryValue GetValue(std::wstring && name);
		RegistryValue operator[](std::wstring const& name);
		RegistryValue operator[](std::wstring && name);

		std::unique_ptr<RegistryKey> Open(std::wstring const& key, REGSAM samDesired = KEY_ALL_ACCESS);
		std::unique_ptr<RegistryKey> Create(
			std::wstring const& key,
			REGSAM samDesired = KEY_ALL_ACCESS,
			DWORD options = REG_OPTION_NON_VOLATILE,
		);
	};

}}
