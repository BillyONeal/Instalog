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

namespace Instalog { namespace SharpStreams {

    // Converts from one code-set (such as UTF-8) to another
    struct Encoder
    {

    };

    enum SeekOrigin
    {
        Beginning,
        Current,
        End
    };

    // Represents a basic on-disk or in-memory representation of a stream of bytes.
    // This is the low-level representation of a file handle or memory buffer.
    struct Stream
    {
        virtual ~Stream() {}
        virtual void Flush();
        virtual void Seek(std::int64_t offset, SeekOrigin origin);
        virtual void Read(char *target, std::uint32_t offset, std::uint32_t length);
        virtual void Write(char *target, std::uint32_t offset, std::uint32_t length);
    };


}} // Instalog::SharpStreams
