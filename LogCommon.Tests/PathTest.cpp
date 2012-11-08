// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include <string>
#include <iterator>
#include <boost/algorithm/string/split.hpp>
#include "../LogCommon/Win32Exception.hpp"
#include "../LogCommon/File.hpp"
#include "../LogCommon/Path.hpp"

using namespace Instalog::Path;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_CLASS(PathAppendTest)
{
public:
    TEST_METHOD(NoSlashes)
    {
        Assert::AreEqual<std::wstring>(L"one\\two", Append(L"one", L"two"));
    }

    TEST_METHOD(PathHasSlashes)
    {
        Assert::AreEqual<std::wstring>(L"one\\two", Append(L"one\\", L"two"));
    }

    TEST_METHOD(MoreHasSlashes)
    {
        Assert::AreEqual<std::wstring>(L"one\\two", Append(L"one", L"\\two"));
    }

    TEST_METHOD(BothHaveSlashes)
    {
        Assert::AreEqual<std::wstring>(L"one\\two", Append(L"one\\", L"\\two"));
    }

    TEST_METHOD( PathHasManySlashes)
    {
        Assert::AreEqual<std::wstring>(L"one\\\\\\two", Append(L"one\\\\\\", L"two"));
    }

    TEST_METHOD(MoreHasManySlashes)
    {
        Assert::AreEqual<std::wstring>(L"one\\\\\\two", Append(L"one", L"\\\\\\two"));
    }

    TEST_METHOD(BothHaveManySlashes)
    {
        Assert::AreEqual<std::wstring>(L"one\\\\\\\\\\two", Append(L"one\\\\\\", L"\\\\\\two"));
    }

    TEST_METHOD(MoreIsEmpty)
    {
        Assert::AreEqual<std::wstring>(L"one", Append(L"one", L""));
    }

    TEST_METHOD(PathIsEmpty)
    {
        Assert::AreEqual<std::wstring>(L"two", Append(L"", L"two"));
    }

    TEST_METHOD(BothAreEmpty)
    {
        Assert::AreEqual<std::wstring>(L"", Append(L"", L""));
    }

    TEST_METHOD(AppendFailure)
    {
        Assert::AreEqual<std::wstring>(L"c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\bin\\amd64\\ExampleFileDoesNotExistFindMeFindMeFindMeFindMe.found",
            Append(L"c:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\bin\\amd64", L"ExampleFileDoesNotExistFindMeFindMeFindMeFindMe.found"));
    }
};

TEST_CLASS(PathPrettifyTests)
{
public:
    TEST_METHOD(CorrectOutput)
    {
        std::wstring path(L"C:\\ExAmPlE\\FooBar\\Target.EXE");
        Prettify(path.begin(), path.end());
        Assert::AreEqual<std::wstring>(L"C:\\Example\\Foobar\\Target.exe", path);
    }

    TEST_METHOD(DriveCapitalized)
    {
        std::wstring path(L"c:\\Example\\Foobar\\Target.exe");
        Prettify(path.begin(), path.end());
        Assert::AreEqual<std::wstring>(L"C:\\Example\\Foobar\\Target.exe", path);
    }

    TEST_METHOD(SpacesOkay)
    {
        std::wstring path(L"C:\\Example\\Foo bar\\Target.exe");
        Prettify(path.begin(), path.end());
        Assert::AreEqual<std::wstring>(L"C:\\Example\\Foo bar\\Target.exe", path);
    }
};

static void TestExpansion(std::wstring const& expected, std::wstring source, bool expectedReturn = true)
{
    Assert::AreEqual(expectedReturn, ExpandShortPath(source));
    Assert::AreEqual(expected, source);
}

