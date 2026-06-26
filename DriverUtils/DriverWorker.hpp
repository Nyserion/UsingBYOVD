#pragma once
#include <windows.h>


namespace DriverWorker
{
	auto InitializeDriver()->BOOLEAN;

	VOID UninitializeDriver();

	auto Read(PVOID VirtualAddress, PVOID ReadBuffer, ULONG Size)->BOOLEAN;

	auto Write(PVOID VirtualAddress, PVOID WriteBuffer, ULONG Size)->BOOLEAN;

	auto KillerInit()->BOOLEAN;

	auto Kill(ULONG Pid)->BOOLEAN;

	VOID KillUnInit();
}

