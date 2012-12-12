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

    __declspec(noreturn) static inline void throw_length_error()
    {
        throw std::length_error("Path maximum length exceeded.");
    }

    //
    // Path uses a memory block like the following:
    // +---------------------+----+-------+-----------------------+----+-------+
    // | Display path string | \0 | Empty | Uppercase path string | \0 | Empty |
    // +---------------------+----+-------+-----------------------+----+-------+
    //  | <-    size()   -> |  1
    //  | <-        capacity() + 1       |
    //                                     | <-    size()     -> |  1
    //                                     | <-        capacity() + 1      -> |
    //  | <-                   capacity() * 2 + 2                          -> |

    /**
     * Converts a given size of a path into the size of the memory block required to hold a path of that size.
     */
    static inline std::size_t capacity_to_memory_capacity(std::size_t capacity)
    {
        return 2 * capacity + 2;
    }

    /*
     * Converts a memory buffer to upper case.
     *
     * @remarks Right now this is Win32 specific.
     */
    void path::uppercase_range(path::size_type length, path::const_pointer start, path::pointer target)
    {
        if (length >= std::numeric_limits<INT>::max())
        {
            // Can't happen. But in case it does....
            std::terminate();
        }

        int asInt = static_cast<int>(length);
        auto result = ::LCMapStringW(LOCALE_INVARIANT, LCMAP_UPPERCASE, source, asInt, target, asInt);
        if (!result)
        {
            assert(!"Something, somewhere, when horribly wrong");
        }
    }

    path::path() throw()
        : base_(nullptr)
        , size_(0)
        , capacity_(0)
    { }

    path::path(path const& other)
        : capacity_(other.size())
        , size_(other.size())
        , base_(new wchar_t[capacity_to_memory_capacity(this->capacity_)])
    {
        auto uppercaseBegin = other.upperBase();
        // +1 for null terminator
        std::copy(other.base_, other.base_ + other.size_ + 1, this->base_);
        std::copy(uppercaseBegin, uppercaseBegin + other.size_ + 1, this->upperBase());
    }

    path::path(path && other) throw()
        : base_(other.base_)
        , size_(other.size_)
        , capacity_(other.capacity_)
    {
        other.base_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    path& path::operator=(path other)
    {
        other.swap(*this);
        return *this;
    }

    path::iterator path::begin() throw()
    {
        return this->base_;
    }

    path::const_iterator path::begin() const throw()
    {
        return this->base_;
    }

    path::const_iterator path::cbegin() const throw()
    {
        return this->base_;
    }

    path::iterator path::end() throw()
    {
        return this->base_ + this->size();
    }

    path::const_iterator path::end() const throw()
    {
        return this->base_ + this->size();
    }

    path::const_iterator path::cend() const throw()
    {
        return this->base_ + this->size();
    }

    void path::swap(path& other) throw()
    {
        using std::swap;
        swap(this->base_, other.base_);
        swap(this->size_, other.size_);
        swap(this->capacity_, other.capacity_);
    }

    path::size_type path::size() const throw()
    {
        return this->size_;
    }

    path::size_type path::max_size() const throw()
    {
        // See http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247.aspx
        // The Windows API has many functions that also have Unicode versions to permit an
        // extended-length path for a maximum total path length of 32,767 characters. This
        // type of path is composed of components separated by backslashes, each up to the
        // value returned in the lpMaximumComponentLength parameter of the GetVolumeInformation
        // function (this value is commonly 255 characters). To specify an extended-length
        // path, use the "\\?\" prefix. For example, "\\?\D:\very long path".
        //
        // Note  The maximum path of 32,767 characters is approximate, because the "\\?\"
        // prefix may be expanded to a longer string by the system at run time, and this
        // expansion applies to the total length
        return 32767;
    }

    bool path::empty() const throw()
    {
        return this->size() == 0;
    }

    path::~path() throw()
    {
        if (this->base_ != nullptr)
        {
            delete [] this->base_;
        }
    }

    path::reverse_iterator path::rbegin() throw()
    {
        return path::reverse_iterator(this->end());
    }

    path::reverse_const_iterator path::rbegin() const throw()
    {
        return path::reverse_const_iterator(this->end());
    }

    path::reverse_const_iterator path::crbegin() const throw()
    {
        return path::reverse_const_iterator(this->end());
    }

    path::reverse_iterator path::rend() throw()
    {
        return path::reverse_iterator(this->begin());
    }

    path::reverse_const_iterator path::rend() const throw()
    {
        return path::reverse_const_iterator(this->begin());
    }

    path::reverse_const_iterator path::crend() const throw()
    {
        return path::reverse_const_iterator(this->begin());
    }

    path::iterator path::ubegin() throw()
    {
        return this->upperBase();
    }

    path::const_iterator path::ubegin() const throw()
    {
        return this->upperBase();
    }

    path::const_iterator path::cubegin() const throw()
    {
        return this->upperBase();
    }

    path::iterator path::uend() throw()
    {
        return this->upperBase() + this->size();
    }

    path::const_iterator path::uend() const throw()
    {
        return this->upperBase() + this->size();
    }

    path::const_iterator path::cuend() const throw()
    {
        return this->upperBase() + this->size();
    }

    path::reverse_iterator path::rubegin() throw()
    {
        return path::reverse_iterator(this->uend());
    }

    path::reverse_const_iterator path::rubegin() const throw()
    {
        return path::reverse_const_iterator(this->uend());
    }

    path::reverse_const_iterator path::crubegin() const throw()
    {
        return path::reverse_const_iterator(this->uend());
    }

    path::reverse_iterator path::ruend() throw()
    {
        return path::reverse_iterator(this->ubegin());
    }

    path::reverse_const_iterator path::ruend() const throw()
    {
        return path::reverse_const_iterator(this->ubegin());
    }

    path::reverse_const_iterator path::cruend() const throw()
    {
        return path::reverse_const_iterator(this->ubegin());
    }

    path::reference_type path::ufront() throw()
    {
        assert(!this->empty());
        return *this->ubegin();
    }

    path::const_reference_type path::ufront() const throw()
    {
        assert(!this->empty());
        return *this->ubegin();
    }

    path::reference_type path::uback() throw()
    {
        assert(!this->empty());
        return *(this->uend() - 1);
    }

    path::const_reference_type path::uback() const throw()
    {
        assert(!this->empty());
        return *(this->uend() - 1);
    }

    void path::ensure_capacity(path::size_type desiredCapacity)
    {
        if (desiredCapacity > this->max_size())
        {
            throw_length_error();
        }

        auto newCapacity = std::max(desiredCapacity, this->capacity_ * 2);
        // Don't let capacity doubling exceed max_size
        newCapacity = std::min(newCapacity, this->max_size());

        this->reserve(newCapacity);
    }

    void path::reserve(size_type count)
    {
        if (count < this->capacity_)
        {
            return;
        }

        if (count >= this->max_size())
        {
            throw_length_error();
        }

        auto buffer = new wchar_t[count * 2 + 2];
        if (this->base_)
        {
            auto upperBegin = this->upperBase();
            std::copy(this->base_, this->base_ + this->size() + 1, buffer);
            std::copy(upperBegin, upperBegin + this->size() + 1, buffer + count + 1);
            delete [] this->base_;
        }

        this->base_ = buffer;
        this->capacity_ = count;
    }

    path::pointer path::upperBase() throw()
    {
        return this->base_ + this->capacity_ + 1;
    }

    path::const_pointer path::upperBase() const throw()
    {
        return this->base_ + this->capacity_ + 1;
    }

    path::pointer path::data() throw()
    {
        return this->base_;
    }

    path::const_pointer path::data() const throw()
    {
        return this->base_;
    }

    path::const_pointer path::c_str() const throw()
    {
        return this->base_;
    }

    path::path(wchar_t const* string)
        : capacity_(0)
        , base_(nullptr)
    {
        auto length = std::wcslen(string);
        this->size_ = length;
        this->ensure_capacity(length);

        auto upperStart = this->upperBase();
        std::copy(string, string + length + 1, this->base_);
        path::uppercase_range(this->size_, this->base_, upperStart);
        upperStart[this->size_] = L'\0';
    }

    path::path(std::wstring const& string)
        : capacity_(0)
        , base_(nullptr)
    {
        this->size_ = string.size();
        this->ensure_capacity(string.size());

        auto upperStart = this->upperBase();
        std::copy(string.cbegin(), string.cend(), this->base_);
        this->base_[this->size_] = L'\0';
        path::uppercase_range(this->size_, this->base_, upperStart);
        upperStart[this->size_] = L'\0';
    }

    void path::push_back(wchar_t character)
    {
        auto lastSize = this->size();
        this->ensure_capacity(lastSize + 1);
        auto upper = this->upperBase();
        this->base_[lastSize] = character;
        path::uppercase_range(1, this->base_ + lastSize, upper + lastSize);
        ++this->size_;
        this->base_[this->size()] = L'\0';
        upper[this->size()] = L'\0';
    }
    
    void path::pop_back() throw()
    {
        if (!this->empty())
        {
            this->base_[this->size() - 1] = L'\0';
            this->upperBase()[this->size() - 1] = L'\0';
            --this->size_;
        }
    }

    path::const_reference_type path::operator[](path::size_type index) const throw()
    {
        return this->base_[index];
    }

    path::const_reference_type path::at(path::size_type index) const
    {
        if (index >= this->size())
        {
            throw std::length_error("Reference to path using path::at() exceeded range.");
        }

        return this->base_[index];
    }
}}
