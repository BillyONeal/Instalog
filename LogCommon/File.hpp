// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <stack>
#include <boost/noncopyable.hpp>
#include <windows.h>
#include "Expected.hpp"

namespace Instalog { namespace SystemFacades {

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
        /// <param name="securityAttributes">(Optional) the security attributes.</param>
        /// <param name="createdDisposition">(Optional) the created disposition.</param>
        /// <param name="flags">(Optional) the flags.</param>
        /// <exception cref="Win32Exception">Thrown when the underlying <c>CreateFileW</c>
        /// call fails.</exception>
        File(
            std::wstring const& filename,
            DWORD desiredAccess = GENERIC_READ,
            DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE,
            LPSECURITY_ATTRIBUTES securityAttributes = nullptr,
            DWORD createdDisposition = OPEN_EXISTING,
            DWORD flags = FILE_ATTRIBUTE_NORMAL
        );

        /// <summary>Move constructor.</summary>
        /// <param name="other">[in,out] The file to move from.</param>
        File(File && other) throw();

        /// <summary>Assignment operator.</summary>
        /// <param name="other">The other file handle.</param>
        /// <returns>*this</returns>
        /// <exception cref="Win32Exception">Thrown when the underlying <c>CreateFileW</c>
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
        /// <param name="bytesToRead">The number of bytes to read from the file.</param>
        /// <returns>The bytes read from the file.</returns>
        /// <exception cref="Win32Exception">Thrown when the underlying <c>ReadFile</c> call
        /// fails.</exception>
        std::vector<char> ReadBytes(unsigned int bytesToRead) const;

        /// <summary>Writes bytes to the file.</summary>
        /// <param name="bytes">The number of bytes to write to the file.</param>
        /// <returns>true if the file completely wrote.</returns>
        /// <exception cref="Win32Exception">Thrown when the underlying <c>WriteFile</c> call
        /// fails.</exception>
        bool WriteBytes(std::vector<char> const& bytes); 

        /// <summary>Gets the size of a file without opening the file.</summary>
        /// <param name="filename">Filename of the file.</param>
        /// <returns>The size of the file.</returns>
        /// <exception cref="Win32Exception" />
        static std::uint64_t GetSize(std::wstring const& filename);

        /// <summary>Gets the attributes of the specified file as a DWORD without opening a file handle</summary>
        /// <param name="filename">Filename of the file.</param>
        /// <returns>The attributes.</returns>
        /// <exception cref="Win32Exception" />
        static DWORD GetAttributes(std::wstring const& filename);

        /// <summary>Gets the extended attributes of the specified file without opening a file handle</summary>
        /// <param name="filename">Filename of the file.</param>
        /// <returns>The extended attributes.</returns>
        /// <exception cref="Win32Exception" />
        static WIN32_FILE_ATTRIBUTE_DATA GetExtendedAttributes(std::wstring const& filename);

        /// <summary>Deletes the given file.</summary>
        /// <param name="filename">Filename of the file.</param>
        static void Delete(std::wstring const& filename);

        /// <summary>Determines if the file exists.</summary>
        /// <param name="filename">Filename of the file.</param>
        /// <returns>true if the file appears to exist; otherwise, false.</returns>
        static bool Exists(std::wstring const& filename);

        /// <summary>Determines if the given file is a directory without opening a file handle</summary>
        /// <param name="filename">Filename of the file.</param>
        /// <returns>true if the file is a directory; otherwise, false.</returns>
        static bool IsDirectory(std::wstring const& filename);

        /// <summary>Determines if the given file is exclusively a file -- that is, it is a file, not a directory, and that it exists.</summary>
        /// <param name="fileName">Filename of the file.</param>
        /// <returns>true the file name is an exclusive file; otherwise, false.</returns>
        static bool IsExclusiveFile(std::wstring const& fileName);

        /// <summary>Queries is executable.</summary>
        /// <param name="filename">Filename of the file.</param>
        /// <returns>true if executable; otherwise, false.</returns>
        static bool IsExecutable(std::wstring const& filename);

        /// <summary>Gets the company of the given file.</summary>
        /// <param name="target">File name of the file to check.</param>
        /// <returns>The company.</returns>
        static std::wstring GetCompany(std::wstring const& target);
    };

    /// <summary>Find files record.</summary>
    class FindFilesRecord
    {
        std::wstring cFileName;
        std::uint64_t ftCreationTime;
        std::uint64_t ftLastAccessTime;
        std::uint64_t ftLastWriteTime;
        std::uint64_t nFileSize;
        DWORD dwFileAttributes;
    public:
        /// <summary>Initializes a new instalce of the <c>FindFilesRecord</c> class.</summary>
        /// <param name="prefix">The prefix path.</param>
        /// <param name="winSource">The windows data record source.</param>
        FindFilesRecord(std::wstring prefix, WIN32_FIND_DATAW const& winSource);

        /// <summary>Copy constructor.</summary>
        /// <param name="other">The object to copy.</param>
        FindFilesRecord(FindFilesRecord const& other);

        /// <summary>Move constructor.</summary>
        /// <param name="other">[in,out] The other.</param>
        FindFilesRecord(FindFilesRecord&& other) throw();

