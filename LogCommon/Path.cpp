// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <unordered_set>
#include <limits>
#include <cstdlib>
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
    std::vector<std::string> splitPath = getSplitPath();
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
        return SystemFacades::File::IsExclusiveFile(path);
    }
    else
    {
        NativePathToWin32Path(path);
        bool status = StripArgumentsFromPath(path);
        if (status)
        {
            ExpandShortPath(path);
        }
        return status;
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

} // Instalog::Path

static std::size_t path_buffer_size_for_characters(std::size_t characterCount)
{
    // 1x characterCount = source text
    // 1 null
    // 1x characterCount = uppercase source text
    // 1 null
    return characterCount * 2 + 2;
}

static void convert_ntfs_upper(wchar_t const* const input, std::size_t const length, wchar_t* const output)
{
    int inputLength = static_cast<int>(length);
    if (static_cast<std::size_t>(inputLength) != length)
    {
        std::terminate();
    }

    if (::LCMapStringW(LOCALE_INVARIANT, LCMAP_UPPERCASE, input, inputLength, output, inputLength) == 0)
    {
        std::terminate();
    }

    output[length] = L'\0';
}

wchar_t* path::get_upper_ptr() BOOST_NOEXCEPT_OR_NOTHROW
{
    return this->buffer.get() + this->actualCapacity + 1;
}

wchar_t const* path::get_upper_ptr() const BOOST_NOEXCEPT_OR_NOTHROW
{
    return this->buffer.get() + this->actualCapacity + 1;
}

void path::construct(char const* const buffer, std::size_t length)
{
    std::size_t u16Length = 0;
    char const* const end = buffer + length;
    for (char const* current = buffer; current != end;)
    {
        uint32_t cp = utf8::next(current, end);
        if (cp > 0xFFFFu)
        {
            // a surrogate pair
            u16Length += 2;
        }
        else
        {
            ++u16Length;
        }
    }

    this->set_sizes_to(u16Length);
    auto const bufferSize = path_buffer_size_for_characters(u16Length);
    this->buffer.reset(new wchar_t[bufferSize]);
    wchar_t * lowerEnd = utf8::utf8to16(buffer, end, this->buffer.get());
    *lowerEnd = L'\0';

    this->construct_upper();
}

void path::set_sizes_to(std::size_t const size)
{
    std::size_t const intMax = std::numeric_limits<int>::max();
    if (size > intMax)
    {
        std::terminate();
    }

    int actualSet = static_cast<int>(size);
    this->actualSize = actualSet;
    this->actualCapacity = actualSet;
}

void path::construct(wchar_t const* const buffer, std::size_t length)
{
    this->set_sizes_to(length);
    auto const bufferSize = path_buffer_size_for_characters(length);
    this->buffer.reset(new wchar_t[bufferSize]);
    wchar_t* lowerEnd = std::copy_n(buffer, length, this->buffer.get());
    *lowerEnd = L'\0';
    this->construct_upper();
}

void path::construct_upper()
{
    // Assumes that the internal buffer has the normal case part
    // of the buffer filled out; fills in the upper case part.
    convert_ntfs_upper(this->get(), this->size(), this->get_upper_ptr());
}

path::path() BOOST_NOEXCEPT_OR_NOTHROW : buffer(nullptr), actualSize(0), actualCapacity(0)
{}

path::path(std::nullptr_t) BOOST_NOEXCEPT_OR_NOTHROW : buffer(nullptr), actualSize(0), actualCapacity(0)
{}

path::path(char const* sourcePath)
{
    this->construct(sourcePath, std::strlen(sourcePath));
}

path::path(wchar_t const* sourcePath)
{
    this->construct(sourcePath, std::wcslen(sourcePath));
}

path::path(std::string const& sourcePath)
{
    this->construct(sourcePath.c_str(), sourcePath.size());
}

path::path(std::wstring const& sourcePath)
{
    this->construct(sourcePath.c_str(), sourcePath.size());
}

path::path(path const& other)
{
    std::size_t const otherSize = other.size();
    std::size_t const bufferSize = path_buffer_size_for_characters(otherSize);
    this->buffer.reset(new wchar_t[bufferSize]);
    this->set_sizes_to(otherSize);
    wchar_t* nullPointer = std::copy_n(other.get(), otherSize, this->buffer.get());
    *nullPointer = L'\0';
    nullPointer = std::copy_n(other.get_upper(), otherSize, this->get_upper_ptr());
    *nullPointer = L'\0';
}

path::path(path && other) BOOST_NOEXCEPT_OR_NOTHROW
: buffer(std::move(other.buffer))
, actualSize(other.actualSize)
, actualCapacity(other.actualCapacity)
{
    other.actualSize = 0u;
    other.actualCapacity = 0u;
}

path& path::operator=(path const& other)
{
    std::size_t const otherSize = other.size();
    std::size_t const bufferSize = path_buffer_size_for_characters(otherSize);
    // Note use of a seperate buffer rather than this->buffer to
    // support assign-to-self
    std::unique_ptr<wchar_t[]> newBuffer(new wchar_t[bufferSize]);
    wchar_t* nullPointer = std::copy_n(other.get(), otherSize, newBuffer.get());
    *nullPointer = L'\0';
    nullPointer = std::copy_n(other.get_upper(), otherSize, newBuffer.get() + otherSize + 1);
    *nullPointer = L'\0';

    // Ok, these are nothrow
    this->buffer = std::move(newBuffer);
    this->set_sizes_to(otherSize);

    return *this;
}

path& path::operator=(path && other) BOOST_NOEXCEPT_OR_NOTHROW
{
    this->buffer = std::move(other.buffer);
    auto const otherSize = other.actualSize;
    auto const otherCapacity = other.actualCapacity;
    other.set_sizes_to(0);
    this->actualSize = otherSize;
    this->actualCapacity = otherCapacity;
    return *this;
}

std::string path::to_string() const
{
    return utf8::ToUtf8(this->get(), this->size());
}

std::string path::to_upper_string() const
{
    return utf8::ToUtf8(this->get_upper(), this->size());
}

std::wstring path::to_wstring() const
{
    return std::wstring(this->get(), this->size());
}

std::wstring path::to_upper_wstring() const
{
    return std::wstring(this->get_upper(), this->size());;
}

static const wchar_t emptyString[] = L"";

wchar_t const* path::get() const BOOST_NOEXCEPT_OR_NOTHROW
{
    return this->empty() ? emptyString : this->buffer.get();
}

wchar_t const* path::get_upper() const BOOST_NOEXCEPT_OR_NOTHROW
{
    return this->empty() ? emptyString : this->get_upper_ptr();
}

path::size_type path::size() const BOOST_NOEXCEPT_OR_NOTHROW
{
    return this->actualSize;
}

path::size_type path::capacity() const BOOST_NOEXCEPT_OR_NOTHROW
{
    return this->actualCapacity;
}

path::size_type path::max_size() const BOOST_NOEXCEPT_OR_NOTHROW
{
    return std::numeric_limits<short>::max();
}

bool path::empty() const BOOST_NOEXCEPT_OR_NOTHROW
{
    return this->size() == 0;
}

void path::swap(path& other) BOOST_NOEXCEPT_OR_NOTHROW
{
    using std::swap;
    swap(buffer, other.buffer);
    swap(actualSize, other.actualSize);
    swap(actualCapacity, other.actualCapacity);
}

path::~path() BOOST_NOEXCEPT_OR_NOTHROW
{
    // delete this->buffer happens in unique_ptr destructor
}


} // Instalog
