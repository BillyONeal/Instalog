// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <functional>
#include <algorithm>
#include <vector>
#include <set>
#include <string>
#include <regex>
#include <iterator>
#include <Sddl.h>
#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <Objbase.h>
#include <ObjIdl.h>
#include <shobjidl.h>
#include "Com.hpp"
#include "SecurityCenter.hpp"
#include "StockOutputFormats.hpp"
#include "Registry.hpp"
#include "LoadPointsReport.hpp"
#include "Path.hpp"
#include "ScopeExit.hpp"
#include "Dns.hpp"
#include "Utf8.hpp"

namespace Instalog
{
using namespace SystemFacades;

std::string LoadPointsReport::GetScriptCommand() const
{
    return "loadpoints";
}

std::string LoadPointsReport::GetName() const
{
    return "Load Points";
}

LogSectionPriorities LoadPointsReport::GetPriority() const
{
    return SCANNING;
}

static std::string Get64Suffix()
{
#ifdef _M_X64
    return "64";
#else
    return std::string();
#endif
}

/**
* Security center output for the Loading Points section.
*
* @param [in,out] output The stream to receive the output.
*/
static void SecurityCenterOutput(log_sink& output)
{
    using SystemFacades::SecurityProduct;
    auto products = SystemFacades::EnumerateSecurityProducts();
    for (auto it = products.cbegin(); it != products.cend(); ++it)
    {
        write(output, it->GetTwoLetterPrefix(), ": [", it->GetInstanceGuid(), "] ", it->IsEnabled() ? 'E' : 'D');
        switch (it->GetUpdateStatus())
        {
        case SecurityProduct::OutOfDate:
            write(output, 'O');
            break;
        case SecurityProduct::UpToDate:
            write(output, 'U');
            break;
        }
        writeln(output, ' ', it->GetName());
    }
}

/**
* A processing function which writes the target string in the default escaped
*function,
* assuming that the newline is the ending delimiter.
*
* @param [in,out] out    The output stream.
* @param [in,out] target Target which should be written.
*/
static void GeneralProcess(log_sink& out, std::string& target)
{
    GeneralEscape(target, '#');
    write(out, target);
}

/**
* A processing function which writes the default file output.
*
* @param [in,out] out    The output stream.
* @param [in,out] target Target name of the file to print.
*/
static void FileProcess(log_sink& out, std::string& target)
{
    WriteDefaultFileOutput(out, target);
}

/**
* A processing function which writes the target as a URL.
*
* @param [in,out] out    The output stream.
* @param [in,out] target Target to write as a URL.
*/
static void HttpProcess(log_sink& out, std::string& target)
{
    HttpEscape(target, '#');
    write(out, target);
}

/**
* Value major based enumeration; general method to enumerate a registry key,
*where a single
* log line is generated per value.
*
* @param [in,out] output The output stream.
* @param root            The root registry key from where the key is
*enumerated.
* @param prefix          The prefix which identifies the log line in the
*report.
* @param dataProcess     (optional) The processing function which generates the
*report of the
*                        values' data. The default writes the data as a file.
*/
static void ValueMajorBasedEnumeration(
    log_sink& output,
    std::string const& root,
    std::string const& prefix,
    std::function<void(log_sink& out, std::string& source)> dataProcess =
        FileProcess)
{
    RegistryKey key(RegistryKey::Open(root, KEY_QUERY_VALUE));
    if (key.Invalid())
    {
        return;
    }
    auto values = key.EnumerateValues();
    std::vector<std::pair<std::string, std::string>> pods;
    for (RegistryValueAndData const& val : values)
    {
        pods.emplace_back(std::make_pair(val.GetName(), val.GetString()));
    }
    ;

    std::sort(pods.begin(), pods.end());
    for (auto& current : pods)
    {
        GeneralEscape(current.first, '#', ']');
        write(output, prefix, ": [", current.first, "] ");
        dataProcess(output, current.second);
        writeln(output);
    }
    ;
}

#pragma warning(push)
#pragma warning(disable : 4100) // Unreferenced formal parameter. (Becomes                                       \
                                // unreferenced in 32 bit mode)
/**
* Value major based enumeration, automatically specialized for 32 bit and 64 bit
*machines.
*
* @param [in,out] output The output stream where output will be written.
* @param root            The root key of the hive being searched.
* @param subkey32        The subkey from the root to use on 64 bit machines when
*accessing the
*                        32 bit registry view.
* @param subkey64        The subkey from the root to use on 64 bit machines when
*accessing the
*                        64 bit registry view. (Also the only registry view on
*32 bit
*                        machines)
* @param prefix          The prefix applied to the log lines.
* @param dataProcess     (optional) [in,out] The process applied to the data
*before it is
*                        printed.
*/
static void ValueMajorBasedEnumerationBitless(
    log_sink& output,
    std::string const& root,
    std::string const& subkey32,
    std::string const& subkey64,
    std::string const& prefix,
    std::function<void(log_sink& out, std::string& source)> dataProcess =
        FileProcess)
{
#ifdef _M_X64
    ValueMajorBasedEnumeration(
        output, root + subkey64, prefix + "64", dataProcess);
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
void RunKeyOutput(log_sink& output,
                  std::string const& runRoot,
                  std::string const& name)
{
    ValueMajorBasedEnumerationBitless(
        output,
        runRoot,
        "\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\" + name,
        "\\Software\\Microsoft\\Windows\\CurrentVersion\\" + name,
        name);
}

// TODO: Make the common bits of these and ValueMajorXXX go away.

/**
* Subkey major based enumeration; general method to enumerate a registry key,
*where a
* single log line is generated per subkey.
*
* @param [in,out] output The output stream.
* @param root            The root registry key from where the key is
*enumerated.
* @param prefix          The prefix which identifies the log line in the
*report.
* @param valueName       Name of the value in each subkey at which to look.
* @param dataProcess     (optional) The processing function which generates the
*report of the
*                        values' data. The default writes the data as a file.
*/
static void SubkeyMajorBasedEnumeration(
    log_sink& output,
    std::string const& root,
    std::string const& valueName,
    std::string const& prefix,
    std::function<void(log_sink& out, std::string& source)> dataProcess =
        FileProcess)
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
    std::vector<std::pair<std::string, std::string>> pods;
    for (RegistryKey const& val : values)
    {
        try
        {
            pods.emplace_back(val.GetLocalName(), val[valueName].GetString());
        }
        catch (ErrorFileNotFoundException const&)
        {
            // Expected
        }
    }
    ;
    std::sort(pods.begin(), pods.end());
    for (auto& current : pods)
    {
        GeneralEscape(current.first, '#', ']');
        write(output, prefix, ": [", current.first, "] ");
        dataProcess(output, current.second);
        writeln(output);
    }
    ;
}

#pragma warning(push)
#pragma warning(disable : 4100) // Unreferenced formal parameter. (Becomes                                       \
                                // unreferenced in 32 bit mode)
/**
* Subkey major based enumeration, automatically specialized for 32 bit and 64
*bit machines.
*
* @param [in,out] output The output stream where output will be written.
* @param root            The root key of the hive being searched.
* @param subkey32        The subkey from the root to use on 64 bit machines when
*accessing the
*                        32 bit registry view.
* @param subkey64        The subkey from the root to use on 64 bit machines when
*accessing the
*                        64 bit registry view. (Also the only registry view on
*32 bit
*                        machines)
* @param prefix          The prefix applied to the log lines.
* @param valueName       Name of the value in each subkey at which to look.
* @param dataProcess     (optional) [in,out] The process applied to the data
*before it is
*                        printed.
*/
static void SubkeyMajorBasedEnumerationBitless(
    log_sink& output,
    std::string const& root,
    std::string const& subkey32,
    std::string const& subkey64,
    std::string const& valueName,
    std::string const& prefix,
    std::function<void(log_sink& out, std::string& source)> dataProcess =
        FileProcess)
{
#ifdef _M_X64
    SubkeyMajorBasedEnumeration(
        output, root + subkey64, valueName, prefix + "64", dataProcess);
    SubkeyMajorBasedEnumeration(
        output, root + subkey32, valueName, prefix, dataProcess);
#else
    SubkeyMajorBasedEnumeration(
        output, root + subkey64, valueName, prefix, dataProcess);
#endif
}
#pragma warning(pop)

/**
* Gets the user registry hive root paths.
*
* @return A list of the registry hives.
*/
static std::vector<std::string> EnumerateUserHives()
{
    std::vector<std::string> hives;
    {
        RegistryKey hiveList(RegistryKey::Open(
            "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\hivelist",
            KEY_QUERY_VALUE));
        if (hiveList.Invalid())
        {
            Win32Exception::ThrowFromNtError(::GetLastError());
        }
        hives = hiveList.EnumerateValueNames();
    } // Block closes key
    hives.erase(
        std::remove_if(
            hives.begin(), hives.end(), [](std::string const & str)->bool {
                return !boost::algorithm::istarts_with(str,
                                                       "\\Registry\\User\\") ||
                       boost::algorithm::ends_with(str, "_Classes");
            }),
        hives.end());
    std::sort(hives.begin(), hives.end());
    return hives;
}

/**
* Explorer run output.
*
* @param [in,out] output The output stream.
* @param rootKey         The root key where the ExplorerRun entries are
*located.
*/
static void ExplorerRunOutput(log_sink& output,
                              std::string const& rootKey)
{
#ifdef _M_X64
    ValueMajorBasedEnumeration(
        output,
        rootKey +
            "Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run",
        "ExplorerRun");
    ValueMajorBasedEnumeration(
        output,
        rootKey +
            "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run",
        "ExplorerRun64");
#else
    ValueMajorBasedEnumeration(
        output,
        rootKey +
            "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run",
        "ExplorerRun");
#endif
}

/**
* Values that represent possible sources of CLSIDs in ClsidValueBasedOutput
* queries.
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
* @param backupClsidKey  The backup clsid key (the machine cLSID key for the
*right bitness)
* @param source          Source for the CLSID (name or value).
* @param nameSource      Which name "wins" between the local name and the remote
*name.
*/
static void ClsidValueBasedOutputWithBits(log_sink& output,
                                          std::string const& prefix,
                                          std::string const& rootKey,
                                          std::string const& subKey,
                                          std::string const& clsidKey,
                                          std::string const& backupClsidKey,
                                          ClsidSource source,
                                          MasterName nameSource)
{
    RegistryKey itemKey(RegistryKey::Open(rootKey + subKey, KEY_QUERY_VALUE));
    if (itemKey.Invalid())
    {
        return;
    }
    auto rawValues = itemKey.EnumerateValues();
    std::vector<std::pair<std::string, std::string>> values;
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
        // First try the user specific CLSID key.
        RegistryKey clsidKey(RegistryKey::Open(
            rootKey + clsidKey + currentEntry.first, KEY_QUERY_VALUE));
        if (clsidKey.Invalid())
        {
            // Next try the machine CLSID key.
            clsidKey = RegistryKey::Open(backupClsidKey + currentEntry.first,
                                         KEY_QUERY_VALUE);
        }
        RegistryKey inProcKey;
        if (clsidKey.Valid())
        {
            // Open the InProcServer32 subkey of that.
            inProcKey =
                RegistryKey::Open(clsidKey, "InProcServer32", KEY_QUERY_VALUE);
        }
        // The CLSID key's name is used if the remote name is the "boss", or if
        // the current try
        // is empty
        if (clsidKey.Valid() &&
            (nameSource == CLASS_ROOT_DEFAULT || currentEntry.second.empty()))
        {
            std::string remoteName(clsidKey[""].GetString());
            // Don't clobber the existing name in the event the actual name in
            // the key is empty.
            if (!remoteName.empty())
            {
                currentEntry.second = std::move(remoteName);
            }
        }
        std::string file;
        if (inProcKey.Valid())
        {
            file = inProcKey[""].GetString();
        }
        if (currentEntry.second.empty())
        {
            currentEntry.second = "N/A";
        }
        GeneralEscape(currentEntry.first, '#', '=');
        GeneralEscape(currentEntry.second, '#', ':');
        write(output, prefix, ": ", currentEntry.second, ": ", currentEntry.first, '=');
        WriteDefaultFileOutput(output, file);
        writeln(output);
    }
}

