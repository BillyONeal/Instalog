// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include <boost/algorithm/string/predicate.hpp>
#include "../LogCommon/ScanningSections.hpp"

using namespace Instalog;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_CLASS(RunningProcessesTest)
{
    std::wostringstream ss;
    std::vector<std::wstring> options;
    ISectionDefinition const* def;
    std::wstring arg;
    void Go()
    {
        def->Execute(ss, ScriptSection(def, arg), options);
    }
    RunningProcesses rp;
public:
    RunningProcessesTest() { def = &rp; }

    TEST_METHOD_INITIALIZE(SetUp)
    {
        ss.clear();
        arg.clear();
    }

    TEST_METHOD(CommandIsrunningprocesses)
    {
        Assert::AreEqual<std::wstring>(L"runningprocesses", rp.GetScriptCommand());
    }

    TEST_METHOD(NameIsRunningProcesses)
    {
        Assert::AreEqual<std::wstring>(L"Running Processes", rp.GetName());
    }

    TEST_METHOD(PriorityIsScanning)
    {
        Assert::AreEqual<int>(SCANNING, rp.GetPriority());
    }

    TEST_METHOD(SvchostHasFullLine)
    {
        std::wstring svcHost = L"C:\\Windows\\system32\\svchost.exe -k netsvcs";
        Go();
        bool svcHostHasFullLine = boost::algorithm::icontains<std::wstring, std::wstring>(ss.str(), svcHost);
        Assert::IsTrue(svcHostHasFullLine);
    }

    TEST_METHOD(ExplorerDoesNotHaveFullLine)
    {
        std::wstring explorer = L"C:\\Windows\\Explorer.exe\n";
        Go();
        Assert::IsTrue(boost::algorithm::contains<std::wstring, std::wstring>(ss.str(), explorer));
    }

    TEST_METHOD(NtoskrnlNotPresent)
    {
        Go();
        Assert::IsFalse(boost::algorithm::contains(ss.str(), L"C:\\Windows\\System32\\Ntoskrnl.exe"));
    }
};

TEST_CLASS(ServicesDriversTest)
{
    std::wostringstream ss;
    std::vector<std::wstring> options;
    ISectionDefinition const* def;
    std::wstring arg;
    void Go()
    {
        def->Execute(ss, ScriptSection(def, arg), options);
    }
    ServicesDrivers sd;
public:
    ServicesDriversTest() { def = &sd; }

    TEST_METHOD_INITIALIZE(SetUp)
    {
        ss.clear();
        arg.clear();
    }

    TEST_METHOD(ScriptCommandIsCorrect)
    {
        Assert::AreEqual<std::wstring>(L"servicesdrivers", sd.GetScriptCommand());
    }

    TEST_METHOD(NameIsCorrect)
    {
        Assert::AreEqual<std::wstring>(L"Services/Drivers", sd.GetName());
    }

    TEST_METHOD(ActuallyGotOutput)
    {
        Go();
        Assert::IsFalse(ss.str().empty());
    }

    TEST_METHOD(TcpipWhitelisted)
    {
        Go();
        Assert::IsFalse(boost::algorithm::contains(ss.str(),
            L"R0 Tcpip;TCP/IP Protocol Driver;C:\\Windows\\System32\\Drivers\\Tcpip.sys"));
    }

    TEST_METHOD(RpcSsSvchost) 
    {
        Go();
        Assert::IsTrue(boost::algorithm::contains(ss.str(), L"R2 RpcSs;Remote Procedure Call (RPC);rpcss->C:\\Windows\\System32\\Rpcss.dll")); // This will fail if RpcSs is not configured to auto-start or is not running
    }
};

TEST_CLASS(EventViewerTest)
{
    std::wostringstream ss;
    std::vector<std::wstring> options;
    ISectionDefinition const* def;
    std::wstring arg;
    void Go()
    {
        def->Execute(ss, ScriptSection(def, arg), options);
    }
    EventViewer ev;
public:
    EventViewerTest() { def = &ev; }

    TEST_METHOD_INITIALIZE(SetUp)
    {
        ss.clear();
        arg.clear();
    }
    
    TEST_METHOD(ScriptCommandIsCorrect)
    {
        Assert::AreEqual<std::wstring>(L"eventviewer", ev.GetScriptCommand());
    }

    TEST_METHOD(NameIsCorrect)
    {
        Assert::AreEqual<std::wstring>(L"Event Viewer", ev.GetName());
    }

    TEST_METHOD(ActuallyGotOutput)
    {
        Go();
        Assert::IsFalse(ss.str().empty());
    }
};
