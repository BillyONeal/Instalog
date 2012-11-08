// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <cstring>
#include <functional>
#include <windows.h>
#include "Win32Exception.hpp"
#include "Library.hpp"
#include "ScopeExit.hpp"
#include "ScopedPrivilege.hpp"
#include "Process.hpp"
#include "DdkStructures.h"

namespace Instalog { namespace SystemFacades {
    
    ProcessEnumerator::ProcessEnumerator()
    {
        NtQuerySystemInformationFunc ntQuerySysInfo = GetNtDll().GetProcAddress<NtQuerySystemInformationFunc>("NtQuerySystemInformation");
        NTSTATUS errorCheck = STATUS_INFO_LENGTH_MISMATCH;
        ULONG goalLength = 0;
        while(errorCheck == STATUS_INFO_LENGTH_MISMATCH)
        {
            if (goalLength == 0)
            {
                informationBlock.resize(informationBlock.size() + 1024);
            }
            else
            {
                informationBlock.resize(goalLength);
            }
            errorCheck = ntQuerySysInfo(
                SystemProcessInformation,
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


    ProcessIterator::ProcessIterator( std::vector<unsigned char>::const_iterator start, std::vector<unsigned char>::const_iterator end )
        : blockPtr(start)
        , end_(end)
    { }

    void ProcessIterator::increment()
    {
        if (blockPtr == end_)
            return;
        SYSTEM_PROCESS_INFORMATION const *casted = reinterpret_cast<SYSTEM_PROCESS_INFORMATION const*>(&*blockPtr);
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

    bool ProcessIterator::equal( ProcessIterator const& other ) const
    {
        return blockPtr == other.blockPtr && end_ == other.end_;
    }

    Process ProcessIterator::dereference() const
    {
        SYSTEM_PROCESS_INFORMATION const *casted = reinterpret_cast<SYSTEM_PROCESS_INFORMATION const*>(&*blockPtr);
        return Process(casted->ProcessId);
    }


    std::size_t Process::GetProcessId() const
    {
        return id_;
    }

    Process::Process( std::size_t pid )
        : id_(pid)
    { }

    static std::wstring GetProcessStr(std::size_t processId, std::function<UNICODE_STRING&(RTL_USER_PROCESS_PARAMETERS&)> stringTargetSelector)
    {
        if (processId == 0)
        {
            return L"System Idle Process";
        }
        if (processId == 4)
        {
            wchar_t target[MAX_PATH] = L"";
            UINT len = ::GetWindowsDirectoryW(target, MAX_PATH) - 1;
            if (target[len] == L'\\')
            {
                ::wcscat_s(target + len, MAX_PATH - len, L"System32\\Ntoskrnl.exe");
            }
            else
            {
                ::wcscat_s(target + len, MAX_PATH - len, L"\\System32\\Ntoskrnl.exe");
            }
            return target;
        }
        NtOpenProcessFunc ntOpen = GetNtDll().GetProcAddress<NtOpenProcessFunc>("NtOpenProcess");
        NtQueryInformationProcessFunc ntQuery = GetNtDll().GetProcAddress<NtQueryInformationProcessFunc>("NtQueryInformationProcess");
        CLIENT_ID cid;
        cid.UniqueProcess = processId;
        cid.UniqueThread = 0;
        HANDLE hProc = INVALID_HANDLE_VALUE;
        OBJECT_ATTRIBUTES attribs;
        std::memset(&attribs, 0, sizeof(attribs));
        attribs.Length = sizeof(attribs);
        try
        {
            NTSTATUS errorCheck = ntOpen(&hProc, PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, &attribs, &cid);
            ScopeExit se([hProc] () { CloseHandle(hProc); });
            if (errorCheck != ERROR_SUCCESS)
            {
                Win32Exception::ThrowFromNtError(errorCheck);
            }
            PROCESS_BASIC_INFORMATION basicInfo;
            errorCheck = ntQuery(hProc, ProcessBasicInformation, &basicInfo, sizeof(basicInfo), nullptr);
            PEB *pebAddr = basicInfo.PebBaseAddress;
            PEB peb;
            if (::ReadProcessMemory(hProc, pebAddr, &peb, sizeof(peb), nullptr) == 0)
            {
                Win32Exception::ThrowFromLastError();
            }
            RTL_USER_PROCESS_PARAMETERS params;
            if (::ReadProcessMemory(hProc, peb.ProcessParameters, &params, sizeof(params), nullptr) == 0)
            {
                Win32Exception::ThrowFromLastError();
            }
            std::wstring result;
            UNICODE_STRING &targetString = stringTargetSelector(params);
            result.resize(targetString.Length / sizeof(wchar_t));
            if (::ReadProcessMemory(hProc, targetString.Buffer, &result[0], result.size() * sizeof(wchar_t), nullptr) == 0)
            {
                Win32Exception::ThrowFromLastError();
            }
            return result;
        }
        catch (ErrorAccessDeniedException const&)
        {
            //This block is vista and later specific; however, this does not matter because the
            //spurious access denied errors are being injected by Vista+'s media protection
            //features.
            NTSTATUS errorCheck = ntOpen(&hProc, PROCESS_QUERY_LIMITED_INFORMATION, &attribs, &cid);
            ScopeExit se([hProc] () { CloseHandle(hProc); });
            if (errorCheck != ERROR_SUCCESS)
            {
                Win32Exception::ThrowFromNtError(errorCheck);
            }
            RuntimeDynamicLinker kernel32(L"Kernel32.dll");
            typedef BOOL (WINAPI *QueryFullProcessImageNameFunc)(HANDLE, DWORD, LPWSTR, PDWORD);
            QueryFullProcessImageNameFunc queryProcessFile = kernel32.GetProcAddress<QueryFullProcessImageNameFunc>("QueryFullProcessImageNameW");
            BOOL boolCheck;
            std::wstring buffer;
            DWORD goalSize = MAX_PATH;
            do
            {
                buffer.resize(goalSize);
                boolCheck = queryProcessFile(hProc, 0, &buffer[0], &goalSize);
            } while (boolCheck == 0 && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER);
            if (boolCheck == 0 && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                Win32Exception::ThrowFromLastError();
            }
            buffer.resize(goalSize);
            return buffer;
        }
    }

    std::wstring Process::GetExecutablePath() const
    {
        return GetProcessStr(GetProcessId(), [](RTL_USER_PROCESS_PARAMETERS& params) -> UNICODE_STRING& {
            return params.ImagePathName;
        });
    }

    std::wstring Process::GetCmdLine() const
    {
        return GetProcessStr(GetProcessId(), [](RTL_USER_PROCESS_PARAMETERS& params) -> UNICODE_STRING& {
            return params.CommandLine;
        });
    }

}}
