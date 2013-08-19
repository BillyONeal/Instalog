// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <cstdint>
#include <windows.h>
#include <boost/noncopyable.hpp>
#include "Win32Exception.hpp"

namespace Instalog {

    /// @brief    Converts Windows FILETIME structure to an int
    ///
    /// @param    ft    The filetime.
    ///
    /// @return    std::uint64_t representation of ft
    inline std::uint64_t FiletimeToInteger(FILETIME const& ft)
    {
        std::uint64_t result = ft.dwHighDateTime;
        result <<= 32;
        result |= ft.dwLowDateTime;
        return result;
    }

    /// @brief    Seconds since 1970 from FILETIME struct
    ///
    /// @param    time    The time as a FILETIME
    ///
    /// @return    Seconds since 1970
    DWORD SecondsSince1970(FILETIME const& time);

    /// @brief    Seconds since 1970 from SYSTEMTIME struct
    ///
    /// @param    time    The time as a SYSTEMTIME
    ///
    /// @return    Seconds since 1970
    DWORD SecondsSince1970(SYSTEMTIME const& time);

    /// @brief    Creates a FILETIME struct from seconds since 1970.
    ///
    /// @param    seconds    The seconds since 1970.
    ///
    /// @return    The time as a FILETIME struct
    FILETIME FiletimeFromSecondsSince1970(DWORD seconds);

    /// @brief    Creates a SYSTEMTIME struct from seconds since 1970.
    ///
    /// @param    seconds    The seconds since 1970.
    ///
    /// @return    The time as a SYSTEMTIME struct
    SYSTEMTIME SystemtimeFromSecondsSince1970(DWORD seconds);

	class UniqueHandle : boost::noncopyable
	{
		HANDLE handle;
	public:
		UniqueHandle(HANDLE handle_ = INVALID_HANDLE_VALUE);
		UniqueHandle(UniqueHandle&& other);
		UniqueHandle& operator=(UniqueHandle&& other);
		bool IsOpen() const;
		HANDLE Get();
		HANDLE* Ptr();
		void Close();
		~UniqueHandle();
	};

    /**
     * Scoped object which disables the Wow64 file system redirector, if applicable.
     * @sa boost::noncopyable
     */
    class Disable64FsRedirector : boost::noncopyable
    {
        void * previousState;
    public:

        /**
         * Disables the file system redirector.
         */
        Disable64FsRedirector();

        /**
         * Disables the file system redirector, if it is enabled.
         */
        void Disable() throw();

        /**
         * Enables the file system redirector, if it is disabled.
         */
        void Enable() throw();

        /**
         * Enables the file system redirector.
         */
        ~Disable64FsRedirector() throw();
    };

    /**
     * Query if this process is running as a WOW64 process.
     * @return true inside WOW64; otherwise, false.
     */
    bool IsWow64Process() throw();
}