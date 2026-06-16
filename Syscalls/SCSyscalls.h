#pragma once
#ifndef SC_SYSCALLS_H
#define SC_SYSCALLS_H

#include <windows.h>
#include <winternl.h>

#ifndef NT_SUCCESS
# define NT_SUCCESS(Status)  (((NTSTATUS)(Status)) >= 0)
#endif

#ifndef STATUS_SUCCESS
# define STATUS_SUCCESS      ((NTSTATUS)0x00000000L)
#endif

#ifndef STATUS_ACCESS_DENIED
# define STATUS_ACCESS_DENIED ((NTSTATUS)0xC0000022L)
#endif

#ifndef STATUS_INFO_LENGTH_MISMATCH
# define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
#endif

/* ---------- Memory information class ------------------------------------- */
#ifndef _MEMORY_INFORMATION_CLASS
typedef enum _MEMORY_INFORMATION_CLASS
{
    MemoryBasicInformation = 0,
    MemoryWorkingSetInformation = 1,
    MemoryMappedFilenameInformation = 2,
    MemoryRegionInformation = 3,
    MemoryWorkingSetExInformation = 4,
    MemorySharedCommitInformation = 5,
    MemoryImageInformation = 6,
} MEMORY_INFORMATION_CLASS;
#endif

typedef CLIENT_ID* PCLIENT_ID;


/* ---------- Section / MapView types --------------------------------------- */
#ifndef _SECTION_INHERIT
typedef enum _SECTION_INHERIT
{
    ViewShare = 1,
    ViewUnmap = 2,
} SECTION_INHERIT;
#endif

#ifndef _WAIT_TYPE
typedef enum _WAIT_TYPE
{
    WaitAll = 0,
    WaitAny = 1,
} WAIT_TYPE;
#endif

/* ---------- PS attributes / create info ---------------------------------- */
#ifndef _PS_ATTRIBUTE_LIST
typedef struct _PS_ATTRIBUTE
{
    ULONG_PTR Attribute;
    SIZE_T    Size;
    union
    {
        ULONG_PTR Value;
        PVOID     ValuePtr;
    };
    PSIZE_T   ReturnLength;
} PS_ATTRIBUTE, * PPS_ATTRIBUTE;

typedef struct _PS_ATTRIBUTE_LIST
{
    SIZE_T       TotalLength;
    PS_ATTRIBUTE Attributes[1];
} PS_ATTRIBUTE_LIST, * PPS_ATTRIBUTE_LIST;
#endif

#ifndef _PS_CREATE_INFO
typedef struct _PS_CREATE_INFO
{
    SIZE_T  Size;
    ULONG   State;
    union
    {
        struct
        {
            ULONG  InitFlags;
            ACCESS_MASK AdditionalFileAccess;
        } InitState;
        struct
        {
            HANDLE FileHandle;
        } FailSection;
        struct
        {
            USHORT DllCharacteristics;
        } ExeFormat;
        struct
        {
            HANDLE IFEOKey;
        } ExeName;
        struct
        {
            ULONG  OutputFlags;
            ULONG  Flags;
            HANDLE FileHandle;
            HANDLE SectionHandle;
            ULONGLONG UserProcessParametersNative;
            ULONG  UserProcessParametersWow64;
            ULONG  CurrentParameterFlags;
            ULONGLONG PebAddressNative;
            ULONG  PebAddressWow64;
            ULONGLONG ManifestAddress;
            ULONG  ManifestSize;
        } SuccessState;
    };
} PS_CREATE_INFO, * PPS_CREATE_INFO;
#endif

/* ---------- APC       -------------------------------------------- */
typedef VOID(NTAPI* PPS_APC_ROUTINE)(
    PVOID ApcArgument1,
    PVOID ApcArgument2,
    PVOID ApcArgument3
    );


/* ---------- Timer types (for sleep encryption) ----------------------------- */
#ifndef _TIMER_TYPE
typedef enum _TIMER_TYPE
{
    NotificationTimer = 0,
    SynchronizationTimer = 1,
} TIMER_TYPE;
#endif

/* =========================================================================
 *  SC Internal structures used by the runtime
 * ========================================================================= */

 /* Entry in the static SSN lookup table */
typedef struct _SC_SSN_ENTRY
{
    DWORD  Count;
    struct
    {
        DWORD Build;
        DWORD Ssn;
    } Entries[64];
} SC_SSN_ENTRY;

