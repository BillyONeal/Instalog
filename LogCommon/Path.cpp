#include "pch.hpp"
#include "Path.hpp"
#include <boost/algorithm/string.hpp>
#include "File.hpp"

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

	static void NativePathToWin32Path(std::wstring &path)
	{
		// Remove \, ??\, \?\, and globalroot\ 
		std::wstring::iterator chop = path.begin();
		if (boost::istarts_with(boost::make_iterator_range(chop, path.end()), L"\\")) { chop += 1; }
		if (boost::istarts_with(boost::make_iterator_range(chop, path.end()), L"??\\")) { chop += 3; }
		if (boost::istarts_with(boost::make_iterator_range(chop, path.end()), L"\\?\\")) { chop += 3; }
		if (boost::istarts_with(boost::make_iterator_range(chop, path.end()), L"globalroot\\")) { chop += 11; }		
		path.erase(path.begin(), chop);

		if (boost::istarts_with(path, L"system32\\"))
		{
			wchar_t windir[MAX_PATH];
			UINT len = ::GetWindowsDirectoryW(windir, MAX_PATH);
			windir[len] = L'\\';
			path.insert(0, windir, len + 1);
		}
		else if (boost::istarts_with(path, L"systemroot\\"))
		{
			wchar_t windir[MAX_PATH];
			UINT len = ::GetWindowsDirectoryW(windir, MAX_PATH);
			windir[len] = L'\\';
			path.replace(0, 11, windir, len + 1);
		}
	}

	static std::vector<std::wstring> getSplitPath()
	{
		using namespace std::placeholders;
		std::vector<std::wstring> splitPath;
		wchar_t pathBuf[32767] = L""; // 32767 is max size of environment variable
		UINT len = ::GetEnvironmentVariableW(L"PATH", pathBuf, 32767);
		boost::split(splitPath, pathBuf, std::bind(std::equal_to<wchar_t>(), _1, L';'));
		return splitPath;
	}

	static std::vector<std::wstring> getSplitPathExt()
	{
		using namespace std::placeholders;
		std::vector<std::wstring> splitPathExt;
		wchar_t pathExtBuf[32767] = L""; // 32767 is max size of environment variable
		UINT len = ::GetEnvironmentVariableW(L"PATHEXT", pathExtBuf, 32767);
		boost::split(splitPathExt, pathExtBuf, std::bind(std::equal_to<wchar_t>(), _1, L';'));
		return splitPathExt;
	}

	static bool TryExtensions( std::wstring &searchpath, std::wstring::iterator extensionat ) 
	{
		static std::vector<std::wstring> splitPathExt = getSplitPathExt();

		// Search with no path extension
		if (SystemFacades::File::Exists(std::wstring(searchpath.begin(), extensionat))) 
		{
			searchpath.erase(extensionat, searchpath.end());
			return true;
		}

		// Try the available path extensions
		for (std::vector<std::wstring>::iterator splitPathExtIt = splitPathExt.begin(); splitPathExtIt != splitPathExt.end(); ++splitPathExtIt)
		{
			if (SystemFacades::File::Exists(std::wstring(searchpath.begin(), extensionat).append(*splitPathExtIt))) 
			{
				searchpath.replace(extensionat, searchpath.end(), splitPathExtIt->begin(), splitPathExtIt->end());
				searchpath.erase(extensionat + splitPathExtIt->size(), searchpath.end());
				return true;
			}
		}

		return false;
	}
	
	static void StripArgumentsFromPath(std::wstring &path)
	{
		// Just try the file
		if (TryExtensions(path, path.end()))
			return;

		// For each spot where there's a space, try all the extensions
		for (std::wstring::iterator subpath = std::find(path.begin(), path.end(), L' '); subpath != path.end(); subpath = std::find(subpath, path.end(), L' '))
		{
			if (TryExtensions(path, subpath))
				return;
		}

		// Prepend the path with each path from %PATH% and try all the extensions
		static std::vector<std::wstring> splitPath = getSplitPath();
		for (std::vector<std::wstring>::iterator splitPathIt = splitPath.begin(); splitPathIt != splitPath.end(); ++splitPathIt)
		{
			std::wstring longpath(*splitPathIt);
			Append(longpath, path);
			if (TryExtensions(longpath, longpath.end()))
			{
				path = longpath;
				return;
			}
		}
	}

	void ResolveFromCommandLine(std::wstring &path)
	{
		NativePathToWin32Path(path);
		StripArgumentsFromPath(path);
	}

}}