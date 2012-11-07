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
#include <boost/noncopyable.hpp>
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
        virtual void Flush() = 0;
        virtual void Seek(std::int64_t offset, SeekOrigin origin) = 0;
        virtual std::uint32_t Read(unsigned char *target, std::uint32_t offset, std::uint32_t length) = 0;
        virtual void Write(unsigned char *target, std::uint32_t offset, std::uint32_t length) = 0;
    };

    class FileStream : public Stream
    {
        HANDLE hFile;
    public:
        FileStream(std::wstring fileName, DWORD desiredAccess, DWORD shareMode, DWORD creationDisposition, DWORD attributes);
        ~FileStream();
        virtual void Flush() override;
        virtual void Seek(std::int64_t offset, SeekOrigin origin) override;
        virtual std::uint32_t Read(unsigned char *target, std::uint32_t offset, std::uint32_t length) override;
        virtual void Write(unsigned char *target, std::uint32_t offset, std::uint32_t length) override;
    };

    class MemoryStream : public Stream
    {
        std::vector<unsigned char> buffer;
        std::size_t pointer;
        std::size_t GetAvailableToRead() const;
    public:
        virtual void Flush() override;
        virtual void Seek(std::int64_t offset, SeekOrigin origin) override;
        virtual std::uint32_t Read(unsigned char *target, std::uint32_t offset, std::uint32_t length) override;
        virtual void Write(unsigned char *target, std::uint32_t offset, std::uint32_t length) override;
        const std::vector<unsigned char>& GetReadOnlyBufferView() const;
        std::vector<unsigned char> GetBufferCopy() const;
        std::vector<unsigned char> StealBuffer();
    };

    class TextWriter
    {
        std::wstring newLine;
        std::unique_ptr<Encoder> encoder;
    public:
        std::wstring const& GetNewline() const;
        void SetNewline(std::wstring line);
        Encoder const * GetEncoder() const;
        void SetEncoder(std::unique_ptr<Encoder> encoder);
        virtual void Flush() = 0;
        virtual void Write(bool value) = 0;
        virtual void Write(char value) = 0;
        virtual void Write(wchar_t value) = 0;
        virtual void Write(double value) = 0;
        virtual void Write(float value) = 0;
        virtual void Write(std::int32_t value) = 0;
        virtual void Write(std::int64_t value) = 0;
        virtual void Write(std::uint32_t value) = 0;
        virtual void Write(std::uint64_t value) = 0;
        virtual void Write(std::string const& value) = 0;
        virtual void Write(char const* value) = 0;
        virtual void Write(std::wstring const& value) = 0;
        virtual void Write(wchar_t const* value) = 0;
        virtual void WriteLine(bool value) = 0;
        virtual void WriteLine(char value) = 0;
        virtual void WriteLine(wchar_t value) = 0;
        virtual void WriteLine(double value) = 0;
        virtual void WriteLine(float value) = 0;
        virtual void WriteLine(std::int32_t value) = 0;
        virtual void WriteLine(std::int64_t value) = 0;
        virtual void WriteLine(std::uint32_t value) = 0;
        virtual void WriteLine(std::uint64_t value) = 0;
        virtual void WriteLine(std::string const& value) = 0;
        virtual void WriteLine(char const* value) = 0;
        virtual void WriteLine(std::wstring const& value) = 0;
        virtual void WriteLine(wchar_t const* value) = 0;
        TextWriter();
        TextWriter(std::unique_ptr<Encoder> encoder);
        virtual ~TextWriter() { }
    };

}} // Instalog::SharpStreams