/* Export entry used during FreshyCalls / Hell's Gate scanning */
typedef struct _SC_EXPORT
{
    PVOID Address;
    DWORD Hash;
    DWORD Ordinal;
} SC_EXPORT, * PSC_EXPORT;

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 *  Runtime initialization
 *  RecycledGate -- FreshyCalls + opcode validation
 * ========================================================================= */
EXTERN_C BOOL SC_Initialize(VOID);

EXTERN_C BOOL SC_PatchEtw(VOID);   /* Optionally patch user-mode ETW writer */
EXTERN_C BOOL SC_PatchAmsi(VOID);  /* Patch AmsiScanBuffer */
EXTERN_C BOOL SC_UnhookNtdll(VOID); /* Remap clean ntdll .text section */
EXTERN_C BOOL SC_AntiDebugCheck(VOID); /* Check for debugger presence */

/* =========================================================================
 *  Syscall function prototypes
 * ========================================================================= */
EXTERN_C NTSTATUS NTAPI SC_NtWriteFile(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);
EXTERN_C NTSTATUS NTAPI SC_NtContinue(PCONTEXT ThreadContext, BOOLEAN RaiseAlert);
EXTERN_C NTSTATUS NTAPI SC_NtQueueApcThread(HANDLE ThreadHandle, PPS_APC_ROUTINE ApcRoutine, PVOID ApcArgument1, PVOID ApcArgument2, PVOID ApcArgument3);
EXTERN_C NTSTATUS NTAPI SC_NtQueryInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
EXTERN_C NTSTATUS NTAPI SC_NtOpenProcessTokenEx(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess, ULONG HandleAttributes, PHANDLE TokenHandle);
EXTERN_C NTSTATUS NTAPI SC_NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus);
EXTERN_C NTSTATUS NTAPI SC_NtUnmapViewOfSection(HANDLE ProcessHandle, PVOID BaseAddress);
EXTERN_C NTSTATUS NTAPI SC_NtOpenThread(PHANDLE ThreadHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PCLIENT_ID ClientId);
EXTERN_C NTSTATUS NTAPI SC_NtCreateProcess(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, HANDLE ParentProcess, BOOLEAN InheritObjectTable, HANDLE SectionHandle, HANDLE DebugPort, HANDLE TokenHandle);
EXTERN_C NTSTATUS NTAPI SC_NtImpersonateThread(HANDLE ServerThreadHandle, HANDLE ClientThreadHandle, PSECURITY_QUALITY_OF_SERVICE SecurityQos);
EXTERN_C NTSTATUS NTAPI SC_NtAdjustPrivilegesToken(HANDLE TokenHandle, BOOLEAN DisableAllPrivileges, PTOKEN_PRIVILEGES NewState, ULONG BufferLength, PTOKEN_PRIVILEGES PreviousState, PULONG ReturnLength);
EXTERN_C NTSTATUS NTAPI SC_NtWaitForSingleObject(HANDLE Handle, BOOLEAN Alertable, PLARGE_INTEGER Timeout);
EXTERN_C NTSTATUS NTAPI SC_NtAlertResumeThread(HANDLE ThreadHandle, PULONG PreviousSuspendCount);
EXTERN_C NTSTATUS NTAPI SC_NtOpenFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions);
EXTERN_C NTSTATUS NTAPI SC_NtWaitForMultipleObjects(ULONG Count, PHANDLE Handles, WAIT_TYPE WaitType, BOOLEAN Alertable, PLARGE_INTEGER Timeout);
EXTERN_C NTSTATUS NTAPI SC_NtWriteVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T NumberOfBytesToWrite, PSIZE_T NumberOfBytesWritten);
EXTERN_C NTSTATUS NTAPI SC_NtTestAlert();
EXTERN_C NTSTATUS NTAPI SC_NtRollbackTransaction(HANDLE TransactionHandle, BOOLEAN Wait);
EXTERN_C NTSTATUS NTAPI SC_NtSetTimer(HANDLE TimerHandle, PLARGE_INTEGER DueTime, PVOID TimerApcRoutine, PVOID TimerContext, BOOLEAN ResumeTimer, LONG Period, PBOOLEAN PreviousState);
EXTERN_C NTSTATUS NTAPI SC_NtClose(HANDLE Handle);
EXTERN_C NTSTATUS NTAPI SC_NtAlertThread(HANDLE ThreadHandle);
EXTERN_C NTSTATUS NTAPI SC_NtCreateUserProcess(PHANDLE ProcessHandle, PHANDLE ThreadHandle, ACCESS_MASK ProcessDesiredAccess, ACCESS_MASK ThreadDesiredAccess, POBJECT_ATTRIBUTES ProcessObjectAttributes, POBJECT_ATTRIBUTES ThreadObjectAttributes, ULONG ProcessFlags, ULONG ThreadFlags, PVOID ProcessParameters, PPS_CREATE_INFO CreateInfo, PPS_ATTRIBUTE_LIST AttributeList);
EXTERN_C NTSTATUS NTAPI SC_NtUnloadDriver(PUNICODE_STRING Description);
EXTERN_C NTSTATUS NTAPI SC_NtSetEvent(HANDLE EventHandle, PLONG PreviousState);
EXTERN_C NTSTATUS NTAPI SC_NtMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID* BaseAddress, ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset, PSIZE_T ViewSize, SECTION_INHERIT InheritDisposition, ULONG AllocationType, ULONG Win32Protect);
EXTERN_C NTSTATUS NTAPI SC_NtDelayExecution(BOOLEAN Alertable, PLARGE_INTEGER DelayInterval);
EXTERN_C NTSTATUS NTAPI SC_NtSuspendThread(HANDLE ThreadHandle, PULONG PreviousSuspendCount);
EXTERN_C NTSTATUS NTAPI SC_NtOpenProcessToken(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess, PHANDLE TokenHandle);
EXTERN_C NTSTATUS NTAPI SC_NtFlushInstructionCache(HANDLE ProcessHandle, PVOID BaseAddress, SIZE_T Length);
EXTERN_C NTSTATUS NTAPI SC_NtCreateProcessEx(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, HANDLE ParentProcess, ULONG Flags, HANDLE SectionHandle, HANDLE DebugPort, HANDLE TokenHandle, ULONG Reserved);
EXTERN_C NTSTATUS NTAPI SC_NtDeviceIoControlFile(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG IoControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);
EXTERN_C NTSTATUS NTAPI SC_NtSetInformationVirtualMemory(HANDLE ProcessHandle, ULONG VmInformationClass, ULONG_PTR NumberOfEntries, PVOID VirtualAddresses, PVOID VmInformation, ULONG VmInformationLength);
EXTERN_C NTSTATUS NTAPI SC_NtProtectVirtualMemory(HANDLE ProcessHandle, PVOID* BaseAddress, PSIZE_T RegionSize, ULONG NewProtect, PULONG OldProtect);
EXTERN_C NTSTATUS NTAPI SC_NtQueryInformationThread(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength, PULONG ReturnLength);
EXTERN_C NTSTATUS NTAPI SC_NtGetContextThread(HANDLE ThreadHandle, PCONTEXT ThreadContext);
EXTERN_C NTSTATUS NTAPI SC_NtOpenProcess(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PCLIENT_ID ClientId);
EXTERN_C NTSTATUS NTAPI SC_NtSetInformationThread(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength);
EXTERN_C NTSTATUS NTAPI SC_NtQueryInformationToken(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, PVOID TokenInformation, ULONG TokenInformationLength, PULONG ReturnLength);
EXTERN_C NTSTATUS NTAPI SC_NtQueueApcThreadEx(HANDLE ThreadHandle, HANDLE ReserveHandle, PPS_APC_ROUTINE ApcRoutine, PVOID ApcArgument1, PVOID ApcArgument2, PVOID ApcArgument3);
EXTERN_C NTSTATUS NTAPI SC_NtAllocateVirtualMemoryEx(HANDLE ProcessHandle, PVOID* BaseAddress, PSIZE_T RegionSize, ULONG AllocationType, ULONG PageProtection, PVOID ExtendedParameters, ULONG ExtendedParameterCount);
EXTERN_C NTSTATUS NTAPI SC_NtDuplicateToken(HANDLE ExistingTokenHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, BOOLEAN EffectiveOnly, TOKEN_TYPE TokenType, PHANDLE NewTokenHandle);
EXTERN_C NTSTATUS NTAPI SC_NtDuplicateObject(HANDLE SourceProcessHandle, HANDLE SourceHandle, HANDLE TargetProcessHandle, PHANDLE TargetHandle, ACCESS_MASK DesiredAccess, ULONG HandleAttributes, ULONG Options);
EXTERN_C NTSTATUS NTAPI SC_NtOpenThreadToken(HANDLE ThreadHandle, ACCESS_MASK DesiredAccess, BOOLEAN OpenAsSelf, PHANDLE TokenHandle);
EXTERN_C NTSTATUS NTAPI SC_NtQueryVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID MemoryInformation, SIZE_T MemoryInformationLength, PSIZE_T ReturnLength);
EXTERN_C NTSTATUS NTAPI SC_NtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);
EXTERN_C NTSTATUS NTAPI SC_NtCommitTransaction(HANDLE TransactionHandle, BOOLEAN Wait);
EXTERN_C NTSTATUS NTAPI SC_NtCreateSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PLARGE_INTEGER MaximumSize, ULONG SectionPageProtection, ULONG AllocationAttributes, HANDLE FileHandle);
EXTERN_C NTSTATUS NTAPI SC_NtAllocateVirtualMemory(HANDLE ProcessHandle, PVOID* BaseAddress, ULONG_PTR ZeroBits, PSIZE_T RegionSize, ULONG AllocationType, ULONG Protect);
EXTERN_C NTSTATUS NTAPI SC_NtCreateTimer(PHANDLE TimerHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, TIMER_TYPE TimerType);
EXTERN_C NTSTATUS NTAPI SC_NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
EXTERN_C NTSTATUS NTAPI SC_NtDeleteFile(POBJECT_ATTRIBUTES ObjectAttributes);
EXTERN_C NTSTATUS NTAPI SC_NtCreateEvent(PHANDLE EventHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, ULONG EventType, BOOLEAN InitialState);
EXTERN_C NTSTATUS NTAPI SC_NtQueryObject(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);
EXTERN_C NTSTATUS NTAPI SC_NtCreateTransaction(PHANDLE TransactionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PVOID Uow, HANDLE TmHandle, ULONG CreateOptions, ULONG IsolationLevel, ULONG IsolationFlags, PLARGE_INTEGER Timeout, PUNICODE_STRING Description);
EXTERN_C NTSTATUS NTAPI SC_NtFreeVirtualMemory(HANDLE ProcessHandle, PVOID* BaseAddress, PSIZE_T RegionSize, ULONG FreeType);
EXTERN_C NTSTATUS NTAPI SC_NtSignalAndWaitForSingleObject(HANDLE SignalHandle, HANDLE WaitHandle, BOOLEAN Alertable, PLARGE_INTEGER Timeout);
EXTERN_C NTSTATUS NTAPI SC_NtReadFile(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);
EXTERN_C NTSTATUS NTAPI SC_NtLoadDriver(PUNICODE_STRING Description);
EXTERN_C NTSTATUS NTAPI SC_NtSetContextThread(HANDLE ThreadHandle, PCONTEXT ThreadContext);
EXTERN_C NTSTATUS NTAPI SC_NtCreateThreadEx(PHANDLE ThreadHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, HANDLE ProcessHandle, PVOID StartRoutine, PVOID Argument, ULONG CreateFlags, SIZE_T ZeroBits, SIZE_T StackSize, SIZE_T MaximumStackSize, PPS_ATTRIBUTE_LIST AttributeList);
EXTERN_C NTSTATUS NTAPI SC_NtSuspendProcess(HANDLE ProcessHandle);
EXTERN_C NTSTATUS NTAPI SC_NtOpenSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);
EXTERN_C NTSTATUS NTAPI SC_NtReadVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T NumberOfBytesToRead, PSIZE_T NumberOfBytesRead);
EXTERN_C NTSTATUS NTAPI SC_NtTerminateThread(HANDLE ThreadHandle, NTSTATUS ExitStatus);
EXTERN_C NTSTATUS NTAPI SC_NtResumeProcess(HANDLE ProcessHandle);
EXTERN_C NTSTATUS NTAPI SC_NtResetEvent(HANDLE EventHandle, PLONG PreviousState);
EXTERN_C NTSTATUS NTAPI SC_NtResumeThread(HANDLE ThreadHandle, PULONG SuspendCount);
EXTERN_C NTSTATUS NTAPI SC_NtSetInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength);

#ifdef __cplusplus
}
#endif

#endif /* SC_SYSCALLS_H */
