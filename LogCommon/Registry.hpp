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
		RegistryValue(HKEY kKey, std::wstring && name);
		RegistryValue(RegistryValue const& other);
		RegistryValue(RegistryValue && other);
	};

	class RegistryKey
	{
		HKEY hKey_;
		RegistryKey(RegistryKey const& other); //Noncopyable
	public:
		RegistryKey(HKEY hKeyRoot, std::wstring const& subkey, DWORD options, REGSAM samDesired);
		RegistryKey(HKEY hKeyRoot, wchar_t const* subkey, DWORD options, REGSAM samDesired);
		RegistryKey(RegistryKey const& hKey, std::wstring const& subkey, DWORD options, REGSAM samDesired);
		RegistryKey(RegistryKey const& hKey, wchar_t const* subkey, DWORD options, REGSAM samDesired);
		RegistryKey(RegistryKey && other);
		~RegistryKey();
		HKEY GetHkey() const;
		RegistryValue GetValue(std::wstring const& name);
		RegistryValue GetValue(std::wstring && name);
		RegistryValue operator[](std::wstring const& name);
		RegistryValue operator[](std::wstring && name);
		void Delete();
		static void Delete(HKEY targetRootKey, std::wstring const& targetSubkey);
	};

}}