        /// <summary>Assignment operator.</summary>
        /// <param name="other">The other.</param>
        /// <returns>*this.</returns>
        FindFilesRecord& operator=(FindFilesRecord other);

        /// <summary>Gets file name.</summary>
        /// <returns>The file name.</returns>
        std::wstring const& GetFileName() const throw();

        /// <summary>Gets creation time.</summary>
        /// <returns>The creation time.</returns>
        std::uint64_t GetCreationTime() const throw();

        /// <summary>Gets the last access time.</summary>
        /// <returns>The last access time.</returns>
        std::uint64_t GetLastAccessTime() const throw();

        /// <summary>Gets the last write time.</summary>
        /// <returns>The last write time.</returns>
        std::uint64_t GetLastWriteTime() const throw();

        /// <summary>Gets the size.</summary>
        /// <returns>The size.</returns>
        std::uint64_t GetSize() const throw();

        /// <summary>Gets the attributes.</summary>
        /// <returns>The attributes.</returns>
        DWORD GetAttributes() const throw();

        /// <summary>Swaps this instance with another <c>FindFilesRecord</c>.</summary>
        /// <param name="other">[in,out] The instance with which this instance is swapped.</param>
        void swap(FindFilesRecord &other) throw();
    };

    /// <summary>Determines if the given <c>FindFilesRecord</c> is a "dot directory".</summary>
    /// <param name="test">The record to test.</param>
    /// <returns>true if dot directory, false if not.</returns>
    inline bool IsDotDirectory(FindFilesRecord const& test) throw()
    {
        auto const& str = test.GetFileName();
        return (test.GetAttributes() & FILE_ATTRIBUTE_DIRECTORY) && (str == L"." || str == L"..");
    }

    /// <summary>Swaps a pair of <c>FindFilesRecord</c> instances.</summary>
    /// <param name="lhs">[in,out] The left hand side.</param>
    /// <param name="rhs">[in,out] The right hand side.</param>
    inline void swap(FindFilesRecord &lhs, FindFilesRecord &rhs) throw()
    {
        lhs.swap(rhs);
    }

    /// @brief    Finds files in directories.  Wrapper around FindFirstFile and FindNextFile
    class FindFiles : boost::noncopyable
    {
        std::stack<HANDLE, std::vector<HANDLE>> handles;
        const bool skipDotDirectories;
        const bool recursive;
        std::wstring pattern;
        std::stack<const std::wstring> subPaths;

        /// @summary    The data of the next file found.
        expected<FindFilesRecord> data;

        /**
         * Gets the next file spec for enumerating recurisively.
         */
        std::wstring GetNextSpec() const;
    public:
        /// Gets the data record for the current index.
        /// @return The data record for the current index.
        expected<FindFilesRecord> const& GetData() const throw()
        {
            return data;
        }

        /// @brief    Constructor
        ///
        /// @param    pattern            A pattern specifying the files of interest. Supports everything that
        ///                              FindFirstFile supports (check MSDN docs for this)
        /// @param    recursive          (optional) whether to recurse deeper into directories or not
        /// @param    skipDotDirectories (optional) if this is true, the implied directories . and .. will be skipped
        /// 
        /// @detail This will swallow invalid path and invalid file exceptions and instead just set
        ///         IsValid to false.
        FindFiles(std::wstring const& patternSpec, bool recursive = false, bool skipDotDirectories = true);

        /// @brief    Destructor.
        ~FindFiles() throw();

        /// @brief    Populates data with the next file (if there is one)
        void Next() throw();

        /**
         * Checks whether or not this instance has more entries to enumerate.
         */
        bool IsValid() throw()
        {
            return !handles.empty();
        }
    };

    /// <summary>Low level find handle.</summary>
    /// <seealso cref="T:boost::noncopyable"/>
    /// <seealso cref="T:WIN32_FIND_DATAW"/>
    class FindHandle : private boost::noncopyable, public WIN32_FIND_DATAW
    {
        HANDLE hFind;
        DWORD lastError;
        void Close() throw();
    public:

        /// <summary>Constructor. Begins a <c>FindFirstFile</c> search.</summary>
        /// <param name="pattern">Specifies the pattern applied to FindFirstFile.</param>
        FindHandle(wchar_t const* pattern);

        /// <summary>Advances this handle to the next <c>WIN32_FIND_DATAW</c>D instance.</summary>
        void Next() throw();

        /// <summary>Queries if this instance has valid file data.</summary>
        /// <returns>true if there is data; otherwise, false.</returns>
        bool HasEntry() const throw();

        /// <summary>Gets the last error encountered when processing files.</summary>
        /// <returns>The last error code encountered when processing files.</returns>
        DWORD LastError() const throw();

        /// <summary>Destructor.</summary>
        ~FindHandle() throw();
    };

    class FindFilesLocal : boost::noncopyable
    {
        std::wstring prefix;
        FindHandle handle;
        bool dotSkipping;
    public:
        FindFilesLocal(std::wstring const& pattern);
        FindFilesLocal(std::wstring && pattern);
        FindFilesLocal(std::wstring const& pattern, bool skipDotDirectories);
        FindFilesLocal(std::wstring && pattern, bool skipDotDirectories);

        void Next() throw();
        bool HasEntry() const throw();

        expected<FindFilesRecord> GetRecord();
    };
}}
