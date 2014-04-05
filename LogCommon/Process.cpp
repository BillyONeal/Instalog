// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <cstring>
#include <functional>
#include <windows.h>
#include "Win32Exception.hpp"
#include "Win32Glue.hpp"
#include "Library.hpp"
#include "ScopeExit.hpp"
#include "ScopedPrivilege.hpp"
#include "Process.hpp"
#include "DdkStructures.h"
#include "Utf8.hpp"

namespace
{
typedef UNICODE_STRING(
    RTL_USER_PROCESS_PARAMETERS::*RtlUserProcessParametersSelector);
}

namespace Instalog
{
namespace SystemFacades
{

ProcessEnumerator::ProcessEnumerator()
{
    NtQuerySystemInformationFunc ntQuerySysInfo =
        library::ntdll().get_function<NtQuerySystemInformationFunc>(
            GetThrowingErrorReporter(),
            "NtQuerySystemInformation"
            );
    NTSTATUS errorCheck = STATUS_INFO_LENGTH_MISMATCH;
    ULONG goalLength = 0;
    while (errorCheck == STATUS_INFO_LENGTH_MISMATCH)
    {
        if (goalLength == 0)
        {
            informationBlock.resize(informationBlock.size() + 1024);
        }
        else
        {
            informationBlock.resize(goalLength);
        }
        errorCheck = ntQuerySysInfo(SystemProcessInformation,
                                    informationBlock.data(),
                                    static_cast<ULONG>(informationBlock.size()),
                                    &goalLength);
    }
    if (errorCheck != 0)
    {
        Win32Exception::ThrowFromNtError(errorCheck);
    }
}

ProcessIterator ProcessEnumerator::begin()
{
    return ProcessIterator(informationBlock.cbegin(), informationBlock.cend());
}

ProcessIterator ProcessEnumerator::end()
{
    return ProcessIterator(informationBlock.cend(), informationBlock.cend());
}

ProcessIterator::ProcessIterator(
    std::vector<unsigned char>::const_iterator start,
    std::vector<unsigned char>::const_iterator end)
    : blockPtr(start)
    , end_(end)
{
}

void ProcessIterator::increment()
{
    if (blockPtr == end_)
        return;
    SYSTEM_PROCESS_INFORMATION const* casted =
        reinterpret_cast<SYSTEM_PROCESS_INFORMATION const*>(&*blockPtr);
    std::size_t offset = casted->NextEntryOffset;
    if (offset == 0)
    {
        blockPtr = end_;
    }
    else
    {
        blockPtr += offset;
    }
}

bool ProcessIterator::equal(ProcessIterator const& other) const
{
    return blockPtr == other.blockPtr && end_ == other.end_;
}

Process ProcessIterator::dereference() const
{
    SYSTEM_PROCESS_INFORMATION const* casted =
        reinterpret_cast<SYSTEM_PROCESS_INFORMATION const*>(&*blockPtr);
    return Process(casted->ProcessId);
}

std::size_t Process::GetProcessId() const
{
    return id_;
}

Process::Process(std::size_t pid) : id_(pid)
{
}

static UniqueHandle OpenProc(std::size_t processId, DWORD access)
{
    UniqueHandle hProc;
    CLIENT_ID cid;
    cid.UniqueProcess = processId;
    cid.UniqueThread = 0;
    OBJECT_ATTRIBUTES attribs;
    std::memset(&attribs, 0, sizeof(attribs));
    attribs.Length = sizeof(attribs);
    NtOpenProcessFunc ntOpen =
        library::ntdll().get_function<NtOpenProcessFunc>(
        GetThrowingErrorReporter(),
        "NtOpenProcess");
    NTSTATUS errorCheck = ntOpen(hProc.Ptr(), access, &attribs, &cid);
    if (errorCheck == ERROR_SUCCESS)
    {
        return hProc;
    }
    else
    {
        Win32Exception::ThrowFromNtError(errorCheck);
    }
}

static expected<std::string>
GetProcessStr(std::size_t processId,
              RtlUserProcessParametersSelector stringTargetSelector)
{
    try
    {
        if (processId == 0)
        {
            return "System Idle Process";
        }
        if (processId == 4)
        {
            wchar_t target[MAX_PATH] = L"";
            UINT len = ::GetWindowsDirectoryW(target, MAX_PATH);
            if (len == 0)
            {
                Win32Exception::ThrowFromLastError();
            }

            --len;
            if (target[len] == L'\\')
            {
                ::wcscat_s(
                    target + len, MAX_PATH - len, L"System32\\Ntoskrnl.exe");
            }
            else
            {
                ::wcscat_s(
                    target + len, MAX_PATH - len, L"\\System32\\Ntoskrnl.exe");
            }
            return utf8::ToUtf8(target);
        }

        try
        {
            UniqueHandle hProc(OpenProc(
                processId, PROCESS_VM_READ | PROCESS_QUERY_INFORMATION));
            PROCESS_BASIC_INFORMATION basicInfo;
            NtQueryInformationProcessFunc ntQuery =
                GetNtDll().GetProcAddress<NtQueryInformationProcessFunc>(
                    "NtQueryInformationProcess");
            NTSTATUS errorCheck = ntQuery(hProc.Get(),
                                          ProcessBasicInformation,
                                          &basicInfo,
                                          sizeof(basicInfo),
                                          nullptr);
            if (errorCheck != ERROR_SUCCESS)
            {
                Win32Exception::ThrowFromNtError(errorCheck);
            }
            PEB* pebAddr = basicInfo.PebBaseAddress;
            PEB peb;
            if (::ReadProcessMemory(
                    hProc.Get(), pebAddr, &peb, sizeof(peb), nullptr) == 0)
            {
                Win32Exception::ThrowFromLastError();
            }
            RTL_USER_PROCESS_PARAMETERS params;
            if (::ReadProcessMemory(hProc.Get(),
                                    peb.ProcessParameters,
                                    &params,
                                    sizeof(params),
                                    nullptr) == 0)
            {
                Win32Exception::ThrowFromLastError();
            }
            std::wstring result;
            UNICODE_STRING& targetString = params.*stringTargetSelector;
            result.resize(targetString.Length / sizeof(wchar_t));
            if (::ReadProcessMemory(hProc.Get(),
                                    targetString.Buffer,
                                    &result[0],
                                    result.size() * sizeof(wchar_t),
                                    nullptr) == 0)
            {
                Win32Exception::ThrowFromLastError();
            }
            return utf8::ToUtf8(result);
        }
        catch (ErrorAccessDeniedException const&)
        {
            // This block is vista and later specific; however, this does not
            // matter because the
            // spurious access denied errors are being injected by Vista+'s
            // media protection
            // features.
            UniqueHandle hProc(
                OpenProc(processId, PROCESS_QUERY_LIMITED_INFORMATION));
            RuntimeDynamicLinker kernel32("Kernel32.dll");
            typedef BOOL(WINAPI * QueryFullProcessImageNameFunc)(
                HANDLE, DWORD, LPWSTR, PDWORD);
            QueryFullProcessImageNameFunc queryProcessFile =
                kernel32.GetProcAddress<QueryFullProcessImageNameFunc>(
                    "QueryFullProcessImageNameW");
            BOOL boolCheck;
            std::wstring buffer;
            DWORD goalSize = MAX_PATH;
            do
            {
                buffer.resize(goalSize);
                boolCheck =
                    queryProcessFile(hProc.Get(), 0, &buffer[0], &goalSize);
            } while (boolCheck == 0 &&
                     ::GetLastError() == ERROR_INSUFFICIENT_BUFFER);
            if (boolCheck == 0 && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                Win32Exception::ThrowFromLastError();
            }
            buffer.resize(goalSize);
            return utf8::ToUtf8(buffer);
        }
    }
    catch (...)
    {
        return expected<std::string>::from_exception();
    }
}

expected<std::string> Process::GetExecutablePath() const
{
    return GetProcessStr(GetProcessId(),
                         &RTL_USER_PROCESS_PARAMETERS::ImagePathName);
}

expected<std::string> Process::GetCmdLine() const
{
    return GetProcessStr(GetProcessId(),
                         &RTL_USER_PROCESS_PARAMETERS::CommandLine);
}

void Process::Terminate()
{
    UniqueHandle hProc(OpenProc(id_, PROCESS_TERMINATE));
    auto terminate =
        GetNtDll().GetProcAddress<NtTerminateProcessFunc>("NtTerminateProcess");
    NTSTATUS errorCheck = terminate(hProc.Get(), -1);
    if (errorCheck != ERROR_SUCCESS)
    {
        Win32Exception::ThrowFromNtError(errorCheck);
    }
}
}
}