TEST_CLASS(PathExpandingTests)
{
public:
    TEST_METHOD(DirectoryExpansion)
    {
        TestExpansion(L"C:\\Program Files\\", L"C:\\Progra~1\\");
    }

    TEST_METHOD(NonExistantDirectoryExpansion)
    {
        TestExpansion(L"C:\\zzzzz~1\\", L"C:\\zzzzz~1\\", false);
    }

    TEST_METHOD(FileExpansion)
    {
        HANDLE hFile = ::CreateFileW(L"Temporary Long Path File", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            std::wcout << ::GetLastError() << std::endl;
        }

        TestExpansion(L"Temporary Long Path File", L"Tempor~1", true);

        ::CloseHandle(hFile);
    }

    TEST_METHOD(NonExistantFileExpansion)
    {
        TestExpansion(L"zzzzzz~1", L"zzzzzz~1", false);
    }
};

static void TestResolve(std::wstring const& expected, std::wstring source, bool expectedReturn = true)
{
    Assert::AreEqual(expectedReturn, ResolveFromCommandLine(source));
    Assert::AreEqual(expected, source);
}

TEST_CLASS(PathResolutionTests)
{
public:
    TEST_METHOD(EmptyGivesEmpty)
    {
        TestResolve(L"", L"", false);
    }

    TEST_METHOD(DoesNotExistUnchanged)
    {
        TestResolve(L"C:\\Windows\\DOESNOTEXIST\\DOESNOTEXIST\\GAHIDONTKNOWWHATSGOINGTOHAPPEN\\Explorer.exe", L"C:\\Windows\\DOESNOTEXIST\\DOESNOTEXIST\\GAHIDONTKNOWWHATSGOINGTOHAPPEN\\Explorer.exe", false);
    }

    TEST_METHOD(CanonicalPathUnchanged)
    {
        TestResolve(L"C:\\Windows\\Explorer.exe", L"C:\\Windows\\Explorer.exe");
    }

    TEST_METHOD(NativePathsCanonicalized)
    {
        TestResolve(L"C:\\Windows\\Explorer.exe", L"\\??\\C:\\Windows\\Explorer.exe");
    }

    TEST_METHOD(NtPathsCanonicalized)
    {
        TestResolve(L"C:\\Windows\\Explorer.exe", L"\\\\?\\C:\\Windows\\Explorer.exe");
    }

    TEST_METHOD(GlobalrootRemoved)
    {
        TestResolve(L"C:\\Windows\\Explorer.exe", L"globalroot\\C:\\Windows\\Explorer.exe");
    }

    TEST_METHOD(SlashGlobalrootRemoved)
    {
        TestResolve(L"C:\\Windows\\Explorer.exe", L"\\globalroot\\C:\\Windows\\Explorer.exe");
    }

    TEST_METHOD(System32Replaced)
    {
        TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"system32\\Ntoskrnl.exe");
    }

    TEST_METHOD(System32SlashedReplaced)
    {
        TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"\\system32\\Ntoskrnl.exe");
    }

    TEST_METHOD(WindDirReplaced)
    {
        TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"\\systemroot\\system32\\Ntoskrnl.exe");
    }

    TEST_METHOD(DefaultKernelFoundOnPath)
    {
        TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"ntoskrnl");
    }

    TEST_METHOD(AddsExtension)
    {
        TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"C:\\Windows\\System32\\Ntoskrnl");
    }

    TEST_METHOD(CombinePhaseOneAndTwo)
    {
        TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"\\globalroot\\System32\\Ntoskrnl");
    }

    TEST_METHOD(RundllVundo)
    {
        TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"rundll32 ntoskrnl,ShellExecute");
    }

    TEST_METHOD(RundllVundoSpaces)
    {
        TestResolve(L"C:\\Windows\\System32\\Ntoskrnl.exe", L"rundll32                                                 ntoskrnl,ShellExecute");
    }

    TEST_METHOD(QuotedPath)
    {
        TestResolve(L"C:\\Program files\\Windows nt\\Accessories\\Wordpad.exe", L"\"C:\\Program Files\\Windows nt\\Accessories\\Wordpad.exe\"  Arguments Arguments Arguments");
    }

    TEST_METHOD(QuotedPathRundll)
    {
        HANDLE hFile = ::CreateFileW(L"C:\\ExampleTestingFile.exe", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
        TestResolve(L"C:\\Exampletestingfile.exe", L"\"C:\\Windows\\System32\\Rundll32.exe\" \"C:\\ExampleTestingFile.exe,Argument arg arg\"");
        ::CloseHandle(hFile);
    }
};

