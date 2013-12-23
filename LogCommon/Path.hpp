// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <cstddef>
#include <string>
#include <algorithm>
#include <boost/config.hpp>

namespace Instalog
{
namespace Path
{

/// @brief    Appends two paths together considering the middle backslash
///
/// @details    This isn't a very advanced method.  It only considers the last
/// character of path and the first character of more.
///
/// @param    [in,out] path    Left of the path.  This variable will be modified
/// to equal the final result.
/// @param    [in] more    The right of the path to be appended
std::string Append(std::string path, std::string const& more);

/// @brief    Expands a short windows path to the corresponding long version
///
/// @param    path    Full long path to the file
bool ExpandShortPath(std::string& path);

/// @brief    Resolve a path from the command line
///
/// @details This is intended to work in the same way as Windows does it
///
/// @param    [in,out]    path    Full pathname of the file.
///
/// @return    true if the path exists and is not a directory, false otherwise
bool ResolveFromCommandLine(std::string& path);

/// @brief    Gets the Windows path.
///
/// @details    Does not include the trailing backslash (e.g. "C:\Windows")
/// unless Windows is installed in the drive root
///
/// @return    The Windows path.
std::string GetWindowsPath();

/**
 * Expands environment strings.
 *
 * @param input The input string to expand environment variables inside.
 *
 * @return The string with environment strings expanded.
 */
std::string ExpandEnvStrings(std::string const& input);

}


class path
{
public:
    typedef std::size_t size_type;
    
    path() BOOST_NOEXCEPT_OR_NOTHROW;
    path(std::nullptr_t) BOOST_NOEXCEPT_OR_NOTHROW;
    path(char const* sourcePath);
    path(wchar_t const* sourcePath);
    path(std::string const& sourcePath);
    path(std::wstring const& sourcePath);
    path(path const& other);
    path(path && other) BOOST_NOEXCEPT_OR_NOTHROW;

    path& operator=(path const& other);
    path& operator=(path && other) BOOST_NOEXCEPT_OR_NOTHROW;

    std::string to_string() const;
    std::string to_upper_string() const;

    std::wstring to_wstring() const;
    std::wstring to_upper_wstring() const;

    wchar_t const* get() const BOOST_NOEXCEPT_OR_NOTHROW;
    wchar_t const* get_upper() const BOOST_NOEXCEPT_OR_NOTHROW;

    size_type size() const BOOST_NOEXCEPT_OR_NOTHROW;
    size_type capacity() const BOOST_NOEXCEPT_OR_NOTHROW;
    size_type max_size() const BOOST_NOEXCEPT_OR_NOTHROW;
    bool empty() const BOOST_NOEXCEPT_OR_NOTHROW;

    void swap(path& other) BOOST_NOEXCEPT_OR_NOTHROW;

    ~path() BOOST_NOEXCEPT_OR_NOTHROW;
private:
    void construct(char const* const buffer, std::size_t length);
    void construct(wchar_t const* const buffer, std::size_t length);
    void construct_upper() BOOST_NOEXCEPT_OR_NOTHROW;
    void set_sizes_to(size_type length) BOOST_NOEXCEPT_OR_NOTHROW;
    wchar_t* get_upper_ptr() BOOST_NOEXCEPT_OR_NOTHROW;
    wchar_t const* get_upper_ptr() const BOOST_NOEXCEPT_OR_NOTHROW;
    std::unique_ptr<wchar_t[]> buffer;
    int actualSize;
    int actualCapacity;
};

inline bool operator==(path const& lhs, path const& rhs) BOOST_NOEXCEPT_OR_NOTHROW
{
    auto const lhsSize = lhs.size();
    auto const rhsSize = rhs.size();
    auto const lhsUpper = lhs.get_upper();
    return lhsSize == rhsSize && std::equal(lhsUpper, lhsUpper + lhsSize, rhs.get_upper());
}

inline bool operator!=(path const& lhs, path const& rhs) BOOST_NOEXCEPT_OR_NOTHROW
{
    return !(lhs == rhs);
}

inline bool operator<(path const& lhs, path const& rhs) BOOST_NOEXCEPT_OR_NOTHROW
{
    auto const lhsSize = lhs.size();
    auto const lhsBegin = lhs.get_upper();
    auto const lhsEnd = lhsBegin + lhsSize;
    auto const rhsSize = rhs.size();
    auto const rhsBegin = rhs.get_upper();
    auto const rhsEnd = rhsBegin + rhsSize;
    return std::lexicographical_compare(lhsBegin, lhsEnd, rhsBegin, rhsEnd);
}

inline bool operator>(path const& lhs, path const& rhs) BOOST_NOEXCEPT_OR_NOTHROW
{
    return rhs < lhs;
}

inline bool operator<=(path const& lhs, path const& rhs) BOOST_NOEXCEPT_OR_NOTHROW
{
    return !(lhs > rhs);
}

inline bool operator>=(path const& lhs, path const& rhs) BOOST_NOEXCEPT_OR_NOTHROW
{
    return !(lhs < rhs);
}

inline void swap(path& lhs, path& rhs) BOOST_NOEXCEPT_OR_NOTHROW
{
    lhs.swap(rhs);
}

}
