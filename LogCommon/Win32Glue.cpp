// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "Library.hpp"
#include "Win32Glue.hpp"
#include "Win32Exception.hpp"

namespace
{
typedef BOOL(WINAPI* IsWow64ProcessFunc)(HANDLE, PBOOL);
typedef BOOL(WINAPI* Wow64DisableFsRedirectionFunc)(PVOID* OldValue);
typedef BOOL(WINAPI* Wow64RevertFsRedirectionFunc)(PVOID OldValue);
}

namespace Instalog
{

// http://msdn.microsoft.com/en-us/library/windows/desktop/ms724928.aspx
static ULARGE_INTEGER GetLargeJan1970()
{
    static ULARGE_INTEGER largeJan1970;
    if (largeJan1970.QuadPart == 0)
    {
        // 3. Initialize a SYSTEMTIME structure with the date and time of the
        // first second of January 1, 1970.
        SYSTEMTIME jan1970 = {1970, 1, 4, 1, 0, 0, 0, 0};

        // 4. Call SystemTimeToFileTime, passing the SYSTEMTIME structure
        // initialized in Step 3 to the call.
        FILETIME ftjan1970;
        if (SystemTimeToFileTime(&jan1970, &ftjan1970) == false)
        {
            SystemFacades::Win32Exception::ThrowFromLastError();
        }

        // 5. Copy the contents of the FILETIME structure returned by
        // SystemTimeToFileTime in Step 4 to a second ULARGE_INTEGER. The copied
        // value should be less than or equal to the value copied in Step 2.
        largeJan1970;
        largeJan1970.LowPart = ftjan1970.dwLowDateTime;
        largeJan1970.HighPart = ftjan1970.dwHighDateTime;
    }

    return largeJan1970;
}

// http://msdn.microsoft.com/en-us/library/windows/desktop/ms724928.aspx
DWORD SecondsSince1970(FILETIME const& time)
{
    // 2. Copy the contents of the FILETIME structure to a ULARGE_INTEGER
    // structure.
    ULARGE_INTEGER inftime;
    inftime.LowPart = time.dwLowDateTime;
    inftime.HighPart = time.dwHighDateTime;

    ULARGE_INTEGER temp = GetLargeJan1970();
    // 6. Subtract the 64-bit value in the ULARGE_INTEGER structure initialized
    // in Step 5(January 1, 1970) from the 64-bit value of the ULARGE_INTEGER
    // structure initialized in Step 2 (the current system time). This produces
    // a value in 100-nanosecond intervals since January 1, 1970. To convert
    // this value to seconds, divide by 10,000,000.
    ULONGLONG value100NS = inftime.QuadPart - temp.QuadPart;
    ULONGLONG result = value100NS / 10000000;

    return static_cast<DWORD>(result);
}

// http://msdn.microsoft.com/en-us/library/windows/desktop/ms724928.aspx
DWORD SecondsSince1970(SYSTEMTIME const& time)
{
    // 1. Call SystemTimeToFileTime to copy the system time to a FILETIME
    // structure. Call GetSystemTime to get the current system time to pass to
    // SystemTimeToFileTime.
    FILETIME filetime;
    if (SystemTimeToFileTime(&time, &filetime) == false)
    {
        SystemFacades::Win32Exception::ThrowFromLastError();
    }
    return SecondsSince1970(filetime);
}

FILETIME FiletimeFromSecondsSince1970(DWORD seconds)
{
    ULARGE_INTEGER temp = GetLargeJan1970();
    ULONGLONG nanoseconds1970 =
        static_cast<ULONGLONG>(seconds) *
        10000000; // Cast is important, otherwise overflows happen
    ULONGLONG nanoseconds1600 = temp.QuadPart + nanoseconds1970;

    ULARGE_INTEGER large1600;
    large1600.QuadPart = nanoseconds1600;

    FILETIME filetime;
    filetime.dwLowDateTime = large1600.LowPart;
    filetime.dwHighDateTime = large1600.HighPart;

    return filetime;
}

SYSTEMTIME SystemtimeFromSecondsSince1970(DWORD seconds)
{
    FILETIME filetime = FiletimeFromSecondsSince1970(seconds);
    SYSTEMTIME systemtime;
    if (FileTimeToSystemTime(&filetime, &systemtime) == false)
    {
        SystemFacades::Win32Exception::ThrowFromLastError();
    }
    return systemtime;
}

UniqueHandle::UniqueHandle(HANDLE handle_) : handle(handle_)
{
}

UniqueHandle::UniqueHandle(UniqueHandle&& other) : handle(other.handle)
{
    other.handle = 0;
}

UniqueHandle& UniqueHandle::operator=(UniqueHandle&& other)
{
    HANDLE hTemp = other.handle; // Assignment to self.
    other.handle = 0;
    handle = hTemp;
    return *this;
}

bool UniqueHandle::IsOpen() const
{
    return handle != 0 && handle != INVALID_HANDLE_VALUE;
}

HANDLE UniqueHandle::Get()
{
    return handle;
}

HANDLE* UniqueHandle::Ptr()
{
    return &handle;
}

void UniqueHandle::Close()
{
    if (IsOpen())
    {
        ::CloseHandle(handle);
        handle = 0;
    }
}

UniqueHandle::~UniqueHandle()
{
    Close();
}

Disable64FsRedirector::Disable64FsRedirector() : previousState(nullptr)
{
    Disable();
}

void Disable64FsRedirector::Disable() throw()
{
#ifndef _M_X64
    if (previousState != nullptr)
    {
        return;
    }

    SystemFacades::RuntimeDynamicLinker kernel32(L"kernel32.dll");
    auto const isWow64 =
        kernel32.GetProcAddress<IsWow64ProcessFunc>("IsWow64Process");
    if (isWow64 == nullptr)
    {
        return;
    }

    BOOL isWow64Result = FALSE;
    isWow64(::GetCurrentProcess(), &isWow64Result);
    if (isWow64Result != TRUE)
    {
        return;
    }

    auto const disableFunc =
        kernel32.GetProcAddress<Wow64DisableFsRedirectionFunc>(
            "Wow64DisableWow64FsRedirection");
    if (disableFunc == nullptr)
    {
        return;
    }

    disableFunc(&previousState);
#endif
}

void Disable64FsRedirector::Enable() throw()
{
#ifndef _M_X64
    if (previousState == nullptr)
    {
        return;
    }

    SystemFacades::RuntimeDynamicLinker kernel32(L"kernel32.dll");
    auto const enableFunc =
        kernel32.GetProcAddress<Wow64RevertFsRedirectionFunc>(
            "Wow64RevertWow64FsRedirection");
    assert(enableFunc != nullptr);
    enableFunc(previousState);
    previousState = nullptr;
#endif
}

Disable64FsRedirector::~Disable64FsRedirector() throw()
{
    Enable();
}

bool IsWow64Process() throw()
{
#ifndef _M_X64
    SystemFacades::RuntimeDynamicLinker kernel32(L"kernel32.dll");
    auto const isWow64 =
        kernel32.GetProcAddress<IsWow64ProcessFunc>("IsWow64Process");
    if (isWow64 == nullptr)
    {
        return false;
    }

    BOOL isWow64Result = FALSE;
    isWow64(::GetCurrentProcess(), &isWow64Result);
    return isWow64Result == TRUE;
#else
    return false;
#endif
}
}