// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <functional>
#include <algorithm>
#include <vector>
#include <set>
#include <string>
#include <iterator>
#include <Sddl.h>
#include <comdef.h>
#include <Wbemidl.h>
#include "Com.hpp"
#include "SecurityCenter.hpp"
#include "StockOutputFormats.hpp"
#include "Registry.hpp"
#include "PseudoHjt.hpp"
#include "ScopeExit.hpp"
#include "Dns.hpp"

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

    static std::wstring Get64Suffix()
    {
#ifdef _M_X64
        return L"64";
#else
        return std::wstring();
#endif
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
        HttpEscape(target, L'#', L'\n');
        out << target;
    }

    /**
    * Value major based enumeration; general method to enumerate a registry key, where a single
    * log line is generated per value.
    *
    * @param [in,out] output The output stream.
    * @param root            The root registry key from where the key is enumerated.
    * @param prefix          The prefix which identifies the log line in the report.
    * @param dataProcess     (optional) The processing function which generates the report of the
    *                        values' data. The default writes the data as a file.
    */
    static void ValueMajorBasedEnumeration(
        std::wostream& output,
        std::wstring const& root,
        std::wstring const& prefix,
        std::function<void (std::wostream& out, std::wstring& source)> dataProcess = FileProcess
        )
    {
        RegistryKey key(RegistryKey::Open(root, KEY_QUERY_VALUE));
        if (key.Invalid())
        {
            return;
        }
        auto values = key.EnumerateValues();
        std::vector<std::pair<std::wstring, std::wstring>> pods;
        for (RegistryValueAndData const& val : values)
        {
            pods.emplace_back(std::make_pair(val.GetName(), val.GetString()));
        };

        std::sort(pods.begin(), pods.end());
        for (auto& current : pods)
        {
            GeneralEscape(current.first, L'#', L']');
            output << prefix << L": [" << current.first << L"] ";
            dataProcess(output, current.second);
            output << L'\n';
        };
    }

#pragma warning (push)
#pragma warning (disable: 4100) //Unreferenced formal parameter. (Becomes unreferenced in 32 bit mode)
    /**
    * Value major based enumeration, automatically specialized for 32 bit and 64 bit machines.
    *
    * @param [in,out] output The output stream where output will be written.
    * @param root            The root key of the hive being searched.
    * @param subkey32        The subkey from the root to use on 64 bit machines when accessing the
    *                        32 bit registry view.
    * @param subkey64        The subkey from the root to use on 64 bit machines when accessing the
    *                        64 bit registry view. (Also the only registry view on 32 bit
    *                        machines)
    * @param prefix          The prefix applied to the log lines.
    * @param dataProcess     (optional) [in,out] The process applied to the data before it is
    *                        printed.
    */
    static void ValueMajorBasedEnumerationBitless(
        std::wostream& output,
        std::wstring const& root,
        std::wstring const& subkey32,
        std::wstring const& subkey64,
        std::wstring const& prefix,
        std::function<void (std::wostream& out, std::wstring& source)> dataProcess = FileProcess
        )
    {
#ifdef _M_X64
        ValueMajorBasedEnumeration(output, root + subkey64, prefix + L"64", dataProcess);
        ValueMajorBasedEnumeration(output, root + subkey32, prefix, dataProcess);
#else
        ValueMajorBasedEnumeration(output, root + subkey64, prefix, dataProcess);
#endif
    }
#pragma warning(pop)

    /**
    * Executes enumeration for run keys.
    *
    * @param [in,out] output The output stream.
    * @param runRoot         The root of the hive where the run key is located.
    * @param name            The name of the run key to enumerate.
    */
    void RunKeyOutput(std::wostream& output, std::wstring const& runRoot, std::wstring const& name)
    {
        ValueMajorBasedEnumerationBitless(output, runRoot,
            L"\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\" + name,
            L"\\Software\\Microsoft\\Windows\\CurrentVersion\\" + name, name);
    }

    //TODO: Make the common bits of these and ValueMajorXXX go away.

    /**
    * Subkey major based enumeration; general method to enumerate a registry key, where a
    * single log line is generated per subkey.
    *
    * @param [in,out] output The output stream.
    * @param root            The root registry key from where the key is enumerated.
    * @param prefix          The prefix which identifies the log line in the report.
    * @param valueName       Name of the value in each subkey at which to look.
    * @param dataProcess     (optional) The processing function which generates the report of the
    *                        values' data. The default writes the data as a file.
    */
    static void SubkeyMajorBasedEnumeration(
        std::wostream& output,
        std::wstring const& root,
        std::wstring const& prefix,
        std::wstring const& valueName,
        std::function<void (std::wostream& out, std::wstring& source)> dataProcess = FileProcess
        )
    {
        RegistryKey key(RegistryKey::Open(root, KEY_ENUMERATE_SUB_KEYS));
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
        auto values = key.EnumerateSubKeys(KEY_QUERY_VALUE);
        std::vector<std::pair<std::wstring, std::wstring>> pods;
        for (RegistryKey const& val : values) {
            try
            {
                pods.emplace_back(val.GetName(), val[valueName].GetString());
            }
            catch (ErrorFileNotFoundException const&)
            {
                //Expected
            }
        };
        std::sort(pods.begin(), pods.end());
        for (auto& current : pods)
        {
            GeneralEscape(current.first, L'#', L']');
            output << prefix << L": [" << current.first << L"] ";
            dataProcess(output, current.second);
            output << L'\n';
        };
    }

