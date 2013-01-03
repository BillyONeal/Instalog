// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "File.hpp"
#include "Win32Exception.hpp"
#include <boost/algorithm/string/predicate.hpp>

#pragma comment(lib, "Version.lib")

namespace Instalog { namespace SystemFacades {

    File::File( 
        std::wstring const& filename, 
        DWORD desiredAccess, 
        DWORD shareMode, 
        LPSECURITY_ATTRIBUTES securityAttributes, 
        DWORD createdDisposition, 
        DWORD flags
        )
        : hFile(::CreateFileW(filename.c_str(), desiredAccess, shareMode, securityAttributes, createdDisposition, flags, nullptr))
    {
        if (hFile == INVALID_HANDLE_VALUE)
        {
            Win32Exception::ThrowFromLastError();
        }
    }

    File::File( File && other )
    {
        hFile = other.hFile;
        other.hFile = INVALID_HANDLE_VALUE;
    }

    File::File()
        : hFile(INVALID_HANDLE_VALUE)
    {
    }

    File::~File()
    {
        if (hFile == INVALID_HANDLE_VALUE)
        {
            return;
        }
        ::CloseHandle(hFile);
    }

    std::uint64_t File::GetSize() const
    {
        BY_HANDLE_FILE_INFORMATION info = GetExtendedAttributes();

        std::uint64_t highSize = info.nFileSizeHigh;
        highSize <<= 32;
        return highSize + info.nFileSizeLow;
    }

    DWORD File::GetAttributes() const
    {
        return GetExtendedAttributes().dwFileAttributes;
    }

    BY_HANDLE_FILE_INFORMATION File::GetExtendedAttributes() const
    {
        BY_HANDLE_FILE_INFORMATION info;

        if (GetFileInformationByHandle(hFile, &info) == false)
        {
            Win32Exception::ThrowFromLastError();
        }

        return info;
    }

    std::vector<char> File::ReadBytes( unsigned int bytesToRead ) const
    {
        std::vector<char> bytes(bytesToRead);
        DWORD bytesRead = 0;

        if (::ReadFile(hFile, bytes.data(), bytesToRead, &bytesRead, NULL) == false)
        {
            Win32Exception::ThrowFromLastError();
        }

        if (bytesRead < bytesToRead)
        {
            bytes.resize(bytesRead);
        }

        return bytes;
    }

    bool File::WriteBytes( std::vector<char> const& bytes )
    {
        DWORD bytesWritten;

        if (WriteFile(hFile, bytes.data(), static_cast<DWORD>(bytes.size()), &bytesWritten, NULL) == false)
        {
            Win32Exception::ThrowFromLastError();
        }

        if (bytesWritten == bytes.size())
            return true;
        else
            return false;
    }

    void File::Delete( std::wstring const& filename)
    {
        if (::DeleteFileW(filename.c_str()) == 0) 
        {
            Win32Exception::ThrowFromLastError();
        }
    }

    bool File::Exists( std::wstring const& filename)
    {
        DWORD attributes = ::GetFileAttributesW(filename.c_str());

        return attributes != INVALID_FILE_ATTRIBUTES;
    }

    bool File::IsDirectory( std::wstring const& filename)
    {
        DWORD attributes = ::GetFileAttributesW(filename.c_str());

        return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && (attributes != INVALID_FILE_ATTRIBUTES);
    }

    bool File::IsExecutable( std::wstring const& filename )
    {
        if (Exists(filename) == false)
            return false;
        if (IsDirectory(filename))
            return false;

        File executable(filename, FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING);
        std::vector<char> bytes = executable.ReadBytes(2);
        return bytes.size() >=2 && bytes[0] == 'M' && bytes[1] == 'Z';
    }

    std::wstring File::GetCompany(std::wstring const& filename)
    {
        DWORD infoSize = ::GetFileVersionInfoSizeW(filename.c_str(), 0);
        if (infoSize == 0)
        {
            Win32Exception::ThrowFromLastError();
        }
        std::vector<char> buff(infoSize);
        if (::GetFileVersionInfoW(filename.c_str(), 0, static_cast<DWORD>(buff.size()), buff.data()) == 0)
        {
            Win32Exception::ThrowFromLastError();
        }
        wchar_t const targetPath[] = L"\\StringFileInfo\\040904B0\\CompanyName";
        void * companyData;
        UINT len;
        if (::VerQueryValueW(buff.data(), targetPath, &companyData, &len) == 0)
        {
            Win32Exception::ThrowFromLastError();
        }
        if (len == 0)
        {
            return std::wstring();
        }
        else
        {
            return std::wstring(static_cast<wchar_t *>(companyData), len-1);
        }
    }

