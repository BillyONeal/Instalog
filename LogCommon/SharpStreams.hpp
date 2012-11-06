// Copyright © 2012 Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

// Sharp Streams are a simple "port" of C# (or rather, the .NET base class library)
// stream interface into C++. Couple of notes:
//
// 1. This is not a direct translation. There are a few design oddities of C# streams
//    which would be nice to get rid of (namely, the XxxReader interfaces taking
//    ownership over freeing the underlying streams)
// 2. Only the bits of the System.IO style streams that Instalog needs have been
//    ported.

#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <Windows.h>

namespace Instalog { namespace SharpStreams {

    // Converts from one code-set (such as UTF-8) to another
    struct Encoder
    {
        virtual std::wstring Name() = 0;
        virtual std::vector<wchar_t> GetChars(const unsigned char *target, std::uint32_t length) = 0;
        virtual std::vector<unsigned char> GetBytes(const wchar_t *target, std::uint32_t length) = 0;
        virtual ~Encoder() {}
    };

#ifdef WIN32
    // Converts wide character strings to or from the system's current active code page. (CP_ACP in Windows Headers)
    struct AcpEncoder : public Encoder
    {
        virtual std::wstring Name() override;
        virtual std::vector<wchar_t> GetChars(const unsigned char *target, std::uint32_t length) override;
        virtual std::vector<unsigned char> GetBytes(const wchar_t *target, std::uint32_t length) override;
    };

    // Converts wide character strings to or from the system's current OEM code page. (CP_OEMCP in Windows Headers)
    struct OemEncoder : public Encoder
    {
        virtual std::wstring Name() override;
        virtual std::vector<wchar_t> GetChars(const unsigned char *target, std::uint32_t length) override;
        virtual std::vector<unsigned char> GetBytes(const wchar_t *target, std::uint32_t length) override;
    };
#endif

    // Converts wide character strings to or from UTF-8
    struct Utf8Encoder : public Encoder
    {
        virtual std::wstring Name() override;
        virtual std::vector<wchar_t> GetChars(const unsigned char *target, std::uint32_t length) override;
        virtual std::vector<unsigned char> GetBytes(const wchar_t *target, std::uint32_t length) override;
    };

    // Converts wide character strings to or from UTF-16 (which is a no-op)
    struct Utf16Encoder : public Encoder
    {
        virtual std::wstring Name() override;
        virtual std::vector<wchar_t> GetChars(const unsigned char *target, std::uint32_t length) override;
        virtual std::vector<unsigned char> GetBytes(const wchar_t *target, std::uint32_t length) override;
    };

    enum class SeekOrigin
    {
        Beginning,
        Current,
        End
    };

    // Represents a basic on-disk or in-memory representation of a stream of bytes.
    // This is the low-level representation of a file handle or memory buffer.
    struct Stream : boost::noncopyable
    {
        virtual ~Stream() {}
        virtual void Flush();
        virtual void Seek(std::int64_t offset, SeekOrigin origin);
        virtual void Read(unsigned char *target, std::uint32_t offset, std::uint32_t length);
        virtual void Write(unsigned char *target, std::uint32_t offset, std::uint32_t length);
    };

    class FileStream : public Stream
    {
        HANDLE hFile;
    public:
        FileStream(std::wstring fileName, DWORD desiredAccess, DWORD shareMode, DWORD creationDisposition, DWORD attributes);
        ~FileStream();
        virtual void Flush() override;
        virtual void Seek(std::int64_t offset, SeekOrigin origin) override;
        virtual void Read(unsigned char *target, std::uint32_t offset, std::uint32_t length) override;
        virtual void Write(unsigned char *target, std::uint32_t offset, std::uint32_t length) override;
    };

    class MemoryStream : public Stream
    {
        std::vector<unsigned char> buffer;
    public:
        virtual void Flush() override;
        virtual void Seek(std::int64_t offset, SeekOrigin origin) override;
        virtual void Read(unsigned char *target, std::uint32_t offset, std::uint32_t length) override;
        virtual void Write(unsigned char *target, std::uint32_t offset, std::uint32_t length) override;
        const std::vector<unsigned char>& GetReadOnlyBufferView() const;
        std::vector<unsigned char> GetBufferCopy() const;
    };

}} // Instalog::SharpStreams