/**
* CLSID value based output.
*
* @param [in,out] output The stream to write the output to.
* @param prefix          The prefix used to identify the type of line generated
*in the report.
* @param rootKey         The root key where the check is rooted. The CLASSES key
*should be
*                        located in \\Software\\Classes relative to this key.
* @param subKey          The sub key under the root key\\Software (or
*\\Software\\Wow6432Node
*                        on x64 machines) where the CLSID values are located.
* @param source          Where the cLSIDs themselves are located (the key or the
*value).
* @param nameSource      Which name is primary; the name stored with the CLSID,
*or the name
*                        stored with the CLASSES key.
*/
static void ClsidValueBasedOutput(log_sink& output,
                                  std::string const& prefix,
                                  std::string const& rootKey,
                                  std::string const& subKey,
                                  ClsidSource source,
                                  MasterName nameSource)
{
#ifdef _M_X64
    ClsidValueBasedOutputWithBits(
        output,
        prefix,
        rootKey,
        "\\Software\\Wow6432Node" + subKey,
        "\\Software\\Wow6432Node\\Classes\\CLSID\\",
        "\\Registry\\Machine\\Software\\Wow6432Node\\Classes\\CLSID\\",
        source,
        nameSource);
#endif
    ClsidValueBasedOutputWithBits(
        output,
        prefix + Get64Suffix(),
        rootKey,
        "\\Software" + subKey,
        "\\Software\\Classes\\CLSID\\",
        "\\Registry\\Machine\\Software\\Classes\\CLSID\\",
        source,
        nameSource);
}

// TODO: Make the common bits of these two go together.
/**
* Clsid subkey based output with the bitness things applied.
*
* @param [in,out] output The output stream.
* @param prefix          The prefix applied to each log line.
* @param rootKey         The root key.
* @param subKey          The sub key where the values are located.
* @param clsidKey        The clsid key for the current bitness.
* @param backupClsidKey  The backup clsid key (the machine cLSID key for the
*right bitness)
* @param source          Source for the CLSID (name or value).
* @param nameSource      Which name "wins" between the local name and the remote
*name.
*/
static void ClsidSubkeyBasedOutputWithBits(log_sink& output,
                                           std::string const& prefix,
                                           std::string const& rootKey,
                                           std::string const& subKey,
                                           std::string const& clsidKey,
                                           std::string const& backupClsidKey,
                                           MasterName nameSource)
{
    RegistryKey itemKey(
        RegistryKey::Open(rootKey + subKey, KEY_ENUMERATE_SUB_KEYS));
    if (itemKey.Invalid())
    {
        return;
    }
    auto rawValues = itemKey.EnumerateSubKeys(KEY_QUERY_VALUE);
    std::vector<std::pair<std::string, std::string>> values;
    values.reserve(rawValues.size());
    for (auto const& entry : rawValues)
    {
        std::string name;
        try
        {
            name = entry[""].GetString();
        }
        catch (ErrorFileNotFoundException const&)
        {
            // Expected
        }
        std::string clsid(entry.GetName());
        clsid.erase(clsid.begin(),
                    std::find(clsid.rbegin(), clsid.rend(), '\\').base());
        values.emplace_back(std::move(clsid), std::move(name));
    }

    for (auto& currentEntry : values)
    {
        // First try the user specific CLSID key.
        RegistryKey clsidKey(RegistryKey::Open(
            rootKey + clsidKey + currentEntry.first, KEY_QUERY_VALUE));
        if (clsidKey.Invalid())
        {
            // Next try the machine CLSID key.
            clsidKey = RegistryKey::Open(backupClsidKey + currentEntry.first,
                                         KEY_QUERY_VALUE);
        }
        RegistryKey inProcKey;
        if (clsidKey.Valid())
        {
            // Open the InProcServer32 subkey of that.
            inProcKey =
                RegistryKey::Open(clsidKey, "InProcServer32", KEY_QUERY_VALUE);
        }
        // The CLSID key's name is used if the remote name is the "boss", or if
        // the current try
        // is empty
        if (clsidKey.Valid() &&
            (nameSource == CLASS_ROOT_DEFAULT || currentEntry.second.empty()))
        {
            std::string remoteName(clsidKey[""].GetString());
            // Don't clobber the existing name in the event the actual name in
            // the key is empty.
            if (!remoteName.empty())
            {
                currentEntry.second = std::move(remoteName);
            }
        }
        std::string file;
        if (inProcKey.Valid())
        {
            file = inProcKey[""].GetString();
        }
        if (currentEntry.second.empty())
        {
            currentEntry.second = "N/A";
        }
        GeneralEscape(currentEntry.first, '#', '=');
        GeneralEscape(currentEntry.second, '#', ':');
        write(output, prefix, ": ", currentEntry.second, ": ", currentEntry.first, '=');
        WriteDefaultFileOutput(output, file);
        writeln(output);
    }
}

