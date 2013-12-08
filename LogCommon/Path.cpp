// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <unordered_set>
#include <boost/algorithm/string.hpp>
#include "File.hpp"
#include "StringUtilities.hpp"
#include "Win32Exception.hpp"
#include "Path.hpp"
#include "Utf8.hpp"

namespace Instalog
{
namespace Path
{

std::string Append(std::string path, std::string const& more)
{
    if (more.size() != 0)
    {
        if (path.size() == 0)
        {
            path.assign(more);
        }
        else
        {
            std::string::const_iterator pathend = path.end() - 1;
            std::string::const_iterator morebegin = more.begin();

            if (*pathend == '\\' && *morebegin == '\\')
            {
                path.append(++morebegin, more.end());
            }
            else if (*pathend == '\\' || *morebegin == '\\')
            {
                path.append(more);
            }
            else
            {
                path.push_back('\\');
                path.append(more);
            }
        }
    }

    return path;
}

std::string GetWindowsPath()
{
    wchar_t windir[MAX_PATH];
    UINT len = ::GetWindowsDirectoryW(windir, MAX_PATH);
    windir[len++] = L'\\';
    return utf8::ToUtf8(windir, windir + len);
}

static void NativePathToWin32Path(std::string& path)
{
    // Remove \, ??\, \?\, and globalroot\ 
    std::string::iterator chop = path.begin();
    if (boost::starts_with(boost::make_iterator_range(chop, path.end()), "\\"))
    {
        chop += 1;
    }
    if (boost::starts_with(boost::make_iterator_range(chop, path.end()),
                           "??\\"))
    {
        chop += 3;
    }
    if (boost::starts_with(boost::make_iterator_range(chop, path.end()),
                           "\\?\\"))
    {
        chop += 3;
    }
    if (boost::istarts_with(boost::make_iterator_range(chop, path.end()),
                            "globalroot\\"))
    {
        chop += 11;
    }
    path.erase(path.begin(), chop);

    static std::string windowsDirectory = GetWindowsPath();
    if (boost::istarts_with(path, "system32\\"))
    {
        path.insert(0, windowsDirectory);
    }
    else if (boost::istarts_with(path, "systemroot\\"))
    {
        path.replace(0, 11, windowsDirectory);
    }
    else if (boost::istarts_with(path, "%systemroot%\\")) // TODO: Move this
                                                          // somewhere else
                                                          // eventually
    {
        path.replace(0, 13, windowsDirectory);
    }
}

static std::vector<std::string>
getSplitEnvironmentVariable(wchar_t const* variable)
{
    using namespace std::placeholders;
    std::vector<std::string> splitVar;
    wchar_t buf[32767] = L""; // 32767 is max size of environment variable
    UINT len = ::GetEnvironmentVariableW(variable, buf, 32767);
    std::string asUtf8(utf8::ToUtf8(buf, buf + len));
    boost::split(splitVar, asUtf8, std::bind1st(std::equal_to<char>(), ';'));
    return splitVar;
}

static std::vector<std::string> getSplitPath()
{
    return getSplitEnvironmentVariable(L"PATH");
}

static std::vector<std::string> getSplitPathExt()
{
    return getSplitEnvironmentVariable(L"PATHEXT");
}

static bool RundllCheck(std::string& path)
{
    static std::string rundllpath =
        GetWindowsPath().append("System32\\rundll32");

    if (boost::istarts_with(path, rundllpath))
    {
        std::string::iterator firstComma =
            std::find(path.begin() + rundllpath.size(), path.end(), ',');
        if (firstComma == path.end())
        {
            return false;
        }
        path.erase(firstComma, path.end());
        path.erase(path.begin(), path.begin() + rundllpath.size());
        if (boost::istarts_with(path, L".exe"))
        {
            path.erase(0, 4);
        }
        boost::trim(path);
        if (path.size() == 0)
        {
            return false;
        }
        ResolveFromCommandLine(path);
        return true;
    }

    return false;
}

static bool IsExclusiveFileCached(std::string const& testPath)
{
    static std::unordered_set<std::string> nonexistentCache;
    auto cacheValue = nonexistentCache.find(testPath);
    if (cacheValue == nonexistentCache.end())
    {
        if (SystemFacades::File::IsExclusiveFile(testPath))
        {
            return true;
        }
        else
        {
            nonexistentCache.emplace(testPath, 1);
            return false;
        }
    }
    else
    {
        return false;
    }
}

static bool TryExtensions(std::string& searchpath,
                          std::string::iterator extensionat)
{
    static std::vector<std::string> splitPathExt = getSplitPathExt();

    // Try rundll32 check first
    if (RundllCheck(searchpath))
    {
        return true;
    }

    // Search with no path extension
    std::string pathNoPathExtension =
        std::string(searchpath.begin(), extensionat);
    if (IsExclusiveFileCached(pathNoPathExtension))
    {
        searchpath = pathNoPathExtension;
        return true;
    }
    auto pathNoExtensionSize = pathNoPathExtension.size();

    // Try the available path extensions
    for (auto splitPathExtIt = splitPathExt.cbegin();
         splitPathExtIt != splitPathExt.cend();
         ++splitPathExtIt)
    {
        pathNoPathExtension.append(*splitPathExtIt);
        if (IsExclusiveFileCached(pathNoPathExtension))
        {
            searchpath.assign(pathNoPathExtension);
            return true;
        }
        pathNoPathExtension.resize(pathNoExtensionSize);
    }

    return false;
}

static bool TryExtensionsAndPaths(std::string& path,
                                  std::string::iterator spacelocation)
{
    // First, try all of the available extensions
    if (TryExtensions(path, spacelocation))
        return true;

    // Second, don't bother trying path prefixes if we start with a drive
    if (path.size() >= 2 && iswalpha(path[0]) && path[1] == ':')
        return false;

    // Third, try to prepend it with each path in %PATH% and try each extension
    static std::vector<std::string> splitPath = getSplitPath();
    for (std::vector<std::string>::iterator splitPathIt = splitPath.begin();
         splitPathIt != splitPath.end();
         ++splitPathIt)
    {
        std::string longpath =
            Path::Append(*splitPathIt, std::string(path));
        std::string::iterator longpathspacelocation =
            longpath.end() - std::distance(spacelocation, path.end());
        if (TryExtensions(longpath, longpathspacelocation))
        {
            path = longpath;
            return true;
        }
    }

    return false;
}

static bool StripArgumentsFromPath(std::string& path)
{
    auto subpath = path.begin();
    // For each spot where there's a space, try all available extensions
    do
    {
        subpath = std::find(subpath + 1, path.end(), ' ');
        if (TryExtensionsAndPaths(path, subpath))
        {
            return true;
        }
    } while (subpath != path.end());

    return false;
}

static std::string GetRundll32Path()
{
    return Path::Append(GetWindowsPath(), "System32\\Rundll32.exe");
}

bool ResolveFromCommandLine(std::string& path)
{
    if (path.empty())
    {
        return false;
    }
    path = Path::ExpandEnvStrings(path);

    if (path[0] == '\"')
    {
        std::string unescaped;
        unescaped.reserve(path.size());
        std::string::iterator endOfUnescape = CmdLineToArgvWUnescape(
            path.begin(), path.end(), std::back_inserter(unescaped));
        if (boost::istarts_with(unescaped, GetRundll32Path()))
        {
            std::string::iterator startOfArgument =
                std::find(endOfUnescape, path.end(), '\"');
            if (startOfArgument != path.end())
            {
                unescaped.push_back(L' ');
                CmdLineToArgvWUnescape(
                    startOfArgument,
                    path.end(),
                    std::back_inserter(unescaped)); // Unescape the argument
                RundllCheck(unescaped);
            }
        }

        path = unescaped;
        ExpandShortPath(path);
        Prettify(path.begin(), path.end());
        return SystemFacades::File::IsExclusiveFile(path);
    }
    else
    {
        NativePathToWin32Path(path);
        bool status = StripArgumentsFromPath(path);
        if (status)
        {
            ExpandShortPath(path);
            Prettify(path.begin(), path.end());
        }
        return status;
    }
}

void Prettify(std::string::iterator first, std::string::iterator last)
{
    bool upperCase = true;
    for (; first != last; ++first)
    {
        if (upperCase)
        {
            *first = static_cast<char>(toupper(*first));
            upperCase = false;
        }
        else
        {
            if (*first == '\\')
            {
                upperCase = true;
            }
            else
            {
                *first = static_cast<char>(tolower(*first));
            }
        }
    }
}

bool ExpandShortPath(std::string& path)
{
    std::wstring widePath(utf8::ToUtf16(path));
    wchar_t buffer[MAX_PATH];
    if (::GetLongPathNameW(widePath.c_str(), buffer, MAX_PATH) == 0)
    {
        return false;
    }
    else
    {
        path = utf8::ToUtf8(buffer);
        return true;
    }
}
std::string ExpandEnvStrings(std::string const& input)
{
    std::wstring wideInput(utf8::ToUtf16(input));
    std::wstring result;
    DWORD errorCheck = static_cast<DWORD>(input.size());
    do
    {
        result.resize(static_cast<std::size_t>(errorCheck));
        errorCheck = ::ExpandEnvironmentStringsW(
            wideInput.c_str(), &result[0], static_cast<DWORD>(result.size()));
    } while (errorCheck != 0 && errorCheck != result.size());
    if (errorCheck == 0)
    {
        using namespace Instalog::SystemFacades;
        Win32Exception::ThrowFromLastError();
    }
    result.resize(static_cast<std::size_t>(errorCheck) - 1);
    return utf8::ToUtf8(result);
}

}
}