    std::uint64_t File::GetSize( std::wstring const& filename )
    {
        WIN32_FILE_ATTRIBUTE_DATA fad = File::GetExtendedAttributes(filename);
        std::uint64_t size = fad.nFileSizeHigh;
        size <<= 32;
        size |= fad.nFileSizeLow;
        return size;
    }

    DWORD File::GetAttributes( std::wstring const& filename )
    {
        DWORD answer = ::GetFileAttributesW(filename.c_str());
        if (answer == INVALID_FILE_ATTRIBUTES)
        {
            Win32Exception::ThrowFromLastError();
        }
        return answer;
    }

    WIN32_FILE_ATTRIBUTE_DATA File::GetExtendedAttributes( std::wstring const& filename )
    {
        WIN32_FILE_ATTRIBUTE_DATA fad;
        if (::GetFileAttributesExW(filename.c_str(), GetFileExInfoStandard, &fad) == 0)
        {
            Win32Exception::ThrowFromLastError();
        }
        return fad;
    }

    File& File::operator=( File other )
    {
        File copied(std::move(other));
        std::swap(this->hFile, copied.hFile);
        return *this;
    }

    bool File::IsExclusiveFile( std::wstring const& fileName )
    {
        DWORD attribs = ::GetFileAttributesW(fileName.c_str());
        if (attribs == INVALID_FILE_ATTRIBUTES)
        {
            return false;
        }
        else
        {
            return (attribs & FILE_ATTRIBUTE_DIRECTORY) == 0;
        }
    }

    /// Initializes a new instance of the FindFilesRecord class.
    /// @param prefix    The prefix path.
    /// @param winSource The windows data record source.
    FindFilesRecord::FindFilesRecord(std::wstring prefix, WIN32_FIND_DATAW const& winSource)
        : dwFileAttributes(winSource.dwFileAttributes)
    {
        prefix.append(winSource.cFileName);
        cFileName = std::move(prefix);
        ftCreationTime = static_cast<std::uint64_t>(winSource.ftCreationTime.dwHighDateTime) << 32 & winSource.ftCreationTime.dwLowDateTime;
        ftLastAccessTime = static_cast<std::uint64_t>(winSource.ftLastAccessTime.dwHighDateTime) << 32 & winSource.ftLastAccessTime.dwLowDateTime;
        ftLastWriteTime = static_cast<std::uint64_t>(winSource.ftLastWriteTime.dwHighDateTime) << 32 & winSource.ftLastWriteTime.dwLowDateTime;
        nFileSize = static_cast<std::uint64_t>(winSource.nFileSizeHigh) << 32 & winSource.nFileSizeLow;
    }

    /// Initializes a new instance of the File class.
    /// @param other The copied record.
    FindFilesRecord::FindFilesRecord(FindFilesRecord const& other)
        : cFileName(other.cFileName)
        , ftCreationTime(other.ftCreationTime)
        , ftLastAccessTime(other.ftLastAccessTime)
        , ftLastWriteTime(other.ftLastWriteTime)
        , nFileSize(other.nFileSize)
        , dwFileAttributes(other.dwFileAttributes)
    { }

    /// Initializes a moved instance of the File class.
    /// @param other The moved record.
    FindFilesRecord::FindFilesRecord(FindFilesRecord&& other) throw()
        : cFileName(std::move(other.cFileName))
        , ftCreationTime(other.ftCreationTime)
        , ftLastAccessTime(other.ftLastAccessTime)
        , ftLastWriteTime(other.ftLastWriteTime)
        , nFileSize(other.nFileSize)
        , dwFileAttributes(other.dwFileAttributes)
    { }

    /// Assignment operator.
    /// @param other The copied item.
    /// @return A shallow copy of this object.
    FindFilesRecord& FindFilesRecord::operator=(FindFilesRecord other)
    {
        other.swap(*this);
        return *this;
    }

    /// Gets file name.
    /// @return The file name.
    std::wstring const& FindFilesRecord::GetFileName() const throw()
    {
        return cFileName;
    }

    /// Gets creation time.
    /// @return The creation time.
    std::uint64_t FindFilesRecord::GetCreationTime() const throw()
    {
        return ftCreationTime;
    }

    /// Gets the last access time.
    /// @return The last access time.
    std::uint64_t FindFilesRecord::GetLastAccessTime() const throw()
    {
        return ftLastAccessTime;
    }