/**
* CLSID value based output.
*
* @param [in,out] output The stream to write the output to.
* @param prefix          The prefix used to identify the type of line generated
* in the report.
* @param rootKey         The root key where the check is rooted. The CLASSES key
* should be
*                        located in \\Software\\Classes relative to this key.
* @param subKey          The sub key under the root key\\Software (or
* \\Software\\Wow6432Node
*                        on x64 machines) where the CLSID values are located.
* @param source          Where the cLSIDs themselves are located (the key or the
* value).
* @param nameSource      Which name is primary; the name stored with the CLSID,
* or the name stored with the CLASSES key.
*/
static void ClsidSubkeyBasedOutput(log_sink& output,
                                   std::string const& prefix,
                                   std::string const& rootKey,
                                   std::string const& subKey,
                                   MasterName nameSource)
{
#ifdef _M_X64
    ClsidSubkeyBasedOutputWithBits(
        output,
        prefix,
        rootKey,
        "\\Software\\Wow6432Node" + subKey,
        "\\Software\\Wow6432Node\\Classes\\CLSID\\",
        "\\Registry\\Machine\\Software\\Wow6432Node\\Classes\\CLSID\\",
        nameSource);
#endif
    ClsidSubkeyBasedOutputWithBits(
        output,
        prefix + Get64Suffix(),
        rootKey,
        "\\Software" + subKey,
        "\\Software\\Classes\\CLSID\\",
        "\\Registry\\Machine\\Software\\Classes\\CLSID\\",
        nameSource);
}

/**
* Single registry value output.
*
* @param [in,out] output The output stream.
* @param key             The key where the output is rooted.
* @param valueName       Name of the value.
* @param prefix          The prefix to use in the log output.
* @param dataProcess     (optional) [in,out] The process by which the line data
*is written.
*                        The default escapes using the general escaping format.
*/
static void SingleRegistryValueOutput(
    log_sink& output,
    RegistryKey const& key,
    std::string const& valueName,
    std::string const& prefix,
    std::function<void(log_sink& out, std::string& source)> dataProcess =
        GeneralProcess)
{
    if (key.Invalid())
    {
        return;
    }
    try
    {
        auto value = key[valueName];
        write(output, prefix, ": ");
        std::string val(value.GetString());
        dataProcess(output, val);
        writeln(output);
    }
    catch (ErrorFileNotFoundException const&)
    {
        // Expected error.
    }
}

/**
* Internet explorer output.
*
* @param [in,out] output The output stream.
* @param rootKey         The root key where Internet Explorer is being rooted.
*/
static void InternetExplorerMainOutput(log_sink& output,
                                       std::string const& rootKey)
{
    RegistryKey ieRoot(
        RegistryKey::Open(rootKey + "\\Software\\Microsoft\\Internet Explorer",
                          KEY_QUERY_VALUE));
    RegistryKey ieMain(RegistryKey::Open(ieRoot, "Main", KEY_QUERY_VALUE));
    RegistryKey ieSearch(RegistryKey::Open(ieRoot, "Search", KEY_QUERY_VALUE));
    std::string suffix64(Get64Suffix());
#ifdef _M_X64
    RegistryKey ieRoot32(RegistryKey::Open(
        rootKey + "\\Software\\Wow6432Node\\Microsoft\\Internet Explorer",
        KEY_QUERY_VALUE));
    RegistryKey ieMain32(RegistryKey::Open(ieRoot, "Main", KEY_QUERY_VALUE));
    RegistryKey ieSearch32(RegistryKey::Open(ieRoot, "Main", KEY_QUERY_VALUE));
#endif
    SingleRegistryValueOutput(output,
                              ieMain,
                              "Default_Page_Url",
                              "DefaultPageUrl" + suffix64,
                              HttpProcess);
#ifdef _M_X64
    SingleRegistryValueOutput(
        output, ieMain32, "Default_Page_Url", "DefaultPageUrl", HttpProcess);
#endif
    SingleRegistryValueOutput(output,
                              ieMain,
                              "Default_Search_Url",
                              "DefaultSearchUrl" + suffix64,
                              HttpProcess);
#ifdef _M_X64
    SingleRegistryValueOutput(output,
                              ieMain32,
                              "Default_Search_Url",
                              "DefaultSearchUrl",
                              HttpProcess);
#endif
    SingleRegistryValueOutput(
        output, ieMain, "Local Page", "LocalPage" + suffix64, HttpProcess);
#ifdef _M_X64
    SingleRegistryValueOutput(
        output, ieMain32, "Local Page", "LocalPage", HttpProcess);
#endif
    SingleRegistryValueOutput(
        output, ieMain, "Start Page", "StartPage" + suffix64, HttpProcess);
#ifdef _M_X64
    SingleRegistryValueOutput(
        output, ieMain32, "Start Page", "StartPage", HttpProcess);
#endif
    SingleRegistryValueOutput(
        output, ieMain, "Search Page", "SearchPage" + suffix64, HttpProcess);
#ifdef _M_X64
    SingleRegistryValueOutput(
        output, ieMain32, "Search Page", "SearchPage", HttpProcess);
#endif
    SingleRegistryValueOutput(
        output, ieMain, "Search Bar", "SearchBar" + suffix64, HttpProcess);
#ifdef _M_X64
    SingleRegistryValueOutput(
        output, ieMain32, "Search Bar", "SearchBar", HttpProcess);
#endif
    SingleRegistryValueOutput(output,
                              ieMain,
                              "SearchMigratedDefaultUrl",
                              "SearchMigratedDefaultUrl" + suffix64,
                              HttpProcess);
#ifdef _M_X64
    SingleRegistryValueOutput(output,
                              ieMain32,
                              "SearchMigratedDefaultUrl",
                              "SearchMigratedDefaultUrl",
                              HttpProcess);
#endif
    SingleRegistryValueOutput(output,
                              ieMain,
                              "Security Risk Page",
                              "SecurityPage" + suffix64,
                              HttpProcess);
#ifdef _M_X64
    SingleRegistryValueOutput(
        output, ieMain32, "Security Risk Page", "SecurityPage", HttpProcess);
#endif
    SingleRegistryValueOutput(output,
                              ieMain,
                              "Window Title",
                              "WindowTitle" + suffix64,
                              HttpProcess);
#ifdef _M_X64
    SingleRegistryValueOutput(
        output, ieMain32, "Window Title", "WindowTitle", HttpProcess);
#endif
    SingleRegistryValueOutput(
        output, ieMain, "SearchURL", "SearchUrl" + suffix64, HttpProcess);
#ifdef _M_X64
    SingleRegistryValueOutput(
        output, ieMain32, "SearchURL", "SearchUrl", HttpProcess);
#endif
    SingleRegistryValueOutput(output,
                              ieSearch,
                              "SearchAssistant",
                              "SearchAssistant" + suffix64,
                              HttpProcess);
#ifdef _M_X64
    SingleRegistryValueOutput(output,
                              ieSearch32,
                              "SearchAssistant",
                              "SearchAssistant",
                              HttpProcess);
#endif
    SingleRegistryValueOutput(output,
                              ieSearch,
                              "CustomizeSearch",
                              "CustomizeSearch" + suffix64,
                              HttpProcess);
#ifdef _M_X64
    SingleRegistryValueOutput(output,
                              ieSearch32,
                              "CustomizeSearch",
                              "CustomizeSearch",
                              HttpProcess);
#endif
}

