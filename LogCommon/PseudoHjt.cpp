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
     * @param [in,out] output The stream to receive the output.
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

    /**
     * A processing function which writes the target string in the default escaped function,
     * assuming that the newline is the ending delimiter.
     *
     * @param [in,out] out    The output stream.
     * @param [in,out] target Target which should be written.
     */
    static void GeneralProcess(std::wostream& out, std::wstring& target)
    {
        GeneralEscape(target, L'#', L'\n');
        out << target;
    }

    /**
     * A processing function which writes the default file output.
     *
     * @param [in,out] out    The output stream.
     * @param [in,out] target Target name of the file to print.
     */
    static void FileProcess(std::wostream& out, std::wstring& target)
    {
        WriteDefaultFileOutput(out, target);
    }

    /**
     * A processing function which writes the target as a URL.
     *
     * @param [in,out] out    The output stream.
     * @param [in,out] target Target to write as a URL.
     */
    static void HttpProcess(std::wostream& out, std::wstring& target)
    {
        UrlEscape(target, L'#', L'\n');
        out << target;
    }

    /**
     * Executes the "do nothing" filter operation. Filters only completely empty entries.
     *
     * @param target Target for the filter operation.
     *
     * @return true if the item should be filtered, false otherwise.
     */
    static bool DoNothingFilter(RegistryValueAndData const& target)
    {
        auto str = target.GetString();
        return target.GetName().empty() && str.empty();
    }

    /**
     * Value major based enumeration; general method to enumerate a registry key, where a single
     * log line is generated per value.
     *
     * @param [in,out] output The output stream.
     * @param root            The root registry key from where the key is enumerated.
     * @param prefix          The prefix which identifies the log line in the report.
     * @param rightProcess    (optional) The processing function which generates the report of the
     *                        values' data. The default writes the data as a file.
     * @param filter          (optional) a filter function which returns true if the target should
     *                        not be displayed in the output. The default function filters only
     *                        empty entries.
     */
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

    /**
     * Executes enumeration for run keys.
     *
     * @param [in,out] output The output stream.
     * @param name            The name of the run key to enumerate.
     */
    void RunKeyOutput(std::wostream& output, std::wstring const& name)
    {
#ifdef _M_X64
        ValueMajorBasedEnumeration(output, L"\\Registry\\Machine\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\" + name, name);
        ValueMajorBasedEnumeration(output, L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\" + name, name + L"64");
#else
        ValueMajorBasedEnumeration(output, L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\" + name, name);
#endif
    }

    /**
     * Gets the user registry hive root paths.
     *
     * @return A list of the registry hives.
     */
    static std::vector<std::wstring> EnumerateUserHives()
    {
        std::vector<std::wstring> wht;
        wht.push_back(L"\\REGISTRY\\MACHINE\\HARDWARE");
        wht.push_back(L"\\REGISTRY\\MACHINE\\SYSTEM");
        wht.push_back(L"\\REGISTRY\\MACHINE\\BCD00000000");
        wht.push_back(L"\\REGISTRY\\MACHINE\\SOFTWARE");
        wht.push_back(L"\\REGISTRY\\MACHINE\\SECURITY");
        wht.push_back(L"\\REGISTRY\\MACHINE\\SAM");
        std::sort(wht.begin(), wht.end());
        std::vector<std::wstring> hives;
        {
            RegistryKey hiveList(RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\hivelist", KEY_QUERY_VALUE));
            if (hiveList.Invalid())
            {
                Win32Exception::ThrowFromNtError(::GetLastError());
            }
            hives = hiveList.EnumerateValueNames();
        } //Block closes key
        hives.erase(std::remove_if(hives.begin(), hives.end(), [&wht] (std::wstring const& str) -> bool {
            if (boost::algorithm::ends_with(str, L"_Classes"))
            {
                return true;
            }
            return std::binary_search(wht.cbegin(), wht.cend(), str);
        }), hives.end());
        std::sort(hives.begin(), hives.end());
        return std::move(hives);
    }

    static void CommonHjt(std::wostream& , std::wstring )
    {

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
