// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <boost/config.hpp>
#include <boost/noncopyable.hpp>
#include <windows.h>
#include "Expected.hpp"

namespace Instalog
{
namespace SystemFacades
{

/// <summary>Represents a plain Win32 file handle.</summary>
class File : boost::noncopyable
{
    HANDLE hFile;

    public:
    /// <summary>Default constructor.</summary>
    File();

    /// <summary>Constructor. Calls <c>CreateFileW</c>.</summary>
    /// <param name="filename">Filename passed to <c>CreateFileW</c>.</param>
    /// <param name="desiredAccess">(Optional) the desired access.</param>
    /// <param name="shareMode">(Optional) the share mode.</param>
    /// <param name="securityAttributes">(Optional) the security
    /// attributes.</param>
    /// <param name="createdDisposition">(Optional) the created
    /// disposition.</param>
    /// <param name="flags">(Optional) the flags.</param>
    /// <exception cref="Win32Exception">Thrown when the underlying
    /// <c>CreateFileW</c>
    /// call fails.</exception>
    File(std::string const& filename,
         DWORD desiredAccess = GENERIC_READ,
         DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE,
         LPSECURITY_ATTRIBUTES securityAttributes = nullptr,
         DWORD createdDisposition = OPEN_EXISTING,
         DWORD flags = FILE_ATTRIBUTE_NORMAL);

    /// <summary>Move constructor.</summary>
    /// <param name="other">[in,out] The file to move from.</param>
    File(File&& other) BOOST_NOEXCEPT_OR_NOTHROW;

    /// <summary>Assignment operator.</summary>
    /// <param name="other">The other file handle.</param>
    /// <returns>*this</returns>
    /// <exception cref="Win32Exception">Thrown when the underlying
    /// <c>CreateFileW</c>
    /// call fails.</exception>
    File& operator=(File other);

    /// <summary>Destructor.</summary>
    ~File();

    /// <summary>Gets the size of the file in bytes.</summary>
    /// <returns>The size of the file in bytes.</returns>
    /// <exception cref="Win32Exception">Thrown when the underlying
    /// <c>GetFileInformationByHandle</c>
    /// call fails.</exception>
    std::uint64_t GetSize() const;

    /// <summary>Gets the attributes of the file as a DWORD.</summary>
    /// <returns>The attributes of the file as a DWORD.</returns>
    /// <exception cref="Win32Exception">Thrown when the underlying
    /// <c>GetFileInformationByHandle</c>
    /// call fails.</exception>
    DWORD GetAttributes() const;

    /// <summary>Gets extended attributes of the file.</summary>
    /// <returns>The extended attributes of the file.</returns>
    /// <exception cref="Win32Exception">Thrown when the underlying
    /// <c>GetFileInformationByHandle</c>
    /// call fails.</exception>
    BY_HANDLE_FILE_INFORMATION GetExtendedAttributes() const;

    /// <summary>Reads bytes from the file.</summary>
    /// <param name="bytesToRead">The number of bytes to read from the
    /// file.</param>
    /// <returns>The bytes read from the file.</returns>
    /// <exception cref="Win32Exception">Thrown when the underlying
    /// <c>ReadFile</c> call
    /// fails.</exception>
    std::vector<char> ReadBytes(unsigned int bytesToRead) const;

    /// <summary>Writes bytes to the file.</summary>
    /// <param name="bytes">The number of bytes to write to the file.</param>
    /// <returns>true if the file completely wrote.</returns>
    /// <exception cref="Win32Exception">Thrown when the underlying
    /// <c>WriteFile</c> call
    /// fails.</exception>
    bool WriteBytes(std::vector<char> const& bytes);

    std::vector<std::string> ReadAllLines() const;

    /// <summary>Gets the size of a file without opening the file.</summary>
    /// <param name="filename">Filename of the file.</param>
    /// <returns>The size of the file.</returns>
    /// <exception cref="Win32Exception" />
    static std::uint64_t GetSize(std::string const& filename);

    /// <summary>Gets the attributes of the specified file as a DWORD without
    /// opening a file handle</summary>
    /// <param name="filename">Filename of the file.</param>
    /// <returns>The attributes.</returns>
    /// <exception cref="Win32Exception" />
    static DWORD GetAttributes(std::string const& filename);

    /// <summary>Gets the extended attributes of the specified file without
    /// opening a file handle</summary>
    /// <param name="filename">Filename of the file.</param>
    /// <returns>The extended attributes.</returns>
    /// <exception cref="Win32Exception" />
    static WIN32_FILE_ATTRIBUTE_DATA
    GetExtendedAttributes(std::string const& filename);