#pragma warning (push)
#pragma warning (disable: 4100) //Unreferenced formal parameter. (Becomes unreferenced in 32 bit mode)
    /**
    * Subkey major based enumeration, automatically specialized for 32 bit and 64 bit machines.
    *
    * @param [in,out] output The output stream where output will be written.
    * @param root            The root key of the hive being searched.
    * @param subkey32        The subkey from the root to use on 64 bit machines when accessing the
    *                        32 bit registry view.
    * @param subkey64        The subkey from the root to use on 64 bit machines when accessing the
    *                        64 bit registry view. (Also the only registry view on 32 bit
    *                        machines)
    * @param prefix          The prefix applied to the log lines.
    * @param valueName       Name of the value in each subkey at which to look.
    * @param dataProcess     (optional) [in,out] The process applied to the data before it is
    *                        printed.
    */
    static void SubkeyMajorBasedEnumerationBitless(
        std::wostream& output,
        std::wstring const& root,
        std::wstring const& subkey32,
        std::wstring const& subkey64,
        std::wstring const& valueName,
        std::wstring const& prefix,
        std::function<void (std::wostream& out, std::wstring& source)> dataProcess = FileProcess
        )
    {
#ifdef _M_X64
        SubkeyMajorBasedEnumeration(output, root + subkey64, valueName, prefix + L"64", dataProcess);
        SubkeyMajorBasedEnumeration(output, root + subkey32, valueName, prefix, dataProcess);
#else
        SubkeyMajorBasedEnumeration(output, root + subkey64, valueName, prefix, dataProcess);
#endif
    }
