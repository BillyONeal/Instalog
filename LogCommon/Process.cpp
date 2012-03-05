#include <cstring>
#include <windows.h>
#include "Win32Exception.hpp"
#include "RuntimeDynamicLinker.hpp"
#include "HandleCloser.hpp"
#include "ScopedPrivilege.hpp"
#include "Process.hpp"

namespace {

#define NTSTATUS LONG

#define MAX_UNICODE_PATH	32767L

typedef DWORD ACCESS_MASK;

typedef struct T_LSA_UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} LSA_UNICODE_STRING, 
	*PLSA_UNICODE_STRING, 
	UNICODE_STRING, 
	*PUNICODE_STRING;


typedef struct T_OBJECT_ATTRIBUTES {
	ULONG  Length;
	HANDLE  RootDirectory;
	PUNICODE_STRING  ObjectName;
	ULONG  Attributes;
	PVOID  SecurityDescriptor;
	PVOID  SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
typedef CONST OBJECT_ATTRIBUTES *PCOBJECT_ATTRIBUTES;

typedef struct T_CLIENT_ID {
	std::size_t UniqueProcess;
	std::size_t	UniqueThread;
} CLIENT_ID, * PCLIENT_ID;

typedef struct T_VM_COUNTERS {
	ULONG PeakVirtualSize;
	ULONG VirtualSize;
	ULONG PageFaultCount;
	ULONG PeakWorkingSetSize;
	ULONG WorkingSetSize;
	ULONG QuotaPeakPagedPoolUsage;
	ULONG QuotaPagedPoolUsage;
	ULONG QuotaPeakNonPagedPoolUsage;
	ULONG QuotaNonPagedPoolUsage;
	ULONG PagefileUsage;
	ULONG PeakPagefileUsage;
} VM_COUNTERS, * PVM_COUNTERS;


typedef NTSTATUS (NTAPI *NtOpenProcessFunc)(
	OUT PHANDLE,
	IN ACCESS_MASK,
	IN POBJECT_ATTRIBUTES,
	IN PCLIENT_ID OPTIONAL);

typedef enum T_SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation,
	SystemProcessorInformation,
	SystemPerformanceInformation,
	SystemTimeOfDayInformation,
	SystemPathInformation,
	SystemProcessInformation,
	SystemCallCountInformation,
	SystemDeviceInformation,
	SystemProcessorPerformanceInformation,
	SystemFlagsInformation,
	SystemCallTimeInformation,
	SystemModuleInformation,
	SystemLocksInformation,
	SystemStackTraceInformation,
	SystemPagedPoolInformation,
	SystemNonPagedPoolInformation,
	SystemHandleInformation,
	SystemObjectInformation,
	SystemPageFileInformation,
	SystemVdmInstemulInformation,
	SystemVdmBopInformation,
	SystemFileCacheInformation,
	SystemPoolTagInformation,
	SystemInterruptInformation,
	SystemDpcBehaviorInformation,
	SystemFullMemoryInformation,
	SystemLoadGdiDriverInformation,
	SystemUnloadGdiDriverInformation,
	SystemTimeAdjustmentInformation,
	SystemSummaryMemoryInformation,
	SystemNextEventIdInformation,
	SystemEventIdsInformation,
	SystemCrashDumpInformation,
	SystemExceptionInformation,
	SystemCrashDumpStateInformation,
	SystemKernelDebuggerInformation,
	SystemContextSwitchInformation,
	SystemRegistryQuotaInformation,
	SystemExtendServiceTableInformation,
	SystemPrioritySeperation,
	SystemPlugPlayBusInformation,
	SystemDockInformation,
	SystemPowerInformation,
	SystemProcessorSpeedInformation,
	SystemCurrentTimeZoneInformation,
	SystemLookasideInformation
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

typedef NTSTATUS (NTAPI *NtQuerySystemInformationFunc) (
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID               SystemInformation,
	IN ULONG                SystemInformationLength,
	OUT PULONG              ReturnLength OPTIONAL );

typedef struct T_CURDIR
{
	UNICODE_STRING DosPath;
	PVOID Handle;
} CURDIR, *PCURDIR;

typedef struct T_RTL_DRIVE_LETTER_CURDIR {
	USHORT Flags;
	USHORT Length;
	ULONG TimeStamp; 
	UNICODE_STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct T_RTL_USER_PROCESS_PARAMETERS
{
	ULONG MaximumLength;
	ULONG Length;
	ULONG Flags;
	ULONG DebugFlags;
	PVOID ConsoleHandle;
	ULONG ConsoleFlags;
	PVOID StandardInput;
	PVOID StandardOutput;
	PVOID StandardError;
	CURDIR CurrentDirectory;
	UNICODE_STRING DllPath;
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
	PVOID Environment;
	ULONG StartingX;
	ULONG StartingY;
	ULONG CountX;
	ULONG CountY;
	ULONG CountCharsX;
	ULONG CountCharsY;
	ULONG FillAttribute;
	ULONG WindowFlags;
	ULONG ShowWindowFlags;
	UNICODE_STRING WindowTitle;
	UNICODE_STRING DesktopInfo;
	UNICODE_STRING ShellInfo;
	UNICODE_STRING RuntimeData;
	RTL_DRIVE_LETTER_CURDIR CurrentDirectores[32];
	ULONG EnvironmentSize;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef LONG KPRIORITY;

typedef enum T_KWAIT_REASON
{
	Executive = 0,
	FreePage = 1,
	PageIn = 2,
	PoolAllocation = 3,
	DelayExecution = 4,
	Suspended = 5,
	UserRequest = 6,
	WrExecutive = 7,
	WrFreePage = 8,
	WrPageIn = 9,
	WrPoolAllocation = 10,
	WrDelayExecution = 11,
	WrSuspended = 12,
	WrUserRequest = 13,
	WrEventPair = 14,
	WrQueue = 15,
	WrLpcReceive = 16,
	WrLpcReply = 17,
	WrVirtualMemory = 18,
	WrPageOut = 19,
	WrRendezvous = 20,
	Spare2 = 21,
	Spare3 = 22,
	Spare4 = 23,
	Spare5 = 24,
	WrCalloutStack = 25,
	WrKernel = 26,
	WrResource = 27,
	WrPushLock = 28,
	WrMutex = 29,
	WrQuantumEnd = 30,
	WrDispatchInt = 31,
	WrPreempted = 32,
	WrYieldExecution = 33,
	WrFastMutex = 34,
	WrGuardedMutex = 35,
	WrRundown = 36,
	MaximumWaitReason = 37
} KWAIT_REASON;

typedef struct T_SYSTEM_THREAD {
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER CreateTime;
	ULONG WaitTime;
	PVOID StartAddress;
	CLIENT_ID ClientId;
	KPRIORITY Priority;
	LONG BasePriority;
	ULONG ContextSwitchCount;
	ULONG State;
	KWAIT_REASON WaitReason;
} SYSTEM_THREAD, *PSYSTEM_THREAD;


typedef struct T_SYSTEM_PROCESS_INFORMATION {

	ULONG                   NextEntryOffset;
	ULONG                   NumberOfThreads;
	LARGE_INTEGER           Reserved[3];
	LARGE_INTEGER           CreateTime;
	LARGE_INTEGER           UserTime;
	LARGE_INTEGER           KernelTime;
	UNICODE_STRING          ImageName;
	KPRIORITY               BasePriority;
	std::size_t             ProcessId;
	std::size_t             InheritedFromProcessId;
	ULONG                   HandleCount;
	ULONG                   Reserved2[2];
	ULONG                   PrivatePageCount;
	VM_COUNTERS             VirtualMemoryCounters;
	IO_COUNTERS             IoCounters;
	SYSTEM_THREAD           Threads; //[]
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;


typedef struct T_PEB_LDR_DATA {
	ULONG Length;
	BOOLEAN Initialized;
	PVOID SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct T_PEB_FREE_BLOCK {
	struct _PEB_FREE_BLOCK *Next;
	ULONG Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

typedef struct T_PEB {
	BOOLEAN InheritedAddressSpace;
	BOOLEAN ReadImageFileExecOptions;
	BOOLEAN BeingDebugged;
	BOOLEAN Spare;
	HANDLE Mutant;
	PVOID ImageBaseAddress;
	PPEB_LDR_DATA LoaderData;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	PVOID SubSystemData;
	PVOID ProcessHeap;
	PVOID FastPebLock;
	PVOID FastPebLockRoutine;
	PVOID FastPebUnlockRoutine;
	ULONG EnvironmentUpdateCount;
	PVOID *KernelCallbackTable;
	PVOID EventLogSection;
	PVOID EventLog;
	PPEB_FREE_BLOCK FreeList;
	ULONG TlsExpansionCounter;
	PVOID TlsBitmap;
	ULONG TlsBitmapBits[0x2];
	PVOID ReadOnlySharedMemoryBase;
	PVOID ReadOnlySharedMemoryHeap;
	PVOID *ReadOnlyStaticServerData;
	PVOID AnsiCodePageData;
	PVOID OemCodePageData;
	PVOID UnicodeCaseTableData;
	ULONG NumberOfProcessors;
	ULONG NtGlobalFlag;
	BYTE Spare2[0x4];
	LARGE_INTEGER CriticalSectionTimeout;
	ULONG HeapSegmentReserve;
	ULONG HeapSegmentCommit;
	ULONG HeapDeCommitTotalFreeThreshold;
	ULONG HeapDeCommitFreeBlockThreshold;
	ULONG NumberOfHeaps;
	ULONG MaximumNumberOfHeaps;
	PVOID *ProcessHeaps;
	PVOID GdiSharedHandleTable;
	PVOID ProcessStarterHelper;
	PVOID GdiDCAttributeList;
	PVOID LoaderLock;
	ULONG OSMajorVersion;
	ULONG OSMinorVersion;
	ULONG OSBuildNumber;
	ULONG OSPlatformId;
	ULONG ImageSubSystem;
	LONG ImageSubSystemMajorVersion;
	ULONG ImageSubSystemMinorVersion;
	ULONG GdiHandleBuffer[0x22];
	ULONG PostProcessInitRoutine;
	ULONG TlsExpansionBitmap;
	BYTE TlsExpansionBitmapBits[0x80];
	ULONG SessionId;
} PEB, *PPEB;


typedef struct T_PROCESS_BASIC_INFORMATION {
	PVOID Reserved1;
	PPEB PebBaseAddress;
	PVOID Reserved2[2];
	ULONG_PTR UniqueProcessId;
	PVOID Reserved3;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef NTSTATUS (NTAPI *NtCloseFunc)(HANDLE);

typedef NTSTATUS (NTAPI *NtOpenProcessTokenFunc)(
	IN HANDLE,
	IN ACCESS_MASK,
	OUT PHANDLE ); 

typedef NTSTATUS (NTAPI *NtAdjustPrivilegesTokenFunc)(
	IN HANDLE,
	IN BOOLEAN,
	IN PTOKEN_PRIVILEGES,
	IN ULONG,
	OUT PTOKEN_PRIVILEGES OPTIONAL,
	OUT PULONG OPTIONAL ); 

typedef enum T_PROCESS_INFORMATION_CLASS {
	ProcessBasicInformation,
	ProcessQuotaLimits,
	ProcessIoCounters,
	ProcessVmCounters,
	ProcessTimes,
	ProcessBasePriority,
	ProcessRaisePriority,
	ProcessDebugPort,
	ProcessExceptionPort,
	ProcessAccessToken,
	ProcessLdtInformation,
	ProcessLdtSize,
	ProcessDefaultHardErrorMode,
	ProcessIoPortHandlers,
	ProcessPooledUsageAndLimits,
	ProcessWorkingSetWatch,
	ProcessUserModeIOPL,
	ProcessEnableAlignmentFaultFixup,
	ProcessPriorityClass,
	ProcessWx86Information,
	ProcessHandleCount,
	ProcessAffinityMask,
	ProcessPriorityBoost,
	MaxProcessInfoClass
} PROCESS_INFORMATION_CLASS, *PPROCESS_INFORMATION_CLASS;


typedef NTSTATUS (NTAPI *NtQueryInformationProcessFunc)(
	IN HANDLE,
	IN PROCESS_INFORMATION_CLASS,
	OUT PVOID,
	IN ULONG,
	OUT PULONG); 

typedef NTSTATUS (NTAPI *NtTerminateProcessFunc)(
	IN HANDLE,
	IN NTSTATUS
	);

#define STATUS_INFO_LENGTH_MISMATCH	((NTSTATUS)0xC0000004L)

}

namespace Instalog { namespace SystemFacades {
	
	ProcessEnumerator::ProcessEnumerator()
	{
		NtQuerySystemInformationFunc ntQuerySysInfo = ntdll.GetProcAddress<NtQuerySystemInformationFunc>("NtQuerySystemInformation");
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
				&informationBlock[0],
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

	std::wstring Process::GetExecutablePath() const
	{
		if (GetProcessId() == 0)
		{
			return L"System Idle Process";
		}
		if (GetProcessId() == 4)
		{
			wchar_t target[MAX_PATH] = L"";
			::ExpandEnvironmentStringsW(L"%WINDIR%\\System32\\Ntoskrnl.exe", target, MAX_PATH);
			return target;
		}
		ScopedPrivilege privilegeHolder(SE_DEBUG_NAME);
		NtOpenProcessFunc ntOpen = ntdll.GetProcAddress<NtOpenProcessFunc>("NtOpenProcess");
		NtQueryInformationProcessFunc ntQuery = ntdll.GetProcAddress<NtQueryInformationProcessFunc>("NtQueryInformationProcess");
		CLIENT_ID cid;
		cid.UniqueProcess = GetProcessId();
		cid.UniqueThread = 0;
		HANDLE hProc = INVALID_HANDLE_VALUE;
		OBJECT_ATTRIBUTES attribs;
		std::memset(&attribs, 0, sizeof(attribs));
		attribs.Length = sizeof(attribs);
		NTSTATUS errorCheck = ntOpen(&hProc, PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, &attribs, &cid);
		std::unique_ptr<void, HandleCloser> handleCloser(hProc);
		if (errorCheck != ERROR_SUCCESS)
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		PROCESS_BASIC_INFORMATION basicInfo;
		errorCheck = ntQuery(hProc, ProcessBasicInformation, &basicInfo, sizeof(basicInfo), nullptr);
		PEB *pebAddr = basicInfo.PebBaseAddress;
		PEB peb;
		::ReadProcessMemory(hProc, pebAddr, &peb, sizeof(peb), nullptr);
		RTL_USER_PROCESS_PARAMETERS params;
		::ReadProcessMemory(hProc, peb.ProcessParameters, &params, sizeof(params), nullptr);
		std::wstring result;
		result.resize(params.ImagePathName.Length / sizeof(wchar_t));
		::ReadProcessMemory(hProc, params.ImagePathName.Buffer, &result[0], result.size() * sizeof(wchar_t), nullptr);
		return result;
	}

	std::wstring Process::GetCmdLine() const
	{
		if (GetProcessId() == 0)
		{
			return L"System Idle Process";
		}
		if (GetProcessId() == 4)
		{
			wchar_t target[MAX_PATH] = L"";
			::ExpandEnvironmentStringsW(L"%WINDIR%\\System32\\Ntoskrnl.exe", target, MAX_PATH);
			return target;
		}
		ScopedPrivilege privilegeHolder(SE_DEBUG_NAME);
		NtOpenProcessFunc ntOpen = ntdll.GetProcAddress<NtOpenProcessFunc>("NtOpenProcess");
		NtQueryInformationProcessFunc ntQuery = ntdll.GetProcAddress<NtQueryInformationProcessFunc>("NtQueryInformationProcess");
		CLIENT_ID cid;
		cid.UniqueProcess = GetProcessId();
		cid.UniqueThread = 0;
		HANDLE hProc = INVALID_HANDLE_VALUE;
		OBJECT_ATTRIBUTES attribs;
		std::memset(&attribs, 0, sizeof(attribs));
		attribs.Length = sizeof(attribs);
		NTSTATUS errorCheck = ntOpen(&hProc, PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, &attribs, &cid);
		std::unique_ptr<void, HandleCloser> handleCloser(hProc);
		if (errorCheck != ERROR_SUCCESS)
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		PROCESS_BASIC_INFORMATION basicInfo;
		errorCheck = ntQuery(hProc, ProcessBasicInformation, &basicInfo, sizeof(basicInfo), nullptr);
		PEB *pebAddr = basicInfo.PebBaseAddress;
		PEB peb;
		::ReadProcessMemory(hProc, pebAddr, &peb, sizeof(peb), nullptr);
		RTL_USER_PROCESS_PARAMETERS params;
		::ReadProcessMemory(hProc, peb.ProcessParameters, &params, sizeof(params), nullptr);
		std::wstring result;
		result.resize(params.CommandLine.Length / sizeof(wchar_t));
		::ReadProcessMemory(hProc, params.CommandLine.Buffer, &result[0], result.size() * sizeof(wchar_t), nullptr);
		return result;
	}

}}
