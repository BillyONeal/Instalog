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

	static std::wstring getWindowsDirectory()
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
		if (boost::istarts_with(boost::make_iterator_range(chop, path.end()), L"\\")) { chop += 1; }
		if (boost::istarts_with(boost::make_iterator_range(chop, path.end()), L"??\\")) { chop += 3; }
		if (boost::istarts_with(boost::make_iterator_range(chop, path.end()), L"\\?\\")) { chop += 3; }
		if (boost::istarts_with(boost::make_iterator_range(chop, path.end()), L"globalroot\\")) { chop += 11; }		
		path.erase(path.begin(), chop);

		static std::wstring windowsDirectory = getWindowsDirectory();
		if (boost::istarts_with(path, L"system32\\"))
		{
			path.insert(0, windowsDirectory);
		}
		else if (boost::istarts_with(path, L"systemroot\\"))
		{
			path.replace(0, 11, windowsDirectory);
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

	static bool RundllCheck(std::wstring &path)
	{
		static std::wstring rundllpath = getWindowsDirectory().append(L"System32\\rundll32");
		
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

	static bool TryExtensions( std::wstring &searchpath, std::wstring::iterator extensionat ) 
	{
		static std::vector<std::wstring> splitPathExt = getSplitPathExt();
		
		// Try rundll32 check first
		if (RundllCheck(searchpath))
		{
			return true;
		}

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
				return true;
			}
		}

		return false;
	}

	static bool TryExtensionsAndPaths( std::wstring &path, std::wstring::iterator spacelocation ) 
	{
		// First, try all of the available extensions
		if (TryExtensions(path, spacelocation))
			return true;

		// Second, try to prepend it with each path in %PATH% and try each extension
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
		// For each spot where there's a space, try all available extensions
		for (std::wstring::iterator subpath = std::find(path.begin(), path.end(), L' '); subpath != path.end(); subpath = std::find(subpath + 1, path.end(), L' '))
		{
			if (TryExtensionsAndPaths(path, subpath))
			{
				return true;
			}
		}	

		// Try all available extensions for the whole path
		if (TryExtensionsAndPaths(path, path.end()))
		{
			return true;
		}

		return false;
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

	bool ResolveFromCommandLine(std::wstring &path)
	{
		NativePathToWin32Path(path);
		if (StripArgumentsFromPath(path))
		{
			Prettify(path.begin(), path.end());
			return !SystemFacades::File::IsDirectory(path);
		}

		return false;
	}

}}