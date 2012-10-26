// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <unordered_set>
#include <boost/algorithm/string.hpp>
#include "File.hpp"
#include "StringUtilities.hpp"
#include "Win32Exception.hpp"
#include "Path.hpp"

namespace Instalog { namespace Path {

	std::wstring Append( std::wstring path, std::wstring const& more )
	{
		if (path.size() == 0 && more.size() == 0)
		{
			return L"";
		}
		else if (path.size() > 0 && more.size() == 0)
		{
			return path;
		}
		else if (path.size() == 0 && more.size() > 0)
		{
			return more;
		}
		else
		{
			std::wstring::const_iterator pathend = path.end() - 1;
			std::wstring::const_iterator morebegin = more.begin();

			if (*pathend == L'\\' && *morebegin == L'\\')
			{
				return path.append(++morebegin, more.end());
			}
			else if (*pathend == L'\\' || *morebegin == L'\\')
			{
				return path.append(more);
			}
			else
			{
				path.push_back(L'\\');
				return path.append(more);
			}
		}
	}

	std::wstring GetWindowsPath()
	{
		wchar_t windir[MAX_PATH];
		UINT len = ::GetWindowsDirectoryW(windir, MAX_PATH);
		windir[len++] = L'\\';
		return std::wstring(windir, len);
	}

	static void NativePathToWin32Path(std::wstring &path)
	{
		// Remove \, ??\, \?\, and globalroot\ 
		std::wstring::iterator chop = path.begin();
		if (boost::starts_with(boost::make_iterator_range(chop, path.end()), L"\\")) { chop += 1; }
		if (boost::starts_with(boost::make_iterator_range(chop, path.end()), L"??\\")) { chop += 3; }
		if (boost::starts_with(boost::make_iterator_range(chop, path.end()), L"\\?\\")) { chop += 3; }
		if (boost::istarts_with(boost::make_iterator_range(chop, path.end()), L"globalroot\\")) { chop += 11; }		
		path.erase(path.begin(), chop);

		static std::wstring windowsDirectory = GetWindowsPath();
		if (boost::istarts_with(path, L"system32\\"))
		{
			path.insert(0, windowsDirectory);
		}
		else if (boost::istarts_with(path, L"systemroot\\"))
		{
			path.replace(0, 11, windowsDirectory);
		}
		else if (boost::istarts_with(path, L"%systemroot%\\")) // TODO: Move this somewhere else eventually
		{
			path.replace(0, 13, windowsDirectory);
		}
	}

	static std::vector<std::wstring> getSplitEnvironmentVariable(wchar_t const* variable)
	{
		using namespace std::placeholders;
		std::vector<std::wstring> splitVar;
		wchar_t buf[32767] = L""; // 32767 is max size of environment variable
		UINT len = ::GetEnvironmentVariableW(variable, buf, 32767);
		auto range = boost::make_iterator_range(buf, buf + len);
		boost::split(splitVar, range, std::bind1st(std::equal_to<wchar_t>(), L';'));
		return splitVar;
	}

	static std::vector<std::wstring> getSplitPath()
	{
		return getSplitEnvironmentVariable(L"PATH");
	}

	static std::vector<std::wstring> getSplitPathExt()
	{
		return getSplitEnvironmentVariable(L"PATHEXT");
	}