    /// <summary>Deletes the given file.</summary>
    /// <param name="filename">Filename of the file.</param>
    static void Delete(std::string const& filename);

    /// <summary>Determines if the file exists.</summary>
    /// <param name="filename">Filename of the file.</param>
    /// <returns>true if the file appears to exist; otherwise, false.</returns>
    static bool Exists(std::string const& filename);

    /// <summary>Determines if the given file is a directory without opening a
    /// file handle</summary>
    /// <param name="filename">Filename of the file.</param>
    /// <returns>true if the file is a directory; otherwise, false.</returns>
    static bool IsDirectory(std::string const& filename);

    /// <summary>Determines if the given file is exclusively a file -- that is,
    /// it is a file, not a directory, and that it exists.</summary>
    /// <param name="fileName">Filename of the file.</param>
    /// <returns>true the file name is an exclusive file; otherwise,
    /// false.</returns>
    static bool IsExclusiveFile(std::string const& fileName);

    /// <summary>Queries is executable.</summary>
    /// <param name="filename">Filename of the file.</param>
    /// <returns>true if executable; otherwise, false.</returns>
    static bool IsExecutable(std::string const& filename);

    /// <summary>Gets the company of the given file.</summary>
    /// <param name="target">File name of the file to check.</param>
    /// <returns>The company.</returns>
    static std::string GetCompany(std::string const& target);
};

/// <summary>Find files record.</summary>
class FindFilesRecord
{
    std::string cFileName;
    std::uint64_t ftCreationTime;
    std::uint64_t ftLastAccessTime;
    std::uint64_t ftLastWriteTime;
    std::uint64_t nFileSize;
    DWORD dwFileAttributes;

    public:
    /// <summary>Initializes a new instalce of the <c>FindFilesRecord</c>
    /// class.</summary>
    /// <param name="prefix">The prefix path.</param>
    /// <param name="winSource">The windows data record source.</param>
    FindFilesRecord(std::string prefix, WIN32_FIND_DATAW const& winSource);

    /// <summary>Copy constructor.</summary>
    /// <param name="other">The object to copy.</param>
    FindFilesRecord(FindFilesRecord const& other);

    /// <summary>Move constructor.</summary>
    /// <param name="other">[in,out] The other.</param>
    FindFilesRecord(FindFilesRecord&& other) BOOST_NOEXCEPT_OR_NOTHROW;

    /// <summary>Assignment operator.</summary>
    /// <param name="other">The other.</param>
    /// <returns>*this.</returns>
    FindFilesRecord& operator=(FindFilesRecord other);

    /// <summary>Gets file name.</summary>
    /// <returns>The file name.</returns>
    std::string const& GetFileName() const BOOST_NOEXCEPT_OR_NOTHROW;

    /// <summary>Gets creation time.</summary>
    /// <returns>The creation time.</returns>
    std::uint64_t GetCreationTime() const BOOST_NOEXCEPT_OR_NOTHROW;

    /// <summary>Gets the last access time.</summary>
    /// <returns>The last access time.</returns>
    std::uint64_t GetLastAccessTime() const BOOST_NOEXCEPT_OR_NOTHROW;

    /// <summary>Gets the last write time.</summary>
    /// <returns>The last write time.</returns>
    std::uint64_t GetLastWriteTime() const BOOST_NOEXCEPT_OR_NOTHROW;

    /// <summary>Gets the size.</summary>
    /// <returns>The size.</returns>
    std::uint64_t GetSize() const BOOST_NOEXCEPT_OR_NOTHROW;

    /// <summary>Gets the attributes.</summary>
    /// <returns>The attributes.</returns>
    DWORD GetAttributes() const BOOST_NOEXCEPT_OR_NOTHROW;