TEST_CLASS(PathResolutionPathOrder)
{
    std::vector<std::wstring> pathItems;
    std::wstring const fileName;
public:
    PathResolutionPathOrder()
        : fileName(L"ExampleFileDoesNotExistFindMeFindMeFindMeFindMe.found")
    { }
    
    TEST_METHOD_INITIALIZE(SetUp)
    {
        using namespace std::placeholders;
        std::wstring pathBuffer;
        DWORD pathLen = ::GetEnvironmentVariableW(L"PATH", nullptr, 0);
        pathBuffer.resize(pathLen);
        ::GetEnvironmentVariable(L"PATH", &pathBuffer[0], pathLen);
        pathBuffer.pop_back(); //remove null
        boost::algorithm::split(pathItems, pathBuffer, std::bind1st(std::equal_to<wchar_t>(), L';'));
        Assert::IsTrue(3ul <= pathItems.size());
        std::transform(pathItems.begin(), pathItems.end(), pathItems.begin(),
            std::bind(Append, _1, fileName));
        std::for_each(pathItems.begin(), pathItems.end(), [] (std::wstring& a) { Prettify(a.begin(), a.end()); });
    }

    TEST_METHOD(NoCreateFails)
    {
        TestResolve(fileName, fileName, false);
    }

    TEST_METHOD(SeesLastPathItem)
    {
        HANDLE hFile = ::CreateFileW(pathItems.back().c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
        TestResolve(pathItems.back(), fileName);
        ::CloseHandle(hFile);
    }

    TEST_METHOD(RespectsPathOrder)
    {
        HANDLE hFile = ::CreateFileW(pathItems[1].c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
        HANDLE hFile2 = ::CreateFileW(pathItems[2].c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
        TestResolve(pathItems[1], fileName);
        ::CloseHandle(hFile);
        ::CloseHandle(hFile2);
    }
};

TEST_CLASS(PathResolutionPathExtOrder)
{
    std::vector<std::wstring> pathItems;
    std::wstring const fileName;
public:
    PathResolutionPathExtOrder()
        : fileName(L"ExampleFileDoesNotExistFindMeFindMeFindMeFindMeExt")
    { }
    TEST_METHOD_INITIALIZE(SetUp)
    {
        using namespace std::placeholders;
        std::wstring pathBuffer;
        DWORD pathLen = ::GetEnvironmentVariableW(L"PATHEXT", nullptr, 0);
        pathBuffer.resize(pathLen);
        ::GetEnvironmentVariable(L"PATHEXT", &pathBuffer[0], pathLen);
        pathBuffer.pop_back(); //remove null
        boost::algorithm::split(pathItems, pathBuffer, std::bind1st(std::equal_to<wchar_t>(), L';'));
        Assert::IsTrue(3u <= pathItems.size());
        std::for_each(pathItems.begin(), pathItems.end(), [this] (std::wstring& a) { a.insert(0, fileName); } );
        std::transform(pathItems.begin(), pathItems.end(), pathItems.begin(),
            std::bind(Append, L"C:\\Windows\\System32", _1));
        std::for_each(pathItems.begin(), pathItems.end(), [] (std::wstring& a) { Prettify(a.begin(), a.end()); });
    }

    TEST_METHOD(NoCreateFails)
    {
        TestResolve(fileName, fileName, false);
    }

    TEST_METHOD(SeesLastPathItem)
    {
        HANDLE hFile = ::CreateFileW(pathItems.back().c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
        TestResolve(pathItems.back(), fileName);
        ::CloseHandle(hFile);
    }

    TEST_METHOD(RespectsPathExtOrder)
    {
        HANDLE hFile = ::CreateFileW(pathItems[1].c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
        HANDLE hFile2 = ::CreateFileW(pathItems[2].c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
        TestResolve(pathItems[1], fileName);
        ::CloseHandle(hFile);
        ::CloseHandle(hFile2);
    }
};
