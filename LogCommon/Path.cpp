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
		static std::wstring rundllpath = getWindowsDirectory().append(L"System32/rundll32.exe");
		
		if (boost::iequals(rundllpath, path))
		{
			ResolveFromCommandLine(path);
			return true;
		}

		return false;
	}

	static bool TryExtensions( std::wstring &searchpath, std::wstring::iterator extensionat ) 
	{
		static std::vector<std::wstring> splitPathExt = getSplitPathExt();

		// Search with no path extension
		if (SystemFacades::File::Exists(std::wstring(searchpath.begin(), extensionat)))
		{
			// Check if it contains rundll32
			if (extensionat != searchpath.end())
			{
				std::wstring remainder(extensionat++, searchpath.end());
				if (RundllCheck(remainder))
				{
					searchpath = remainder;
					return SystemFacades::File::Exists(searchpath);
				}
			}

			// Doesn't contain rundll32
			searchpath.erase(extensionat, searchpath.end());
			return true;
		}

		// Try the available path extensions
		for (std::vector<std::wstring>::iterator splitPathExtIt = splitPathExt.begin(); splitPathExtIt != splitPathExt.end(); ++splitPathExtIt)
		{
			if (SystemFacades::File::Exists(std::wstring(searchpath.begin(), extensionat).append(*splitPathExtIt))) 
			{
				// Check if it contains rundll32
				if (extensionat != searchpath.end())
				{
					std::wstring remainder(extensionat++, searchpath.end());
					if (RundllCheck(remainder))
					{
						searchpath = remainder;
						return SystemFacades::File::Exists(searchpath);
					}
				}

				// Doesn't contain rundll32
				searchpath.replace(extensionat, searchpath.end(), splitPathExtIt->begin(), splitPathExtIt->end());
				searchpath.erase(extensionat + splitPathExtIt->size(), searchpath.end());
				return true;
			}
		}

		return false;
	}
	
	static bool StripArgumentsFromPath(std::wstring &path)
	{
		// Just try the file
		if (TryExtensions(path, path.end()))
			return true;

		// For each spot where there's a space, try all the extensions
		for (std::wstring::iterator subpath = std::find(path.begin(), path.end(), L' '); subpath != path.end(); subpath = std::find(subpath, path.end(), L' '))
		{
			if (TryExtensions(path, subpath))
				return true;
		}

		// Prepend the path with each path from %PATH% and try all the extensions
		static std::vector<std::wstring> splitPath = getSplitPath();
		for (std::vector<std::wstring>::iterator splitPathIt = splitPath.begin(); splitPathIt != splitPath.end(); ++splitPathIt)
		{
			std::wstring longpath(*splitPathIt);
			longpath.append(path);
			if (TryExtensions(longpath, longpath.end()))
			{
				path = longpath;
				return true;
			}
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