    /// <summary>Swaps this instance with another
    /// <c>FindFilesRecord</c>.</summary>
    /// <param name="other">[in,out] The instance with which this instance is
    /// swapped.</param>
    void swap(FindFilesRecord& other) BOOST_NOEXCEPT_OR_NOTHROW;
};

/// <summary>Swaps a pair of <c>FindFilesRecord</c> instances.</summary>
/// <param name="lhs">[in,out] The left hand side.</param>
/// <param name="rhs">[in,out] The right hand side.</param>
inline void swap(FindFilesRecord& lhs, FindFilesRecord& rhs) BOOST_NOEXCEPT_OR_NOTHROW
{
    lhs.swap(rhs);
}

class FindHandle;

/// <summary>Options controlling a search for files.</summary>
enum class FindFilesOptions : unsigned char
{
    LocalSearch = 0,
    RecursiveSearch = 1,
    IncludeDotDirectories = 2
};

/// <summary>Bitwise 'or' operator.</summary>
/// <param name="lhs">A bitfield to process.</param>
/// <param name="rhs">One or more bits to OR into the bitfield.</param>
/// <returns>The result of the operation.</returns>
inline FindFilesOptions operator|(FindFilesOptions lhs, FindFilesOptions rhs)
{
    return static_cast<FindFilesOptions>(static_cast<unsigned char>(lhs) |
                                         static_cast<unsigned char>(rhs));
}

class FindFiles : boost::noncopyable
{
    std::vector<FindHandle> handleStack;
    std::string prefix;
    std::string pattern;
    DWORD lastError;
    WIN32_FIND_DATAW findData;
    FindFilesOptions options;
    bool IsRecursive() const BOOST_NOEXCEPT_OR_NOTHROW;
    bool IncludingDotDirectories() const BOOST_NOEXCEPT_OR_NOTHROW;
    bool CanEnter() const BOOST_NOEXCEPT_OR_NOTHROW;
    bool LastSuccess() const BOOST_NOEXCEPT_OR_NOTHROW;
    void Leave();
    void WinEnter();
    void WinNext();
    void NextImpl();
    bool OnEndShouldLeave() BOOST_NOEXCEPT_OR_NOTHROW;
    bool OnDotKeepGoing() BOOST_NOEXCEPT_OR_NOTHROW;
    void Construct(std::string const& pattern);

    public:
    /// <summary>Default constructor. Operates as a successful empty file search
    /// result.</summary>
    FindFiles() BOOST_NOEXCEPT_OR_NOTHROW;

    /// <summary>Constructor. Initiates a file search with the default options
    /// of nonrecursive, skipping dot directories.</summary>
    /// <param name="pattern">Specifies the pattern for which the search is
    /// conducted.</param>
    FindFiles(std::string const& pattern);

    /// <summary>Constructor. Initiates a file search.</summary>
    /// <param name="pattern">Specifies the pattern for which the search is
    /// conducted.</param>
    /// <param name="options">Options for controlling the search.</param>
    FindFiles(std::string const& pattern, FindFilesOptions options);

    /// <summary>Move constructor.</summary>
    /// <param name="toMove">[in,out] The instance from which move construction
    /// occurs. The move-
    /// constructed instance is placed in the default constructed state.</param>
    FindFiles(FindFiles&& toMove) BOOST_NOEXCEPT_OR_NOTHROW;

    /// <summary>Move assignment operator.</summary>
    /// <param name="toMove">[in,out] The instance from which move assignment
    /// occurs. The move-
    /// assigned instance is placed in the default constructed state.</param>
    /// <returns>*this.</returns>
    FindFiles& operator=(FindFiles&& toMove) BOOST_NOEXCEPT_OR_NOTHROW;

    /// <summary>Destructor.</summary>
    ~FindFiles() BOOST_NOEXCEPT_OR_NOTHROW;

    /// <summary>Swaps this instance with another FindFiles instance.</summary>
    /// <param name="other">[in,out] The other.</param>
    void Swap(FindFiles& other) BOOST_NOEXCEPT_OR_NOTHROW;

    /// <summary>Advances this instance to the next record.</summary>
    /// <returns>true if it succeeds, false if it fails.</returns>
    bool Next();

    /// <summary>Advances this instance to the next successful record.</summary>
    /// <returns>true if it succeeds, false if it fails.</returns>
    bool NextSuccess();

    /// <summary>Gets the last error encountered in a previous call to one of
    /// the advancing
    /// functions. Typically <c>ERROR_SUCCESS</c> when the last advance was
    /// successful, or
    /// <c>ERROR_NO_MORE_FILES</c> at the end of a search.</summary>
    /// <returns>The last error code encountered.</returns>
    DWORD LastError() const BOOST_NOEXCEPT_OR_NOTHROW;

    /// <summary>Gets the current record, or throws a <see cref="Win32Exception"
    /// /> if no successful record is available.</summary>
    /// <returns>The current record.</returns>
    /// <exception cref="Win32Exception" />
    FindFilesRecord GetRecord() const;

    /// <summary>Gets the current record, or an expected containing an exception
    /// if there was no successful record.</summary>
    /// <returns>The current record.</returns>
    expected<FindFilesRecord> TryGetRecord() const BOOST_NOEXCEPT_OR_NOTHROW;
};

/// <summary>Swaps a pair of FindFiles instances.</summary>
/// <param name="lhs">[in,out] The left hand side.</param>
/// <param name="rhs">[in,out] The right hand side.</param>
inline void swap(FindFiles& lhs, FindFiles& rhs) BOOST_NOEXCEPT_OR_NOTHROW
{
    lhs.Swap(rhs);
}
}
}