	static bool RundllCheck(std::wstring &path)
	{
		static std::wstring rundllpath = GetWindowsPath().append(L"System32\\rundll32");
		
		if (boost::istarts_with(path, rundllpath))
		{
			std::wstring::iterator firstComma = std::find(path.begin() + rundllpath.size(), path.end(), L',');
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

    static bool IsExclusiveFileCached(std::wstring const& testPath)
    {
        static std::unordered_set<std::wstring> nonexistentCache;
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

	static bool TryExtensions( std::wstring &searchpath, std::wstring::iterator extensionat ) 
	{
		static std::vector<std::wstring> splitPathExt = getSplitPathExt();
		
		// Try rundll32 check first
		if (RundllCheck(searchpath))
		{
			return true;
		}

		// Search with no path extension
		std::wstring pathNoPathExtension = std::wstring(searchpath.begin(), extensionat);
        if (IsExclusiveFileCached(pathNoPathExtension)) 
		{
			searchpath = pathNoPathExtension;
			return true;
		}
		auto pathNoExtensionSize = pathNoPathExtension.size();

		// Try the available path extensions
		for (auto splitPathExtIt = splitPathExt.cbegin(); splitPathExtIt != splitPathExt.cend(); ++splitPathExtIt)
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

	static bool TryExtensionsAndPaths( std::wstring &path, std::wstring::iterator spacelocation ) 
	{
		// First, try all of the available extensions
		if (TryExtensions(path, spacelocation))
			return true;

		// Second, don't bother trying path prefixes if we start with a drive
		if (path.size() >= 2 && iswalpha(path[0]) && path[1] == L':')
			return false;

		// Third, try to prepend it with each path in %PATH% and try each extension
		static std::vector<std::wstring> splitPath = getSplitPath();
		for (std::vector<std::wstring>::iterator splitPathIt = splitPath.begin(); splitPathIt != splitPath.end(); ++splitPathIt)
		{
			std::wstring longpath = Path::Append(*splitPathIt, std::wstring(path.begin(), path.end()));
			std::wstring::iterator longpathspacelocation = longpath.end() - std::distance(spacelocation, path.end());
			if (TryExtensions(longpath, longpathspacelocation))
			{
				path = longpath;
				return true;
			}
		}

		return false;
	}
	
	static bool StripArgumentsFromPath(std::wstring &path)
	{
		auto subpath = path.begin();
		// For each spot where there's a space, try all available extensions
		do
		{
			subpath = std::find(subpath + 1, path.end(), L' ');
			if (TryExtensionsAndPaths(path, subpath))
			{
				return true;
			}
		} while (subpath != path.end());

		return false;
	}

	static std::wstring GetRundll32Path()
	{
		return Path::Append(GetWindowsPath(), L"System32\\Rundll32.exe");
	}

	bool ResolveFromCommandLine(std::wstring &path)
	{
		if (path.empty())
		{
            return false;
        }
        path = Path::ExpandEnvStrings(path);

        if (path[0] == L'\"')
        {
            std::wstring unescaped;
            unescaped.reserve(path.size());
            std::wstring::iterator endOfUnescape = CmdLineToArgvWUnescape(path.begin(), path.end(), std::back_inserter(unescaped));
            if (boost::istarts_with(unescaped, GetRundll32Path()))
            {
                std::wstring::iterator startOfArgument = std::find(endOfUnescape, path.end(), L'\"');
                if (startOfArgument != path.end())
                {
                    unescaped.push_back(L' ');
                    CmdLineToArgvWUnescape(startOfArgument, path.end(), std::back_inserter(unescaped)); // Unescape the argument
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

	void Prettify(std::wstring::iterator first, std::wstring::iterator last)
	{
		bool upperCase = true;
		for(; first != last; ++first)
		{
			if (upperCase)
			{
				*first = towupper(*first);
				upperCase = false;
			}
			else
			{
				if (*first == L'\\')
				{
					upperCase = true;
				}
				else
				{
					*first = towlower(*first);
				}
			}
		}
	}

	bool ExpandShortPath( std::wstring &path )
	{
		if (SystemFacades::File::Exists(path))
		{
			wchar_t buffer[MAX_PATH];
			::GetLongPathNameW(path.c_str(), buffer, MAX_PATH);
			path = std::wstring(buffer);

			return true;
		}

		return false;
	}
    std::wstring ExpandEnvStrings( std::wstring const& input )
    {
        std::wstring result;
        DWORD errorCheck = static_cast<DWORD>(input.size());
        do
        {
            result.resize(static_cast<std::size_t>(errorCheck));
            errorCheck = ::ExpandEnvironmentStringsW(input.c_str(), &result[0], static_cast<DWORD>(result.size()));
        } while (errorCheck != 0 && errorCheck != result.size());
        if (errorCheck == 0)
        {
            using namespace Instalog::SystemFacades;
            Win32Exception::ThrowFromLastError();
        }
        result.resize(static_cast<std::size_t>(errorCheck) - 1);
        return std::move(result);
    }

}}