#pragma warning(pop)

    /**
    * Gets the user registry hive root paths.
    *
    * @return A list of the registry hives.
    */
    static std::vector<std::wstring> EnumerateUserHives()
    {
        std::vector<std::wstring> hives;
        {
            RegistryKey hiveList(RegistryKey::Open(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\hivelist", KEY_QUERY_VALUE));
            if (hiveList.Invalid())
            {
                Win32Exception::ThrowFromNtError(::GetLastError());
            }
            hives = hiveList.EnumerateValueNames();
        } //Block closes key
        hives.erase(std::remove_if(hives.begin(), hives.end(), [] (std::wstring const& str) -> bool {
            return !boost::algorithm::istarts_with(str, L"\\Registry\\User\\") || boost::algorithm::ends_with(str, L"_Classes");
        }), hives.end());
        std::sort(hives.begin(), hives.end());
        return hives;
    }

    /**
    * Explorer run output.
    *
    * @param [in,out] output The output stream.
    * @param rootKey         The root key where the ExplorerRun entries are located.
    */
    static void ExplorerRunOutput( std::wostream& output, std::wstring const& rootKey ) 
    {
#ifdef _M_X64
        ValueMajorBasedEnumeration(output, rootKey + L"Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run", L"ExplorerRun");
        ValueMajorBasedEnumeration(output, rootKey + L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run", L"ExplorerRun64");
#else
        ValueMajorBasedEnumeration(output, rootKey + L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run", L"ExplorerRun");
#endif
    }

    /**
    * Values that represent possible sources of CLSIDs in ClsidValueBasedOutput queries.
    */
    enum ClsidSource
    {
        NAME,
        VALUE
    };

    enum MasterName
    {
        LOCAL_DATA,
        CLASS_ROOT_DEFAULT
    };

    /**
    * Clsid value based output with the bitness things applied.
    *
    * @param [in,out] output The output stream.
    * @param prefix          The prefix applied to each log line.
    * @param rootKey         The root key.
    * @param subKey          The sub key where the values are located.
    * @param clsidKey        The clsid key for the current bitness.
    * @param backupClsidKey  The backup clsid key (the machine cLSID key for the right bitness)
    * @param source          Source for the CLSID (name or value).
    * @param nameSource      Which name "wins" between the local name and the remote name.
    */
    static void ClsidValueBasedOutputWithBits(
        std::wostream& output,
        std::wstring const& prefix,
        std::wstring const& rootKey,
        std::wstring const& subKey,
        std::wstring const& clsidKey,
        std::wstring const& backupClsidKey,
        ClsidSource source,
        MasterName nameSource
        )
    {
        RegistryKey itemKey(RegistryKey::Open(rootKey + subKey, KEY_QUERY_VALUE));
        if (itemKey.Invalid())
        {
            return;
        }
        auto rawValues = itemKey.EnumerateValues();
        std::vector<std::pair<std::wstring, std::wstring>> values;
        values.reserve(rawValues.size());
        for (auto const& entry : rawValues)
        {
            if (source == VALUE)
            {
                values.emplace_back(entry.GetString(), entry.GetName());
            }
            else
            {
                values.emplace_back(entry.GetName(), entry.GetString());
            }
        }

        for (auto& currentEntry : values)
        {
            //First try the user specific CLSID key.
            RegistryKey clsidKey(RegistryKey::Open(rootKey + clsidKey + currentEntry.first, KEY_QUERY_VALUE));
            if (clsidKey.Invalid())
            {
                //Next try the machine CLSID key.
                clsidKey = RegistryKey::Open(backupClsidKey + currentEntry.first, KEY_QUERY_VALUE);
            }
            RegistryKey inProcKey;
            if (clsidKey.Valid())
            {
                //Open the InProcServer32 subkey of that.
                inProcKey = RegistryKey::Open(clsidKey, L"InProcServer32", KEY_QUERY_VALUE);
            }
            //The CLSID key's name is used if the remote name is the "boss", or if the current try
            //is empty
            if (clsidKey.Valid() && (nameSource == CLASS_ROOT_DEFAULT || currentEntry.second.empty()))
            {
                std::wstring remoteName(clsidKey[L""].GetString());
                //Don't clobber the existing name in the event the actual name in the key is empty.
                if (!remoteName.empty())
                {
                    currentEntry.second = std::move(remoteName);
                }
            }
            std::wstring file;
            if (inProcKey.Valid())
            {
                file = inProcKey[L""].GetString();
            }
            if (currentEntry.second.empty())
            {
                currentEntry.second = L"N/A";
            }
            GeneralEscape(currentEntry.first, L'#', L'=');
            GeneralEscape(currentEntry.second, L'#', L':');
            output << prefix << L": " << currentEntry.second << L": " << currentEntry.first << L'=';
            WriteDefaultFileOutput(output, file);
            output << L'\n';
        }
    }

    /**
    * CLSID value based output.
    *
    * @param [in,out] output The stream to write the output to.
    * @param prefix          The prefix used to identify the type of line generated in the report.
    * @param rootKey         The root key where the check is rooted. The CLASSES key should be
    *                        located in \\Software\\Classes relative to this key.
    * @param subKey          The sub key under the root key\\Software (or \\Software\\Wow6432Node
    *                        on x64 machines) where the CLSID values are located.
    * @param source          Where the cLSIDs themselves are located (the key or the value).
    * @param nameSource      Which name is primary; the name stored with the CLSID, or the name
    *                        stored with the CLASSES key.
    */
    static void ClsidValueBasedOutput(
        std::wostream& output,
        std::wstring const& prefix,
        std::wstring const& rootKey,
        std::wstring const& subKey,
        ClsidSource source,
        MasterName nameSource
        )
    {
#ifdef _M_X64
        ClsidValueBasedOutputWithBits(
            output,
            prefix,
            rootKey,
            L"\\Software\\Wow6432Node" + subKey,
            L"\\Software\\Wow6432Node\\Classes\\CLSID\\",
            L"\\Registry\\Machine\\Software\\Wow6432Node\\Classes\\CLSID\\",
            source,
            nameSource
            );
#endif
        ClsidValueBasedOutputWithBits(
            output,
            prefix + Get64Suffix(),
            rootKey,
            L"\\Software" + subKey,
            L"\\Software\\Classes\\CLSID\\",
            L"\\Registry\\Machine\\Software\\Classes\\CLSID\\",
            source,
            nameSource
            );
    }

    //TODO: Make the common bits of these two go together.
    /**
    * Clsid subkey based output with the bitness things applied.
    *
    * @param [in,out] output The output stream.
    * @param prefix          The prefix applied to each log line.
    * @param rootKey         The root key.
    * @param subKey          The sub key where the values are located.
    * @param clsidKey        The clsid key for the current bitness.
    * @param backupClsidKey  The backup clsid key (the machine cLSID key for the right bitness)
    * @param source          Source for the CLSID (name or value).
    * @param nameSource      Which name "wins" between the local name and the remote name.
    */
    static void ClsidSubkeyBasedOutputWithBits(
        std::wostream& output,
        std::wstring const& prefix,
        std::wstring const& rootKey,
        std::wstring const& subKey,
        std::wstring const& clsidKey,
        std::wstring const& backupClsidKey,
        MasterName nameSource
        )
    {
        RegistryKey itemKey(RegistryKey::Open(rootKey + subKey, KEY_ENUMERATE_SUB_KEYS));
        if (itemKey.Invalid())
        {
            return;
        }
        auto rawValues = itemKey.EnumerateSubKeys(KEY_QUERY_VALUE);
        std::vector<std::pair<std::wstring, std::wstring>> values;
        values.reserve(rawValues.size());
        for (auto const& entry : rawValues)
        {
            std::wstring name;
            try
            {
                name = entry[L""].GetString();
            }
            catch (ErrorFileNotFoundException const&)
            {
                //Expected
            }
            std::wstring clsid(entry.GetName());
            clsid.erase(clsid.begin(), std::find(clsid.rbegin(), clsid.rend(), L'\\').base());
            values.emplace_back(std::move(clsid), std::move(name));
        }

        for (auto& currentEntry : values)
        {
            //First try the user specific CLSID key.
            RegistryKey clsidKey(RegistryKey::Open(rootKey + clsidKey + currentEntry.first, KEY_QUERY_VALUE));
            if (clsidKey.Invalid())
            {
                //Next try the machine CLSID key.
                clsidKey = RegistryKey::Open(backupClsidKey + currentEntry.first, KEY_QUERY_VALUE);
            }
            RegistryKey inProcKey;
            if (clsidKey.Valid())
            {
                //Open the InProcServer32 subkey of that.
                inProcKey = RegistryKey::Open(clsidKey, L"InProcServer32", KEY_QUERY_VALUE);
            }
            //The CLSID key's name is used if the remote name is the "boss", or if the current try
            //is empty
            if (clsidKey.Valid() && (nameSource == CLASS_ROOT_DEFAULT || currentEntry.second.empty()))
            {
                std::wstring remoteName(clsidKey[L""].GetString());
                //Don't clobber the existing name in the event the actual name in the key is empty.
                if (!remoteName.empty())
                {
                    currentEntry.second = std::move(remoteName);
                }
            }
            std::wstring file;
            if (inProcKey.Valid())
            {
                file = inProcKey[L""].GetString();
            }
            if (currentEntry.second.empty())
            {
                currentEntry.second = L"N/A";
            }
            GeneralEscape(currentEntry.first, L'#', L'=');
            GeneralEscape(currentEntry.second, L'#', L':');
            output << prefix << L": " << currentEntry.second << L": " << currentEntry.first << L'=';
            WriteDefaultFileOutput(output, file);
            output << L'\n';
        }
    }

    /**
    * CLSID value based output.
    *
    * @param [in,out] output The stream to write the output to.
    * @param prefix          The prefix used to identify the type of line generated in the report.
    * @param rootKey         The root key where the check is rooted. The CLASSES key should be
    *                        located in \\Software\\Classes relative to this key.
    * @param subKey          The sub key under the root key\\Software (or \\Software\\Wow6432Node
    *                        on x64 machines) where the CLSID values are located.
    * @param source          Where the cLSIDs themselves are located (the key or the value).
    * @param nameSource      Which name is primary; the name stored with the CLSID, or the name
    *                        stored with the CLASSES key.
    */
    static void ClsidSubkeyBasedOutput(
        std::wostream& output,
        std::wstring const& prefix,
        std::wstring const& rootKey,
        std::wstring const& subKey,
        MasterName nameSource
        )
    {
#ifdef _M_X64
        ClsidSubkeyBasedOutputWithBits(
            output,
            prefix,
            rootKey,
            L"\\Software\\Wow6432Node" + subKey,
            L"\\Software\\Wow6432Node\\Classes\\CLSID\\",
            L"\\Registry\\Machine\\Software\\Wow6432Node\\Classes\\CLSID\\",
            nameSource
            );
#endif
        ClsidSubkeyBasedOutputWithBits(
            output,
            prefix + Get64Suffix(),
            rootKey,
            L"\\Software" + subKey,
            L"\\Software\\Classes\\CLSID\\",
            L"\\Registry\\Machine\\Software\\Classes\\CLSID\\",
            nameSource
            );
    }

    /**
    * Single registry value output.
    *
    * @param [in,out] output The output stream.
    * @param key             The key where the output is rooted.
    * @param valueName       Name of the value.
    * @param prefix          The prefix to use in the log output.
    * @param dataProcess     (optional) [in,out] The process by which the line data is written.
    *                        The default escapes using the general escaping format.
    */
    static void SingleRegistryValueOutput(
        std::wostream& output,
        RegistryKey const& key,
        std::wstring const& valueName,
        std::wstring const& prefix,
        std::function<void (std::wostream& out, std::wstring& source)> dataProcess = GeneralProcess
        )
    {
        if (key.Invalid())
        {
            return;
        }
        try
        {
            auto value = key[valueName];
            output << prefix << L": ";
            std::wstring val(value.GetString());
            dataProcess(output, val);
            output << L'\n';
        }
        catch (ErrorFileNotFoundException const&)
        {
            //Expected error.
        }
    }

    /**
    * Internet explorer output.
    *
    * @param [in,out] output The output stream.
    * @param rootKey         The root key where Internet Explorer is being rooted.
    */
    static void InternetExplorerMainOutput(std::wostream& output, std::wstring const& rootKey)
    {
        RegistryKey ieRoot(RegistryKey::Open(rootKey + L"\\Software\\Microsoft\\Internet Explorer", KEY_QUERY_VALUE));
        RegistryKey ieMain(RegistryKey::Open(ieRoot, L"Main", KEY_QUERY_VALUE));
        RegistryKey ieSearch(RegistryKey::Open(ieRoot, L"Search", KEY_QUERY_VALUE));
        std::wstring suffix64(Get64Suffix());
#ifdef _M_X64
        RegistryKey ieRoot32(RegistryKey::Open(rootKey + L"\\Software\\Wow6432Node\\Microsoft\\Internet Explorer", KEY_QUERY_VALUE));
        RegistryKey ieMain32(RegistryKey::Open(ieRoot, L"Main", KEY_QUERY_VALUE));
        RegistryKey ieSearch32(RegistryKey::Open(ieRoot, L"Main", KEY_QUERY_VALUE));
#endif
        SingleRegistryValueOutput(output, ieMain, L"Default_Page_Url", L"DefaultPageUrl" + suffix64, HttpProcess);
#ifdef _M_X64
        SingleRegistryValueOutput(output, ieMain32, L"Default_Page_Url", L"DefaultPageUrl", HttpProcess);
#endif
        SingleRegistryValueOutput(output, ieMain, L"Default_Search_Url", L"DefaultSearchUrl" + suffix64, HttpProcess);
#ifdef _M_X64
        SingleRegistryValueOutput(output, ieMain32, L"Default_Search_Url", L"DefaultSearchUrl", HttpProcess);
#endif
        SingleRegistryValueOutput(output, ieMain, L"Local Page", L"LocalPage" + suffix64, HttpProcess);
#ifdef _M_X64
        SingleRegistryValueOutput(output, ieMain32, L"Local Page", L"LocalPage", HttpProcess);
#endif
        SingleRegistryValueOutput(output, ieMain, L"Start Page", L"StartPage" + suffix64, HttpProcess);
#ifdef _M_X64
        SingleRegistryValueOutput(output, ieMain32, L"Start Page", L"StartPage", HttpProcess);
#endif
        SingleRegistryValueOutput(output, ieMain, L"Search Page", L"SearchPage" + suffix64, HttpProcess);
#ifdef _M_X64
        SingleRegistryValueOutput(output, ieMain32, L"Search Page", L"SearchPage", HttpProcess);
#endif
        SingleRegistryValueOutput(output, ieMain, L"Search Bar", L"SearchBar" + suffix64, HttpProcess);
#ifdef _M_X64
        SingleRegistryValueOutput(output, ieMain32, L"Search Bar", L"SearchBar", HttpProcess);
#endif
        SingleRegistryValueOutput(output, ieMain, L"SearchMigratedDefaultUrl", L"SearchMigratedDefaultUrl" + suffix64, HttpProcess);
#ifdef _M_X64
        SingleRegistryValueOutput(output, ieMain32, L"SearchMigratedDefaultUrl", L"SearchMigratedDefaultUrl", HttpProcess);
#endif
        SingleRegistryValueOutput(output, ieMain, L"Security Risk Page", L"SecurityPage" + suffix64, HttpProcess);
#ifdef _M_X64
        SingleRegistryValueOutput(output, ieMain32, L"Security Risk Page", L"SecurityPage", HttpProcess);
#endif
        SingleRegistryValueOutput(output, ieMain, L"Window Title", L"WindowTitle" + suffix64, HttpProcess);
#ifdef _M_X64
        SingleRegistryValueOutput(output, ieMain32, L"Window Title", L"WindowTitle", HttpProcess);
#endif
        SingleRegistryValueOutput(output, ieMain, L"SearchURL", L"SearchUrl" + suffix64, HttpProcess);
#ifdef _M_X64
        SingleRegistryValueOutput(output, ieMain32, L"SearchURL", L"SearchUrl", HttpProcess);
#endif
        SingleRegistryValueOutput(output, ieSearch, L"SearchAssistant", L"SearchAssistant" + suffix64, HttpProcess);
#ifdef _M_X64
        SingleRegistryValueOutput(output, ieSearch32, L"SearchAssistant", L"SearchAssistant", HttpProcess);
#endif
        SingleRegistryValueOutput(output, ieSearch, L"CustomizeSearch", L"CustomizeSearch" + suffix64, HttpProcess);
#ifdef _M_X64
        SingleRegistryValueOutput(output, ieSearch32, L"CustomizeSearch", L"CustomizeSearch", HttpProcess);
#endif
    }

    /**
    * Process a single IE script entry.
    *
    * @param [in,out] out The output stream where the output is written.
    * @param suffix       The suffix applied to the script entry (either "64" or nothing)
    * @param valueName    Name of the value in question.
    * @param subKey       The sub key to which the query of script is being posed.
    */
    static void ProcessIeScript( std::wostream& out, std::wstring const& suffix, std::wstring const& valueName, RegistryKey const& subKey ) 
    {
        try
        {
            std::wstring name(subKey.GetName());
            std::wstring command(subKey[valueName].GetString());
            GeneralEscape(name, L'#', L']');
            GeneralEscape(command);
            out << L"IeScript" << suffix << L": [" << std::move(name) << L"] " << std::move(command) << L'\n';
        }
        catch (ErrorFileNotFoundException const&)
        {
            //Expected
        }
    }

    /**
    * Process a single IE COM entry.
    *
    * @param [in,out] out The output stream where the output is written.
    * @param suffix       The suffix applied to the script entry (either "64" or nothing)
    * @param valueName    Name of the value in question.
    * @param subKey       The sub key to which the query of script is being posed.
    */
    static void ProcessIeCom( std::wostream& out, std::wstring const& suffix, std::wstring const& valueName, RegistryKey const& subKey, std::wstring const& hiveRootPath, std::wstring const& software )
    {
        try
        {
            std::wstring name(subKey.GetName());
            std::wstring clsid(subKey[valueName].GetString());
            std::wstring file(L"N/A");
            RegistryKey clsidKey(RegistryKey::Open(hiveRootPath + L"\\" + software + L"\\Classes\\CLSID\\" + clsid + L"\\InProcServer32", KEY_QUERY_VALUE));
            if (clsidKey.Invalid())
            {
                clsidKey = RegistryKey::Open(L"\\Registry\\Machine\\" + software + L"\\Classes\\CLSID\\" + clsid + L"\\InProcServer32", KEY_QUERY_VALUE);
            }
            if (clsidKey.Valid())
            {
                std::wstring fileTry(clsidKey[L""].GetString());
                if (!fileTry.empty())
                {
                    file = std::move(fileTry);
                }
            }

            name.erase(name.cbegin(), std::find(name.crbegin(), name.crend(), L'\\').base());
            GeneralEscape(name, L'#', L'-');
            GeneralEscape(clsid, L'#', L']');
            out << L"IeCom" << suffix << L": [" << name << L"->" << clsid << L"] ";
            WriteDefaultFileOutput(out, file);
            out << L'\n';
        }
        catch (ErrorFileNotFoundException const&)
        {
            //Expected
        }
    }

    static void TrustedZoneRecursive(std::wostream& out, std::wstring const& rootKey, std::wstring const& domainToEnter, std::wstring const& prefix)
    {
        RegistryKey currentDomainKey = RegistryKey::Open(rootKey, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE);

        for (auto const& value : currentDomainKey.EnumerateValues())
        {
            if (value.GetType() != REG_DWORD)
            {
                continue;
            }

            DWORD domainClass = value.GetDWordStrict();
            if (domainClass != 2) // Trusted zone marker
            {
                continue;
            }

            std::wstring outputUrl(value.GetName());
            outputUrl += L"://";
            outputUrl += domainToEnter;
            HttpEscape(outputUrl);

            out << prefix << L": " << outputUrl << L"\n";
        }

        auto const& subKeys = currentDomainKey.EnumerateSubKeyNames();
        currentDomainKey.Close();

        for (auto const& key : subKeys)
        {
            std::wstring subDomain(key);
            subDomain += L'.';
            subDomain += domainToEnter;

            TrustedZoneRecursive(out, rootKey + L"\\" + key, subDomain, prefix);
        }
    }

    static void TrustedZone(std::wostream& out, std::wstring const& keyPath, std::wstring const& prefix)
    {
        RegistryKey domainsRoot = RegistryKey::Open(keyPath, KEY_ENUMERATE_SUB_KEYS);
        if (domainsRoot.Invalid())
        {
            return;
        }

        auto rootDomains = domainsRoot.EnumerateSubKeyNames();
        for (auto const& rootDomain : rootDomains)
        {
            TrustedZoneRecursive(out, keyPath + L"\\" + rootDomain, rootDomain, prefix);
        }
    }

    static void TrustedDefaults(std::wostream& out, std::wstring const& keyPath, std::wstring const& prefix)
    {
        RegistryKey targetKey(RegistryKey::Open(keyPath, KEY_QUERY_VALUE));
        if (targetKey.Invalid())
        {
            return;
        }

        for (auto const& value : targetKey.EnumerateValues())
        {
            if (value.GetType() != REG_DWORD)
            {
                continue;
            }

            if (value.GetDWordStrict() != 2)
            {
                continue;
            }

            out << prefix << L": " << value.GetName() << L"\n";
        }
    }

    static void TrustedIpRange(std::wostream& out, std::wstring const& keyPath, std::wstring const& prefix)
    {
        RegistryKey targetKey(RegistryKey::Open(keyPath, KEY_ENUMERATE_SUB_KEYS));
        if (targetKey.Invalid())
        {
            return;
        }

        for (auto const& subKey : targetKey.EnumerateSubKeyNames())
        {
            RegistryKey subHKey(RegistryKey::Open(keyPath + L"\\" + subKey, KEY_QUERY_VALUE));
            std::wstring ipRange = subHKey[L":Range"].GetStringStrict();
            GeneralEscape(ipRange);

            for (auto const& value : subHKey.EnumerateValues())
            {
                if (value.GetType() != REG_DWORD)
                {
                    continue;
                }

                std::wstring name(subKey);
                GeneralEscape(name, L'#', L']');

                std::wstring range(value.GetName());
                range += L"://";
                range += ipRange;
                HttpEscape(range);
                out << prefix << L": [" << name << L"] " << range << L"\n";
            }
        }
    }

    /**
    * Explorer extensions output.
    *
    * @param [in,out] out The output stream.
    * @param rootKey      The root key where the IE settings are rooted. (user or machine hive
    *                     root)
    */
    static void ExplorerExtensionsOutput(std::wostream& out, std::wstring const& rootKey)
    {
        std::wstring suffix(Get64Suffix());
        std::wstring software(L"Software");
        auto keyProcessor = [&](RegistryKey const& subKey) {
            ProcessIeScript(out, suffix, L"Exec", subKey);
            ProcessIeScript(out, suffix, L"Script", subKey);
            ProcessIeCom(out, suffix, L"clsidextension", subKey, rootKey, software);
            ProcessIeCom(out, suffix, L"bandclsid", subKey, rootKey, software);
        };
        RegistryKey key(RegistryKey::Open(rootKey+L"\\Software\\Microsoft\\Internet Explorer\\Extensions", KEY_ENUMERATE_SUB_KEYS));
        if (key.Valid())
        {
            auto subkeys = key.EnumerateSubKeys(KEY_QUERY_VALUE);
            std::for_each(subkeys.cbegin(), subkeys.cend(), keyProcessor);
        }
#ifdef _M_X64
        key = RegistryKey::Open(rootKey+L"\\Software\\Wow6432Node\\Microsoft\\Internet Explorer\\Extensions", KEY_ENUMERATE_SUB_KEYS);
        suffix = std::wstring();
        software = L"Software\\Wow6432Node";
        if (key.Valid())
        {
            auto subkeys = key.EnumerateSubKeys(KEY_QUERY_VALUE);
            std::for_each(subkeys.cbegin(), subkeys.cend(), keyProcessor);
        }
#endif
        TrustedZone(out, rootKey+L"\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\Domains", L"Trusted Zone" + Get64Suffix());
#ifdef _M_X64
        TrustedZone(out, rootKey+L"\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\Domains", L"Trusted Zone");
#endif
        TrustedZone(out, rootKey+L"\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\EscDomains", L"ESC Trusted Zone" + Get64Suffix());
#ifdef _M_X64
        TrustedZone(out, rootKey+L"\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\EscDomains", L"ESC Trusted Zone");
#endif
        TrustedDefaults(out, rootKey+L"\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\ProtocolDefaults", L"Trusted Default Protocol" + Get64Suffix());
#ifdef _M_X64
        TrustedDefaults(out, rootKey+L"\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\ProtocolDefaults", L"Trusted Default Protocol");
#endif
        TrustedIpRange(out, rootKey+L"\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\Ranges", L"Trusted IP Range" + Get64Suffix());
#ifdef _M_X64
        TrustedIpRange(out, rootKey+L"\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\Ranges", L"Trusted IP Range");
#endif
    }

    /**
    * Common HJT outputs. Does all the things that are repeated for the entire machine, and for
    * each user's registry.
    *
    * @param [in,out] output The output stream to which the log is written.
    * @param rootKey         The root key to check. (e.g. \\Registry\\Machine or \\Registry\\User\\
    *                        ${Sid}
    */
    static void CommonHjt(std::wostream& output, std::wstring const& rootKey)
    {
        InternetExplorerMainOutput(output, rootKey);
        ClsidValueBasedOutput(output, L"UrlSearchHook", rootKey, L"\\Microsoft\\Internet Explorer\\URLSearchHooks", NAME, CLASS_ROOT_DEFAULT);
        RegistryKey winlogon(RegistryKey::Open(rootKey + L"\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", KEY_QUERY_VALUE));
        SingleRegistryValueOutput(output, winlogon, L"Shell", L"Shell", FileProcess);
        SingleRegistryValueOutput(output, winlogon, L"Userinit", L"Userinit", [](std::wostream& out, std::wstring &src) {
            if (src.back() == L',')
            {
                src.pop_back();
            }
            FileProcess(out, src);
        });
        SingleRegistryValueOutput(output, winlogon, L"UIHost", L"UIHost", FileProcess);
        SingleRegistryValueOutput(output, winlogon, L"TaskMan", L"TaskMan", FileProcess);
        if (winlogon.Valid())
        {
            try
            {
                DWORD sfc = winlogon[L"SFCDisable"].GetDWord();
                if (sfc)
                {
                    output << L"SFC: Disabled\n";
                }
                else
                {
                    output << L"SFC: Enabled\n";
                }
            }
            catch (ErrorFileNotFoundException const&)
            {
                //Expected
            }
        }
        ClsidSubkeyBasedOutput(output, L"BHO", rootKey, L"\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects", CLASS_ROOT_DEFAULT);
        ClsidValueBasedOutput(output, L"TB", rootKey, L"\\Microsoft\\Internet Explorer\\Toolbar", NAME, CLASS_ROOT_DEFAULT);
        ClsidValueBasedOutput(output, L"TB", rootKey, L"\\Microsoft\\Internet Explorer\\Toolbar\\WebBrowser", NAME, CLASS_ROOT_DEFAULT);
        ClsidValueBasedOutput(output, L"EB", rootKey, L"\\Microsoft\\Internet Explorer\\Explorer Bars", NAME, CLASS_ROOT_DEFAULT);
        RunKeyOutput(output, rootKey, L"Run");
        RunKeyOutput(output, rootKey, L"RunOnce");
        RunKeyOutput(output, rootKey, L"RunServices");
        RunKeyOutput(output, rootKey, L"RunServicesOnce");
        ExplorerRunOutput(output, rootKey);
        ValueMajorBasedEnumerationBitless(output, rootKey,
            L"\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
            L"\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
            L"PoliciesExplorer",
            GeneralProcess
            );
        ValueMajorBasedEnumerationBitless(output, rootKey,
            L"\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
            L"\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
            L"PoliciesSystem",
            GeneralProcess
            );
        ValueMajorBasedEnumerationBitless(output, rootKey,
            L"\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\DisallowRun",
            L"\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\DisallowRun",
            L"PoliciesDisallowRun",
            GeneralProcess
            );
        SubkeyMajorBasedEnumerationBitless(output, rootKey,
            L"\\Software\\Microsoft\\Internet Explorer\\MenuExt",
            L"\\Software\\Wow6432Node\\Microsoft\\Internet Explorer\\MenuExt",
            L"",
            L"IeMenu",
            GeneralProcess
            );
        ExplorerExtensionsOutput(output, rootKey);
    }

    /**
    * Looks up a given SID to find its associated account name, and returns a DOMAIN\User form
    * string matching that user's name.
    *
    * @param stringSid The SID of the user in string format.
    *
    * @return The user's name and domain in DOMAIN\USER format.
    */
    static std::wstring LookupAccountNameBySid(std::wstring const& stringSid)
    {
        if (stringSid == L".DEFAULT")
        {
            return L"Default User";
        }
        PSID sidPtr = nullptr;
        ScopeExit killSidPtr([sidPtr]() { if (sidPtr != nullptr) { ::LocalFree(sidPtr); } });
        if (::ConvertStringSidToSidW(stringSid.c_str(), &sidPtr) == 0)
        {
            Win32Exception::ThrowFromLastError();
        }
        SID_NAME_USE use;
        std::wstring userName, domainName;
        userName.resize(128);
        DWORD userNameCount = static_cast<DWORD>(userName.size());
        domainName.resize(128);
        DWORD domainNameCount = static_cast<DWORD>(domainName.size());
        if (::LookupAccountSidW(nullptr, sidPtr, &userName[0], &userNameCount, &domainName[0], &domainNameCount, &use) == 0)
        {
            if (userName.size() != static_cast<std::size_t>(userNameCount) || domainName.size() != static_cast<std::size_t>(domainNameCount))
            {
                userName.resize(userNameCount);
                domainName.resize(domainNameCount);
                if (::LookupAccountSidW(nullptr, sidPtr, &userName[0], &userNameCount, &domainName[0], &domainNameCount, &use) == 0)
                {
                    Win32Exception::ThrowFromLastError();
                }
            }
            else
            {
                Win32Exception::ThrowFromLastError();
            }
        }
        userName.resize(userNameCount);
        domainName.resize(domainNameCount);
        domainName.push_back(L'\\');
        return domainName + userName;
    }

    //static void SpoofedDnsCheck(std::wostream& output, std::wstring queryHostname, std::wstring expectedHostname)
    //{
    //    std::wstring responseIpAddress(IpAddressFromHostname(queryHostname));
    //    std::wstring responseHostname(HostnameFromIpAddress(responseIpAddress, true));

    //    if (boost::iends_with(responseHostname, expectedHostname) == false)
    //    {
    //        HttpEscape(queryHostname);
    //        HttpEscape(responseHostname);

    //        output << L"SpoofedDNS: " << queryHostname << L" -> ";
    //        if (responseHostname.empty())
    //        {
    //            output << L"not available";
    //        }
    //        else
    //        {
    //            output << responseHostname;
    //        }
    //        output << L" (" << responseIpAddress << L")\n";
    //    }
    //}

    static void ExecuteLspChain(std::wostream& output, RegistryKey &protocolCatalogKey, std::wstring suffix)
    {
        DWORD catalogEntries = protocolCatalogKey[L"Num_Catalog_Entries" + suffix].GetDWord();
        bool chainOk = true;
        std::set<std::wstring> catalogFiles;
        RegistryKey catalogEntriesKey(RegistryKey::Open(protocolCatalogKey, L"Catalog_Entries" + suffix, KEY_ENUMERATE_SUB_KEYS));
        auto catalogEntryNames = catalogEntriesKey.EnumerateSubKeyNames();
        std::sort(catalogEntryNames.begin(), catalogEntryNames.end());
        if (catalogEntries != catalogEntryNames.size())
        {
            chainOk = false;
        }

        for (std::size_t idx = 0; idx < catalogEntryNames.size(); ++idx)
        {
            std::wstring const& actualName = catalogEntryNames[idx];
            wchar_t expectedName[13];
            swprintf_s(expectedName, L"%012u", idx + 1);
            if (actualName != expectedName)
            {
                chainOk = false;
            }

            RegistryKey catalogEntryKey(RegistryKey::Open(catalogEntriesKey, actualName, KEY_QUERY_VALUE));
            RegistryValue packedCatalogItem(catalogEntryKey[L"PackedCatalogItem"]);
            auto const end = std::find(packedCatalogItem.cbegin(), packedCatalogItem.cend(), '\0');
            catalogFiles.emplace(packedCatalogItem.cbegin(), end);
        }

        for (std::wstring catalogFile : catalogFiles)
        {
            GeneralEscape(catalogFile);
            output << L"LSP" << suffix << L": " << catalogFile << L"\n";
        }

        if (!chainOk)
        {
            output << L"LSP" << suffix << L": Chain Broken\n";
        }
    }

    static void ExecuteNspChain(std::wostream& output, RegistryKey &namespaceCatalogKey, std::wstring suffix)
    {
        DWORD catalogEntries = namespaceCatalogKey[L"Num_Catalog_Entries" + suffix].GetDWord();
        bool chainOk = true;
        std::set<std::wstring> catalogFiles;
        RegistryKey catalogEntriesKey(RegistryKey::Open(namespaceCatalogKey, L"Catalog_Entries" + suffix, KEY_ENUMERATE_SUB_KEYS));
        auto catalogEntryNames = catalogEntriesKey.EnumerateSubKeyNames();
        std::sort(catalogEntryNames.begin(), catalogEntryNames.end());
        if (catalogEntries != catalogEntryNames.size())
        {
            chainOk = false;
        }

        for (std::size_t idx = 0; idx < catalogEntryNames.size(); ++idx)
        {
            std::wstring const& actualName = catalogEntryNames[idx];
            wchar_t expectedName[13];
            swprintf_s(expectedName, L"%012u", idx + 1);
            if (actualName != expectedName)
            {
                chainOk = false;
            }

            RegistryKey catalogEntryKey(RegistryKey::Open(catalogEntriesKey, actualName, KEY_QUERY_VALUE));
            catalogFiles.emplace(catalogEntryKey[L"LibraryPath"].GetString());
        }

        for (std::wstring catalogFile : catalogFiles)
        {
            GeneralEscape(catalogFile);
            output << L"NSP" << suffix << L": " << catalogFile << L"\n";
        }

        if (!chainOk)
        {
            output << L"NSP" << suffix << L": Chain Broken\n";
        }
    }

    void PseudoHjt::Execute(
        std::wostream& output,
        ScriptSection const&,
        std::vector<std::wstring> const&
        ) const
    {
        auto hives = EnumerateUserHives();
        SecurityCenterOutput(output);
        CommonHjt(output, L"\\Registry\\Machine");

        RegistryKey dpfRoot(RegistryKey::Open(L"\\Registry\\Machine\\Software\\Microsoft\\Code Store Database\\Distribution Units", KEY_ENUMERATE_SUB_KEYS));
        if (dpfRoot.Valid())
        {
            for (std::wstring const& clsid : dpfRoot.EnumerateSubKeyNames())
            {
                std::wstring clsidEscaped(clsid);
                GeneralEscape(clsidEscaped);
                output << L"DPF" << Get64Suffix() << L": " << clsidEscaped << L"\n";
            }
        }
#ifdef _M_X64
        dpfRoot = RegistryKey::Open(L"\\Registry\\Machine\\Software\\Wow6432Node\\Microsoft\\Code Store Database\\Distribution Units", KEY_ENUMERATE_SUB_KEYS);
        if (dpfRoot.Valid())
        {
            for (std::wstring const& clsid : dpfRoot.EnumerateSubKeyNames())
            {
                std::wstring clsidEscaped(clsid);
                GeneralEscape(clsidEscaped);
                output << L"DPF: " << clsidEscaped << L"\n";
            }
        }
#endif
        dpfRoot.Close();

        RegistryKey winsockParameters(RegistryKey::Open(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Winsock2\\Parameters", KEY_QUERY_VALUE));
        
        std::wstring protocolCatalog(winsockParameters[L"Current_Protocol_Catalog"].GetStringStrict());
        RegistryKey protocolCatalogKey(RegistryKey::Open(winsockParameters, protocolCatalog, KEY_QUERY_VALUE));
        ExecuteLspChain(output, protocolCatalogKey, L"");
        ExecuteLspChain(output, protocolCatalogKey, L"64");
        protocolCatalogKey.Close();

        std::wstring namespaceCatalog(winsockParameters[L"Current_NameSpace_Catalog"].GetStringStrict());
        RegistryKey namespaceCatalogKey(RegistryKey::Open(winsockParameters, namespaceCatalog, KEY_QUERY_VALUE));
        ExecuteNspChain(output, namespaceCatalogKey, L"");
        ExecuteNspChain(output, namespaceCatalogKey, L"64");
        namespaceCatalogKey.Close();
        winsockParameters.Close();

        for (std::wstring const& hive : hives)
        {
            std::wstring head(L"User Settings");
            Header(head);
            std::wstring sid(std::find(hive.crbegin(), hive.crend(), L'\\').base(), hive.end());
            std::wstring user(LookupAccountNameBySid(sid));
            GeneralEscape(user, L'#', L']');
            output << L'\n' << head << L"\n\nIdentity: [" << user << L"] " << sid << L'\n';
            CommonHjt(output, hive);
        }
    }

}