    /// Gets the last write time.
    /// @return The last write time.
    std::uint64_t FindFilesRecord::GetLastWriteTime() const throw()
    {
        return ftLastWriteTime;
    }

    /// Gets the size.
    /// @return The size.
    std::uint64_t FindFilesRecord::GetSize() const throw()
    {
        return nFileSize;
    }

    /// Gets the attributes.
    /// @return The attributes.
    DWORD FindFilesRecord::GetAttributes() const throw()
    {
        return dwFileAttributes;
    }

    /// Swaps the given record.
    /// @param [in,out] other The other record with which to swap.
    void FindFilesRecord::swap(FindFilesRecord &other) throw()
    {
        using std::swap;
        swap(cFileName, other.cFileName);
        swap(ftCreationTime, other.ftCreationTime);
        swap(ftLastAccessTime, other.ftLastAccessTime);
        swap(ftLastWriteTime, other.ftLastWriteTime);
        swap(nFileSize, other.nFileSize);
        swap(dwFileAttributes, other.dwFileAttributes);
    }

    FindFiles::FindFiles( std::wstring patternSpec, bool recursive /*= false*/, bool skipDotDirectories /*= true*/ )
        : recursive(recursive)
        , skipDotDirectories(skipDotDirectories)
    {
        auto patternStart = std::find(patternSpec.crbegin(), patternSpec.crend(), L'\\');
        auto patternBase = patternStart.base();
        if (patternStart == patternSpec.crend() || patternStart + 1 == patternSpec.crend())
        {
            // We didn't find a pattern, default to selecting everything.
            pattern = L"*";
        }
        else
        {
            // We found a pattern, store it
            pattern.assign(patternBase + 1, patternSpec.cend());
        }
        subPaths.emplace(patternSpec.cbegin(), patternBase);
        subPaths.top().push_back(L'\\');

        WIN32_FIND_DATAW dataBlock;
        HANDLE handle = ::FindFirstFileW(patternSpec.c_str(), &dataBlock);

        if (handle == INVALID_HANDLE_VALUE)
        {
            data = expected<FindFilesRecord>::from_exception(Win32Exception::FromLastError());
        }
        else
        {
            data = FindFilesRecord(subPaths.top(), dataBlock);
            if (skipDotDirectories && IsDotDirectory(data.get()))
            {
                Next();
            }
        }
    }

    FindFiles::~FindFiles() throw()
    {
        while (handles.empty() == false)
        {
            ::FindClose(handles.top());
            handles.pop();
        }
    }

    void FindFiles::Next() throw()
    {
        WIN32_FIND_DATAW dataBlock;
        if (recursive && data.is_valid())
        {
            auto const& previous = data.get();
            if ((previous.GetAttributes() & FILE_ATTRIBUTE_DIRECTORY) && !IsDotDirectory(previous))
            {
                assert(!subPaths.empty() && "Attempted to recurse; but no root path left.");
                auto const& previousRoot = subPaths.top();
                std::wstring nextRoot;
                nextRoot.reserve(previousRoot.size() + previous.GetFileName().size() + 1);
                nextRoot.append(previousRoot);

                nextRoot.append(dataBlock.cFileName);
                nextRoot.push_back(L'\\');
                subPaths.push(nextRoot);

                nextRoot.append(pattern);
                HANDLE handle = ::FindFirstFile(nextRoot.c_str(), &dataBlock);
                if (handle == INVALID_HANDLE_VALUE)
                {
                    data = expected<FindFilesRecord>::from_exception(Win32Exception::FromLastError());
                }
                else
                {
                    data = FindFilesRecord(subPaths.top(), dataBlock);
                    handles.push(handle);

                    if (skipDotDirectories && IsDotDirectory(data.get()))
                    {
                        this->Next();
                    }
                    return;
                }
            }
        }

        // Get the next file, skip . and .. if requested
        do 
        {
            if (::FindNextFile(handles.top(), &dataBlock) == false)
            {
                DWORD errorStatus = ::GetLastError();
                if (errorStatus == ERROR_NO_MORE_FILES)
                {
                    ::FindClose(handles.top());
                    handles.pop();
                    subPaths.pop();
                    if (!handles.empty())
                    {
                        data.clear();
                        this->Next();
                        return;
                    }
                }
                else
                {
                    data = expected<FindFilesRecord>::from_exception(Win32Exception::FromLastError());
                }
            }
        } while (skipDotDirectories && (wcscmp(dataBlock.cFileName, L".") == 0 || wcscmp(dataBlock.cFileName, L"..") == 0));

        data = FindFilesRecord(subPaths.top(), dataBlock);
    }

}}