/**
* Process a single IE script entry.
*
* @param [in,out] out The output stream where the output is written.
* @param suffix       The suffix applied to the script entry (either "64" or
*nothing)
* @param valueName    Name of the value in question.
* @param subKey       The sub key to which the query of script is being posed.
*/
static void ProcessIeScript(log_sink& out,
                            std::string const& suffix,
                            std::string const& valueName,
                            RegistryKey const& subKey)
{
    try
    {
        std::string name(subKey.GetName());
        std::string command(subKey[valueName].GetString());
        GeneralEscape(name, '#', ']');
        GeneralEscape(command);
        writeln(out, "IeScript", suffix, ": [", std::move(name), "] ", command);
    }
    catch (ErrorFileNotFoundException const&)
    {
        // Expected
    }
}

/**
* Process a single IE COM entry.
*
* @param [in,out] out The output stream where the output is written.
* @param suffix       The suffix applied to the script entry (either "64" or
*nothing)
* @param valueName    Name of the value in question.
* @param subKey       The sub key to which the query of script is being posed.
*/
static void ProcessIeCom(log_sink& out,
                         std::string const& suffix,
                         std::string const& valueName,
                         RegistryKey const& subKey,
                         std::string const& hiveRootPath,
                         std::string const& software)
{
    try
    {
        std::string name(subKey.GetName());
        std::string clsid(subKey[valueName].GetString());
        std::string file("N/A");
        RegistryKey clsidKey(RegistryKey::Open(hiveRootPath + "\\" + software +
                                                   "\\Classes\\CLSID\\" +
                                                   clsid + "\\InProcServer32",
                                               KEY_QUERY_VALUE));
        if (clsidKey.Invalid())
        {
            clsidKey = RegistryKey::Open("\\Registry\\Machine\\" + software +
                                             "\\Classes\\CLSID\\" + clsid +
                                             "\\InProcServer32",
                                         KEY_QUERY_VALUE);
        }
        if (clsidKey.Valid())
        {
            std::string fileTry(clsidKey[""].GetString());
            if (!fileTry.empty())
            {
                file = std::move(fileTry);
            }
        }

        name.erase(name.cbegin(),
                   std::find(name.crbegin(), name.crend(), '\\').base());
        GeneralEscape(name, '#', ' ');
        GeneralEscape(clsid, '#', ']');
        write(out, "IeCom", suffix , ": [", name, ' ', clsid, "] ");
        WriteDefaultFileOutput(out, file);
        writeln(out);
    }
    catch (ErrorFileNotFoundException const&)
    {
        // Expected
    }
}

static void TrustedZoneRecursive(log_sink& out,
                                 std::string const& rootKey,
                                 std::string const& domainToEnter,
                                 std::string const& prefix)
{
    RegistryKey currentDomainKey =
        RegistryKey::Open(rootKey, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE);

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

        std::string outputUrl(value.GetName());
        outputUrl += "://";
        outputUrl += domainToEnter;
        HttpEscape(outputUrl);

        writeln(out, prefix, ": ", outputUrl);
    }

    auto const& subKeys = currentDomainKey.EnumerateSubKeyNames();
    currentDomainKey.Close();

    for (auto const& key : subKeys)
    {
        std::string subDomain(key);
        subDomain += '.';
        subDomain += domainToEnter;

        TrustedZoneRecursive(out, rootKey + "\\" + key, subDomain, prefix);
    }
}

static void TrustedZone(log_sink& out,
                        std::string const& keyPath,
                        std::string const& prefix)
{
    RegistryKey domainsRoot =
        RegistryKey::Open(keyPath, KEY_ENUMERATE_SUB_KEYS);
    if (domainsRoot.Invalid())
    {
        return;
    }

    auto rootDomains = domainsRoot.EnumerateSubKeyNames();
    for (auto const& rootDomain : rootDomains)
    {
        TrustedZoneRecursive(
            out, keyPath + "\\" + rootDomain, rootDomain, prefix);
    }
}

static void TrustedDefaults(log_sink& out,
                            std::string const& keyPath,
                            std::string const& prefix)
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

        writeln(out, prefix, ": ", value.GetName());
    }
}

static void TrustedIpRange(log_sink& out,
                           std::string const& keyPath,
                           std::string const& prefix)
{
    RegistryKey targetKey(RegistryKey::Open(keyPath, KEY_ENUMERATE_SUB_KEYS));
    if (targetKey.Invalid())
    {
        return;
    }

    for (auto const& subKey : targetKey.EnumerateSubKeyNames())
    {
        RegistryKey subHKey(
            RegistryKey::Open(keyPath + "\\" + subKey, KEY_QUERY_VALUE));
        std::string ipRange = subHKey[":Range"].GetStringStrict();
        GeneralEscape(ipRange);

        for (auto const& value : subHKey.EnumerateValues())
        {
            if (value.GetType() != REG_DWORD)
            {
                continue;
            }

            std::string name(subKey);
            GeneralEscape(name, '#', ']');

            std::string range(value.GetName());
            range += "://";
            range += ipRange;
            HttpEscape(range);
            writeln(out, prefix, ": [", name, "] ", range);
        }
    }
}

/**
* Explorer extensions output.
*
* @param [in,out] out The output stream.
* @param rootKey      The root key where the IE settings are rooted. (user or
*machine hive
*                     root)
*/
static void ExplorerExtensionsOutput(log_sink& out,
                                     std::string const& rootKey)
{
    std::string suffix(Get64Suffix());
    std::string software("Software");
    auto keyProcessor = [&](RegistryKey const & subKey) {
        ProcessIeScript(out, suffix, "Exec", subKey);
        ProcessIeScript(out, suffix, "Script", subKey);
        ProcessIeCom(out, suffix, "clsidextension", subKey, rootKey, software);
        ProcessIeCom(out, suffix, "bandclsid", subKey, rootKey, software);
    };
    RegistryKey key(RegistryKey::Open(
        rootKey + "\\Software\\Microsoft\\Internet Explorer\\Extensions",
        KEY_ENUMERATE_SUB_KEYS));
    if (key.Valid())
    {
        auto subkeys = key.EnumerateSubKeys(KEY_QUERY_VALUE);
        std::for_each(subkeys.cbegin(), subkeys.cend(), keyProcessor);
    }
#ifdef _M_X64
    key = RegistryKey::Open(
        rootKey +
            "\\Software\\Wow6432Node\\Microsoft\\Internet Explorer\\Extensions",
        KEY_ENUMERATE_SUB_KEYS);
    suffix = std::string();
    software = "Software\\Wow6432Node";
    if (key.Valid())
    {
        auto subkeys = key.EnumerateSubKeys(KEY_QUERY_VALUE);
        std::for_each(subkeys.cbegin(), subkeys.cend(), keyProcessor);
    }
#endif
    TrustedZone(
        out,
        rootKey +
            "\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\Domains",
        "Trusted Zone" + Get64Suffix());
#ifdef _M_X64
    TrustedZone(
        out,
        rootKey +
            "\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\Domains",
        "Trusted Zone");
#endif
    TrustedZone(
        out,
        rootKey +
            "\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\EscDomains",
        "ESC Trusted Zone" + Get64Suffix());
#ifdef _M_X64
    TrustedZone(
        out,
        rootKey +
            "\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\EscDomains",
        "ESC Trusted Zone");
#endif
    TrustedDefaults(
        out,
        rootKey +
            "\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\ProtocolDefaults",
        "Trusted Default Protocol" + Get64Suffix());
#ifdef _M_X64
    TrustedDefaults(
        out,
        rootKey +
            "\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\ProtocolDefaults",
        "Trusted Default Protocol");
#endif
    TrustedIpRange(
        out,
        rootKey +
            "\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\Ranges",
        "Trusted IP Range" + Get64Suffix());
#ifdef _M_X64
    TrustedIpRange(
        out,
        rootKey +
            "\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\Ranges",
        "Trusted IP Range");
#endif
}

