// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <functional>
#include <atlbase.h>
#include <comdef.h>
#include <Wbemidl.h>
#include "Com.hpp"
#include "SecurityCenter.hpp"
#include "StockOutputFormats.hpp"
#include "Registry.hpp"
#include "PseudoHjt.hpp"

namespace Instalog {
    using namespace SystemFacades;

	std::wstring PseudoHjt::GetScriptCommand() const
	{
		return L"pseudohijackthis";
	}

	std::wstring PseudoHjt::GetName() const
	{
		return L"Pseudo HijackThis";
	}

	LogSectionPriorities PseudoHjt::GetPriority() const
	{
		return SCANNING;
	}

    /**
     * Security center output for the PseudoHJT section.
     *
     * @param [in,out] output The stream to recieve the output.
     */
	static void SecurityCenterOutput( std::wostream& output )
	{
		using SystemFacades::SecurityProduct;
		auto products = SystemFacades::EnumerateSecurityProducts();
		for (auto it = products.cbegin(); it != products.cend(); ++it)
		{
			output << it->GetTwoLetterPrefix()
				<< L": [" << it->GetInstanceGuid() << L"] ";
			if (it->IsEnabled())
			{
				output << L'E';
			}
			else
			{
				output << L'D';
			}
			switch (it->GetUpdateStatus())
			{
			case SecurityProduct::OutOfDate:
				output << L'O';
				break;
			case SecurityProduct::UpToDate:
				output << L'U';
				break;
			}
			output << L' ' << it->GetName() << L'\n';
		}
	}

    static void GeneralProcess(std::wostream& out, std::wstring& target)
    {
        GeneralEscape(target, L'#', L'\n');
        out << target;
    }

    static void FileProcess(std::wostream& out, std::wstring& target)
    {
        WriteDefaultFileOutput(out, target);
    }

    static void HttpProcess(std::wostream& out, std::wstring& target)
    {
        UrlEscape(target, L'#', L'\n');
        out << target;
    }

    static bool DoNothingFilter(RegistryValueAndData const& target)
    {
        auto str = target.GetString();
        return target.GetName().empty() && str.empty();
    }
    
    static void ValueMajorBasedEnumeration(
        std::wostream& output,
        std::wstring const& root,
        std::wstring const& prefix,
        std::function<void (std::wostream& out, std::wstring& source)> rightProcess = FileProcess,
        std::function<bool(RegistryValueAndData const& entry)> filter = DoNothingFilter
        )
    {
        RegistryKey key(RegistryKey::Open(root, KEY_QUERY_VALUE));
        if (key.Invalid())
        {
            DWORD err = ::GetLastError();
            if (err == STATUS_OBJECT_NAME_NOT_FOUND)
            {
                return;
            }
            else
            {
                Win32Exception::ThrowFromNtError(::GetLastError());
            }
        }
        auto values = key.EnumerateValues();
        std::vector<std::pair<std::wstring, std::wstring>> pods;
        std::for_each(values.cbegin(), values.cend(), [&](RegistryValueAndData const& val) {
            if (filter(val))
            {
                return;
            }
            pods.emplace_back(std::pair<std::wstring, std::wstring>(val.GetName(), val.GetString()));
        });
        std::sort(pods.begin(), pods.end());
        std::for_each(pods.begin(), pods.end(), [&](std::pair<std::wstring, std::wstring>& current) {
            GeneralEscape(current.first, L'#', L']');
            output << prefix << L": [" << current.first << L"] ";
            rightProcess(output, current.second);
            output << L'\n';
        });
    }

    void RunKeyOutput(std::wostream& output, std::wstring const& name)
    {
#ifdef _M_X64
        ValueMajorBasedEnumeration(output, L"\\Registry\\Machine\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\" + name, name);
        ValueMajorBasedEnumeration(output, L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\" + name, name + L"64");
#else
        ValueMajorBasedEnumeration(output, L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\" + name, name);
#endif
    }

	void PseudoHjt::Execute(
		std::wostream& output,
		ScriptSection const&,
		std::vector<std::wstring> const&
	) const
	{
		SecurityCenterOutput(output);
        RunKeyOutput(output, L"Run");
        RunKeyOutput(output, L"RunOnce");
        RunKeyOutput(output, L"RunServices");
        RunKeyOutput(output, L"RunServicesOnce");
#ifdef _M_X64
        ValueMajorBasedEnumeration(output, L"\\Registry\\Machine\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run", L"ExplorerRun");
        ValueMajorBasedEnumeration(output, L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run", L"ExplorerRun64");
#else
        ValueMajorBasedEnumeration(output, L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run", L"ExplorerRun");
#endif
	}

}