/**
* Common HJT outputs. Does all the things that are repeated for the entire
*machine, and for
* each user's registry.
*
* @param [in,out] output The output stream to which the log is written.
* @param rootKey         The root key to check. (e.g. \\Registry\\Machine or
*\\Registry\\User\\
*                        ${Sid}
*/
static void CommonHjt(log_sink& output, std::string const& rootKey)
{
    InternetExplorerMainOutput(output, rootKey);
    ClsidValueBasedOutput(output,
                          "UrlSearchHook",
                          rootKey,
                          "\\Microsoft\\Internet Explorer\\URLSearchHooks",
                          NAME,
                          CLASS_ROOT_DEFAULT);
    RegistryKey winlogon(RegistryKey::Open(
        rootKey +
            "\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        KEY_QUERY_VALUE));
    SingleRegistryValueOutput(
        output, winlogon, "Shell", "Shell", FileProcess);
    SingleRegistryValueOutput(output,
                              winlogon,
                              "Userinit",
                              "Userinit",
                              [](log_sink & out, std::string & src) {
        if (src.back() == ',')
        {
            src.pop_back();
        }
        FileProcess(out, src);
    });
    SingleRegistryValueOutput(
        output, winlogon, "UIHost", "UIHost", FileProcess);
    SingleRegistryValueOutput(
        output, winlogon, "TaskMan", "TaskMan", FileProcess);
    if (winlogon.Valid())
    {
        try
        {
            DWORD sfc = winlogon["SFCDisable"].GetDWord();
            if (sfc)
            {
                writeln(output, "SFC: Disabled");
            }
            else
            {
                writeln(output, "SFC: Enabled");
            }
        }
        catch (ErrorFileNotFoundException const&)
        {
            // Expected
        }
    }
    ClsidSubkeyBasedOutput(
        output,
        "BHO",
        rootKey,
        "\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects",
        CLASS_ROOT_DEFAULT);
    ClsidValueBasedOutput(output,
                          "TB",
                          rootKey,
                          "\\Microsoft\\Internet Explorer\\Toolbar",
                          NAME,
                          CLASS_ROOT_DEFAULT);
    ClsidValueBasedOutput(
        output,
        "TB",
        rootKey,
        "\\Microsoft\\Internet Explorer\\Toolbar\\WebBrowser",
        NAME,
        CLASS_ROOT_DEFAULT);
    ClsidValueBasedOutput(output,
                          "EB",
                          rootKey,
                          "\\Microsoft\\Internet Explorer\\Explorer Bars",
                          NAME,
                          CLASS_ROOT_DEFAULT);
    RunKeyOutput(output, rootKey, "Run");
    RunKeyOutput(output, rootKey, "RunOnce");
    RunKeyOutput(output, rootKey, "RunServices");
    RunKeyOutput(output, rootKey, "RunServicesOnce");
    ExplorerRunOutput(output, rootKey);
    ValueMajorBasedEnumerationBitless(
        output,
        rootKey,
        "\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
        "\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
        "PoliciesExplorer",
        GeneralProcess);
    ValueMajorBasedEnumerationBitless(
        output,
        rootKey,
        "\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
        "\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
        "PoliciesSystem",
        GeneralProcess);
    ValueMajorBasedEnumerationBitless(
        output,
        rootKey,
        "\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\DisallowRun",
        "\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\DisallowRun",
        "PoliciesDisallowRun",
        GeneralProcess);
    SubkeyMajorBasedEnumerationBitless(
        output,
        rootKey,
        "\\Software\\Microsoft\\Internet Explorer\\MenuExt",
        "\\Software\\Wow6432Node\\Microsoft\\Internet Explorer\\MenuExt",
        "",
        "IeMenu",
        GeneralProcess);
    ExplorerExtensionsOutput(output, rootKey);
}

/**
* Looks up a given SID to find its associated account name, and returns a
*DOMAIN\User form
* string matching that user's name.
*
* @param stringSid The SID of the user in string format.
*
* @return The user's name and domain in DOMAIN\USER format.
*/
static std::string LookupAccountNameBySid(std::string const& stringSid)
{
    std::wstring stringSideWide(utf8::ToUtf16(stringSid));
    if (stringSid == ".DEFAULT")
    {
        return "Default User";
    }
    PSID sidPtr = nullptr;
    ScopeExit killSidPtr([sidPtr]() {
        if (sidPtr != nullptr)
        {
            ::LocalFree(sidPtr);
        }
    });
    if (::ConvertStringSidToSidW(stringSideWide.c_str(), &sidPtr) == 0)
    {
        Win32Exception::ThrowFromLastError();
    }
    SID_NAME_USE use;
    std::wstring userName, domainName;
    userName.resize(128);
    DWORD userNameCount = static_cast<DWORD>(userName.size());
    domainName.resize(128);
    DWORD domainNameCount = static_cast<DWORD>(domainName.size());
    if (::LookupAccountSidW(nullptr,
                            sidPtr,
                            &userName[0],
                            &userNameCount,
                            &domainName[0],
                            &domainNameCount,
                            &use) == 0)
    {
        if (userName.size() != static_cast<std::size_t>(userNameCount) ||
            domainName.size() != static_cast<std::size_t>(domainNameCount))
        {
            userName.resize(userNameCount);
            domainName.resize(domainNameCount);
            if (::LookupAccountSidW(nullptr,
                                    sidPtr,
                                    &userName[0],
                                    &userNameCount,
                                    &domainName[0],
                                    &domainNameCount,
                                    &use) == 0)
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
    domainName.push_back('\\');
    return utf8::ToUtf8(domainName + userName);
}

static void ExecuteDpf(log_sink& output)
{
    RegistryKey dpfRoot(RegistryKey::Open(
        "\\Registry\\Machine\\Software\\Microsoft\\Code Store Database\\Distribution Units",
        KEY_ENUMERATE_SUB_KEYS));
    if (dpfRoot.Valid())
    {
        for (std::string const& clsid : dpfRoot.EnumerateSubKeyNames())
        {
            std::string clsidEscaped(clsid);
            GeneralEscape(clsidEscaped);
            writeln(output, "DPF", Get64Suffix(), ": ", clsidEscaped);
        }
    }
#ifdef _M_X64
    dpfRoot = RegistryKey::Open(
        "\\Registry\\Machine\\Software\\Wow6432Node\\Microsoft\\Code Store Database\\Distribution Units",
        KEY_ENUMERATE_SUB_KEYS);
    if (dpfRoot.Valid())
    {
        for (std::string const& clsid : dpfRoot.EnumerateSubKeyNames())
        {
            std::string clsidEscaped(clsid);
            GeneralEscape(clsidEscaped);
            writeln(output, "DPF: ", clsidEscaped);
        }
    }
#endif
}

static void ExecuteLspChain(log_sink& output,
                            RegistryKey& protocolCatalogKey,
                            std::string suffix)
{
    DWORD catalogEntries =
        protocolCatalogKey["Num_Catalog_Entries" + suffix].GetDWord();
    bool chainOk = true;
    std::set<std::string> catalogFiles;
    RegistryKey catalogEntriesKey(RegistryKey::Open(protocolCatalogKey,
                                                    "Catalog_Entries" + suffix,
                                                    KEY_ENUMERATE_SUB_KEYS));
    auto catalogEntryNames = catalogEntriesKey.EnumerateSubKeyNames();
    std::sort(catalogEntryNames.begin(), catalogEntryNames.end());
    if (catalogEntries != catalogEntryNames.size())
    {
        chainOk = false;
    }

    for (std::size_t idx = 0; idx < catalogEntryNames.size(); ++idx)
    {
        std::string const& actualName = catalogEntryNames[idx];
        char expectedName[13];
        sprintf_s(expectedName, "%012u", idx + 1);
        if (actualName != expectedName)
        {
            chainOk = false;
        }

        RegistryKey catalogEntryKey(
            RegistryKey::Open(catalogEntriesKey, actualName, KEY_QUERY_VALUE));
        RegistryValue packedCatalogItem(catalogEntryKey["PackedCatalogItem"]);
        auto const end = std::find(
            packedCatalogItem.cbegin(), packedCatalogItem.cend(), '\0');
        catalogFiles.emplace(packedCatalogItem.cbegin(), end);
    }

    for (std::string catalogFile : catalogFiles)
    {
        GeneralEscape(catalogFile);
        writeln(output, "LSP", suffix, ": ", catalogFile);
    }

    if (!chainOk)
    {
        writeln(output, "LSP", suffix, ": Chain Broken");
    }
}

static void ExecuteNspChain(log_sink& output,
                            RegistryKey& namespaceCatalogKey,
                            std::string suffix)
{
    DWORD catalogEntries =
        namespaceCatalogKey["Num_Catalog_Entries" + suffix].GetDWord();
    bool chainOk = true;
    std::set<std::string> catalogFiles;
    RegistryKey catalogEntriesKey(RegistryKey::Open(namespaceCatalogKey,
                                                    "Catalog_Entries" + suffix,
                                                    KEY_ENUMERATE_SUB_KEYS));
    auto catalogEntryNames = catalogEntriesKey.EnumerateSubKeyNames();
    std::sort(catalogEntryNames.begin(), catalogEntryNames.end());
    if (catalogEntries != catalogEntryNames.size())
    {
        chainOk = false;
    }

    for (std::size_t idx = 0; idx < catalogEntryNames.size(); ++idx)
    {
        std::string const& actualName = catalogEntryNames[idx];
        char expectedName[13];
        sprintf_s(expectedName, "%012u", idx + 1);
        if (actualName != expectedName)
        {
            chainOk = false;
        }

        RegistryKey catalogEntryKey(
            RegistryKey::Open(catalogEntriesKey, actualName, KEY_QUERY_VALUE));
        catalogFiles.emplace(catalogEntryKey["LibraryPath"].GetString());
    }

    for (std::string catalogFile : catalogFiles)
    {
        GeneralEscape(catalogFile);
        writeln(output, "NSP", suffix, ": ", catalogFile);
    }

    if (!chainOk)
    {
        writeln(output, "NSP", suffix, ": Chain Broken");
    }
}

static void ExecuteWinsock2Parameters(log_sink& output)
{
    RegistryKey winsockParameters(RegistryKey::Open(
        "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Winsock2\\Parameters",
        KEY_QUERY_VALUE));

    std::string protocolCatalog(
        winsockParameters["Current_Protocol_Catalog"].GetStringStrict());
    RegistryKey protocolCatalogKey(
        RegistryKey::Open(winsockParameters, protocolCatalog, KEY_QUERY_VALUE));
    ExecuteLspChain(output, protocolCatalogKey, "");
#ifdef _M_X64
    ExecuteLspChain(output, protocolCatalogKey, "64");
#endif
    protocolCatalogKey.Close();

    std::string namespaceCatalog(
        winsockParameters["Current_NameSpace_Catalog"].GetStringStrict());
    RegistryKey namespaceCatalogKey(RegistryKey::Open(
        winsockParameters, namespaceCatalog, KEY_QUERY_VALUE));
    ExecuteNspChain(output, namespaceCatalogKey, "");
#ifdef _M_X64
    ExecuteNspChain(output, namespaceCatalogKey, "64");
#endif
    namespaceCatalogKey.Close();
    winsockParameters.Close();
}

static void TcpSettingForInterface(log_sink& output,
                                   RegistryKey const& parametersKey,
                                   std::string const& interfaceName,
                                   std::string const& value)
{
    try
    {
        std::string nameServer(parametersKey[value].GetStringStrict());
        if (nameServer.empty())
        {
            return;
        }

        if (interfaceName.empty())
        {
            writeln(output, "Tcp", value, ": ", nameServer);
        }
        else
        {
            writeln(output, "Tcp", value, ": [", interfaceName, "] ", nameServer);
        }
    }
    catch (ErrorFileNotFoundException const&)
    {
    }
}

static void TcpNameserversForInterface(log_sink& output,
                                       RegistryKey const& parametersKey,
                                       std::string interfaceName)
{
    GeneralEscape(interfaceName, '#', ']');
    TcpSettingForInterface(output, parametersKey, interfaceName, "NameServer");
    TcpSettingForInterface(
        output, parametersKey, interfaceName, "DHCPNameServer");
}

static void TcpNameservers(log_sink& output)
{
    RegistryKey parametersKey(RegistryKey::Open(
        "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters",
        KEY_QUERY_VALUE));
    TcpNameserversForInterface(output, parametersKey, "");
    RegistryKey interfacesKey(RegistryKey::Open(
        parametersKey, "Interfaces", KEY_ENUMERATE_SUB_KEYS));
    for (auto const& interfaceKeyName : interfacesKey.EnumerateSubKeyNames())
    {
        RegistryKey interfaceKey(RegistryKey::Open(
            interfacesKey, interfaceKeyName, KEY_QUERY_VALUE));
        TcpNameserversForInterface(
            output, interfaceKey, std::move(interfaceKeyName));
    }
}

static void ProtocolsSectionRecursive(log_sink& output,
    std::string const& rootKey,
    std::string const& namespaceToEnter,
    std::string const& prefix)
{
    RegistryKey currentNamespaceKey =
        RegistryKey::Open(rootKey, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE);

    try
    {
        std::string itemName(namespaceToEnter);
        std::string clsid = currentNamespaceKey["Clsid"].GetStringStrict();
        std::string clsidKeyName = "\\Registry\\Machine\\SOFTWARE\\Classes\\CLSID\\" + clsid + "\\InProcServer32";
        RegistryKey clsidKey = RegistryKey::Open(clsidKeyName, KEY_QUERY_VALUE);
        if (clsidKey.Valid())
        {
            std::string file = clsidKey[""].GetStringStrict();
            GeneralEscape(itemName, '#', ' ');
            GeneralEscape(clsid, '#', ']');
            write(output, prefix, ": [", itemName, " ", clsid, "] ");
            WriteDefaultFileOutput(output, file);
            writeln(output);
        }
    }
    catch (ErrorFileNotFoundException const&)
    {
    }

    auto const& subKeys = currentNamespaceKey.EnumerateSubKeyNames();
    currentNamespaceKey.Close();

    for (auto const& key : subKeys)
    {
        std::string subNamespace(key);
        subNamespace += '.';
        subNamespace += namespaceToEnter;

        ProtocolsSectionRecursive(output, rootKey + "\\" + key, subNamespace, prefix);
    }
}

static void ProtocolsSection(log_sink& out, std::string const& registryPath, std::string const& prefix)
{
    std::string rootPath(registryPath);
    RegistryKey handlersRoot =
        RegistryKey::Open(rootPath, KEY_ENUMERATE_SUB_KEYS);
    if (handlersRoot.Invalid())
    {
        return;
    }

    auto handlers = handlersRoot.EnumerateSubKeyNames();
    for (auto const& rootHandler : handlers)
    {
        ProtocolsSectionRecursive(
            out, rootPath + "\\" + rootHandler, rootHandler, prefix);
    }
}

static void Protocols(log_sink& output)
{
    ProtocolsSection(output, "\\Registry\\Machine\\SOFTWARE\\Classes\\PROTOCOLS\\Filter", "IeFilter");
    ProtocolsSection(output, "\\Registry\\Machine\\SOFTWARE\\Classes\\PROTOCOLS\\Handler", "IeHandler");
    ProtocolsSection(output, "\\Registry\\Machine\\SOFTWARE\\Classes\\PROTOCOLS\\Name-Space Handler", "IeNamespace");
}

static void WinlogonNotify(log_sink& output)
{
    SubkeyMajorBasedEnumeration(output, "\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify", "DllName", "Notify", FileProcess);
}

static void SingleCommaValueBitless(log_sink& output, std::string const& rootKey, std::string const& valueName, std::string const& prefix, std::string const& suffix)
{
    RegistryKey key(RegistryKey::Open(rootKey, KEY_QUERY_VALUE));
    if (!key.Valid())
    {
        return;
    }

    try
    {
        for (auto const& dll : key[valueName].GetCommaStringArray())
        {
            if (dll.empty())
            {
                continue;
            }

            write(output, prefix, suffix, ": ");
            WriteDefaultFileOutput(output, dll);
            writeln(output);
        }
    }
    catch (ErrorFileNotFoundException const&)
    {
    }
}

static void SingleCommaValue(log_sink& output, std::string const& rootKey, std::string const& valueName, std::string const& prefix)
{
#ifdef _M_X64
    SingleCommaValueBitless(output, "\\Registry\\Machine\\SOFTWARE\\Wow6432Node\\" + rootKey, valueName, prefix, "");
#endif
    SingleCommaValueBitless(output, "\\Registry\\Machine\\SOFTWARE\\" + rootKey, valueName, prefix, Get64Suffix());
}

static void AppinitDlls(log_sink& output)
{
    SingleCommaValue(output, "Microsoft\\Windows NT\\CurrentVersion\\Windows", "Appinit_DLLs", "AppinitDll");
}

static void ShellServiceObjectDelayLoad(log_sink& output)
{
    ClsidValueBasedOutput(
        output,
        "Ssodl",
        "\\Registry\\Machine",
        "\\Microsoft\\Windows\\CurrentVersion\\ShellServiceObjectDelayLoad",
        VALUE,
        CLASS_ROOT_DEFAULT);
}

static void SharedTaskScheduler(log_sink& output)
{
    ClsidValueBasedOutput(
        output,
        "Sts",
        "\\Registry\\Machine",
        "\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SharedTaskScheduler",
        NAME,
        CLASS_ROOT_DEFAULT);
}

static void ShellExecuteHooks(log_sink& output)
{
    ClsidValueBasedOutput(
        output,
        "Seh",
        "\\Registry\\Machine",
        "\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellExecuteHooks",
        NAME,
        CLASS_ROOT_DEFAULT);
}

static void SecurityProviders(log_sink& output)
{
    SingleCommaValueBitless(output, "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\SecurityProviders", "SecurityProviders", "SecurityProvider", "");
}

static void LocalSecurityAuthorityValue(log_sink& output, RegistryKey const& key, std::string const& value)
{
    std::string valueName(value + " Packages");
    std::vector<std::string> entries;
    for (auto const& entry : key[valueName].GetMultiStringArray())
    {
        if (entry.empty())
        {
            continue;
        }

        entries.emplace_back(entry + ".dll");
    }

    std::sort(entries.begin(), entries.end());
    for (std::string const& entry : entries)
    {
        write(output, value, "Package: ");
        WriteDefaultFileOutput(output, entry);
        writeln(output);
    }
}

static void LocalSecurityAuthority(log_sink& output)
{
    RegistryKey key(RegistryKey::Open("\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Lsa", KEY_QUERY_VALUE));
    LocalSecurityAuthorityValue(output, key, "Authentication");
    LocalSecurityAuthorityValue(output, key, "Notification");
    LocalSecurityAuthorityValue(output, key, "Security");
}

static void CsrssDll(log_sink& output)
{
    RegistryKey key(RegistryKey::Open("\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Subsystems"));
    std::string const csrssCommandLine = key["Windows"].GetStringStrict();
    std::regex serverDllRegex("ServerDll=([^:,]+)(:[^,]+)?,[\\d]", std::regex_constants::ECMAScript | std::regex_constants::icase);
    std::sregex_iterator begin(csrssCommandLine.cbegin(), csrssCommandLine.cend(), serverDllRegex);
    std::sregex_iterator end;

    std::vector<std::string> values;
    for (; begin != end; ++begin)
    {
        std::ssub_match currentMatch = (*begin)[1];
        std::string value = currentMatch.str() + ".dll";
        if (Path::ResolveFromCommandLine(value))
        {
            values.emplace_back(std::move(value));
        }
    }

    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    for (std::string& str : values)
    {
        write(output, "SubSystems: ");
        WriteDefaultFileOutput(output, str);
        writeln(output);
    }
}

static void ActiveSetup(log_sink& output)
{
    SubkeyMajorBasedEnumerationBitless(output,
        "\\Registry\\Machine\\Software",
        "\\Microsoft\\Active Setup\\Installed Components",
        "\\Wow6432Node\\Microsoft\\Active Setup\\Installed Components",
        "StubPath",
        "ActiveSetup");
}

static void ImageFileExecutionOptions(log_sink& output)
{
    SubkeyMajorBasedEnumeration(output,
        "\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options",
        "Debugger",
        "Ifeo");
}

static void FileAssociation(log_sink& output, std::string const& association)
{
    RegistryKey key(RegistryKey::Open("\\Registry\\Machine\\Software\\Classes\\" + association));
    if (key.Invalid())
    {
        return;
    }

    std::string namedExtension = key[""].GetStringStrict();
    key = RegistryKey::Open("\\Registry\\Machine\\Software\\Classes\\" + namedExtension + "\\Shell");
    std::string defaultVerb("open");
    try
    {
        defaultVerb = key[""].GetStringStrict();
    }
    catch (ErrorFileNotFoundException const&)
    {}

    key = RegistryKey::Open("\\Registry\\Machine\\Software\\Classes\\" + namedExtension + "\\Shell\\" + defaultVerb + "\\Command");
    std::string command = key[""].GetStringStrict();

    GeneralEscape(namedExtension, '#', '-');
    GeneralEscape(defaultVerb, '#', '-');
    GeneralEscape(command, '#');

    writeln(output, "Association: ", association, "->", namedExtension, "->", defaultVerb, "->", command);
}

static void FileAssociations(log_sink& output)
{
    char const* associationList[] = {
        ".exe",
        ".bat",
        ".cmd",
        ".com",
        ".pif",
        ".scr",
        ".reg",
        ".txt",
        ".chm",
        ".inf",
        ".ini",
        ".vbe",
        ".vbs",
        ".jse",
        ".jsf"
    };

    for (char const* association : associationList)
    {
        FileAssociation(output, association);
    }
}

static void HostsFile(log_sink& output)
{
    RegistryKey key(RegistryKey::Open("\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters"));
    std::string dataBasePath = key["DataBasePath"].GetStringStrict();
    dataBasePath += "\\Hosts";
    dataBasePath = Path::ExpandEnvStrings(std::move(dataBasePath));
    File hostsFile(dataBasePath);

    std::vector<std::string> hostsLines = hostsFile.ReadAllLines();
    GeneralEscape(dataBasePath);
    writeln(output, "HostsFile: ", dataBasePath);
    for (std::string& hostsLine : hostsLines)
    {
        boost::algorithm::trim(hostsLine);
        if (hostsLine.empty())
        {
            continue;
        }

        if (boost::algorithm::starts_with(hostsLine, "#"))
        {
            continue;
        }

        HttpEscape(hostsLine);
        writeln(output, "Hosts: ", hostsLine);
    }
}

static void MachineSpecificHjt(log_sink& output)
{
    ExecuteDpf(output);
    ExecuteWinsock2Parameters(output);
    TcpNameservers(output);
    Protocols(output);
    WinlogonNotify(output);
    AppinitDlls(output);
    ShellServiceObjectDelayLoad(output);
    SharedTaskScheduler(output);
    ShellExecuteHooks(output);
    SecurityProviders(output);
    LocalSecurityAuthority(output);
    CsrssDll(output);
    ActiveSetup(output);
    ImageFileExecutionOptions(output);
    FileAssociations(output);
    HostsFile(output);
}

static void InternetConnectionWizardShellNext(log_sink& output, std::string const& rootKey)
{
    RegistryKey key(RegistryKey::Open(rootKey + "\\Software\\Microsoft\\Internet Connection Wizard"));
    if (key.Invalid())
    {
        return;
    }

    try
    {
        std::string shellNext = key["ShellNext"].GetStringStrict();
        HttpEscape(shellNext);
        writeln(output, "InternetConnectionWizard: ", shellNext);
    }
    catch (ErrorFileNotFoundException const&)
    { }
}

static void ProxySettings(log_sink& output, std::string const& rootKey)
{
    RegistryKey key(RegistryKey::Open(rootKey + "\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"));
    if (key.Invalid())
    {
        return;
    }

    try
    {
        std::string proxyServer = key["ProxyServer"].GetStringStrict();
        GeneralEscape(proxyServer);
        writeln(output, "ProxyServer: ", proxyServer);
    }
    catch (ErrorFileNotFoundException const&)
    { }

    try
    {
        std::string proxyOverride = key["ProxyOverride"].GetStringStrict();
        GeneralEscape(proxyOverride);
        writeln(output, "ProxyOverride: ", proxyOverride);
    }
    catch (ErrorFileNotFoundException const&)
    { }
}

static void IniAutostarts(log_sink& output, std::string const& rootKey)
{
    RegistryKey key(RegistryKey::Open(rootKey + "\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows"));
    if (key.Invalid())
    {
        return;
    }

    try
    {
        std::string load = key["load"].GetStringStrict();
        GeneralEscape(load);
        writeln(output, "IniLoad: ", load);
    }
    catch (ErrorFileNotFoundException const&)
    { }

    try
    {
    std::string run = key["run"].GetStringStrict();
    GeneralEscape(run);
    writeln(output, "IniRun: ", run);
    }
    catch (ErrorFileNotFoundException const&)
    { }
}

static std::string ResolveLink(std::string const& lnkPathNarrow)
{
    // Stolen from PEV
    std::string result;
    std::wstring lnkPath(utf8::ToUtf16(lnkPathNarrow));
    IPersistFile * persistFile;
    IShellLink * theLink;
    HRESULT errorCheck = 0;
    wchar_t linkTarget[MAX_PATH];
    wchar_t expandedTarget[MAX_PATH];
    wchar_t arguments[INFOTIPSIZE];
    errorCheck = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IPersistFile, reinterpret_cast<void **>(&persistFile));
    if (!SUCCEEDED(errorCheck)) goto error;
    if (!SUCCEEDED(persistFile->Load(lnkPath.c_str(), 0))) goto unloadPersist;
    errorCheck = persistFile->QueryInterface(IID_IShellLinkW, reinterpret_cast<void **>(&theLink));
    if (!SUCCEEDED(errorCheck)) goto unloadPersist;
    if (!SUCCEEDED(theLink->Resolve(NULL, SLR_NO_UI))) goto unloadLink;
    if (!SUCCEEDED(theLink->GetPath(linkTarget, MAX_PATH, NULL, SLGP_RAWPATH))) goto unloadLink;
    errorCheck = theLink->GetArguments(arguments, INFOTIPSIZE);
    if (!SUCCEEDED(errorCheck)) arguments[0] = L'\0';
    ExpandEnvironmentStringsW(linkTarget, expandedTarget, MAX_PATH);
    persistFile->Release();
    theLink->Release();
    result = utf8::ToUtf8(static_cast<std::wstring>(expandedTarget)+L" " + arguments);

unloadLink:
    theLink->Release();
unloadPersist:
    persistFile->Release();
error:
    return result;
}

static void StartupFolder(log_sink& output, std::string const& rootKey)
{
    RegistryKey key(RegistryKey::Open(rootKey + "\\Volatile Environment"));
    if (key.Invalid())
    {
        return;
    }

    std::string startupFolderSpec = key["USERPROFILE"].GetStringStrict() + "\\Start Menu\\Startup\\*";

    FindFiles startupFiles(startupFolderSpec);
    while (startupFiles.NextSuccess())
    {
        std::string fileName = startupFiles.GetRecord().GetFileName();
        std::string target = ResolveLink(fileName);
        write(output, "StartupFolder: ");
        if (target.empty())
        {
            WriteDefaultFileOutput(output, fileName);
        }
        else
        {
            write(output, fileName, "->");
            WriteDefaultFileOutput(output, target);
        }

        writeln(output);
    }
}

static void UserSpecificHjt(log_sink& output, std::string const& rootKey)
{
    InternetConnectionWizardShellNext(output, rootKey);
    ProxySettings(output, rootKey);
    IniAutostarts(output, rootKey);
    StartupFolder(output, rootKey);
}

static void WriteMachineIdentity(log_sink& output)
{
    auto const length = MAX_COMPUTERNAME_LENGTH + 1;
    DWORD resultLength = length;
    wchar_t compName[length];
    if (::GetComputerNameW(compName, &resultLength) == 0)
    {
        Win32Exception::ThrowFromLastError();
    }

    std::string narrowName = utf8::ToUtf8(compName);
    GeneralEscape(narrowName, '#', ']');
    writeln(output, "Identity: [", narrowName, "] MACHINE");
}

void LoadPointsReport::Execute(log_sink& output,
                        ScriptSection const&,
                        std::vector<std::string> const&) const
{
    WriteMachineIdentity(output);
    SecurityCenterOutput(output);
    CommonHjt(output, "\\Registry\\Machine");
    MachineSpecificHjt(output);

    auto hives = EnumerateUserHives();
    for (std::string const& hive : hives)
    {
        string_sink userSink;
        CommonHjt(userSink, hive);
        UserSpecificHjt(userSink, hive);
        std::string const& userSettings = userSink.get();
        if (userSettings.empty())
        {
            continue;
        }

        std::string head("User Settings");
        Header(head);
        std::string sid(std::find(hive.crbegin(), hive.crend(), '\\').base(),
                         hive.end());
        std::string user(LookupAccountNameBySid(sid));
        GeneralEscape(user, '#', ']');
        writeln(output);
        writeln(output, head);
        writeln(output);
        writeln(output, "Identity: [", user, "] ", sid);
        write(output, userSettings);
    }
}
}
