#pragma once
#include <windows.h>
#include <string>
#include "DriverProvider.hpp"
#include "DriverService.hpp"
#include "Singleton.hpp"
#include "ObjectProxy.hpp"
#include "MyPortIODevBin.hpp"


// DON'T USE THIS DRIVER TO MAPPING UNSIGNED DRIVER ,IT TOO SLOWLY.

class MyPortIODev final :
	public DriverProvider<MyPortIODev>,
	public Singleton<MyPortIODev>
{
	friend class Singleton<MyPortIODev>;
private:
	static constexpr ULONG IOCTL_READ_PHYSICAL	= 0x9C406680u;
	static constexpr ULONG IOCTL_WRITE_PHYSICAL = 0x9C40A684u;
public:
	explicit MyPortIODev(Token) noexcept : MyPortIODev()
	{
	}
	~MyPortIODev() = default;

	BOOLEAN InitDriver() noexcept
	{
		return Initialize(MyPortIODevBin::hexData,
						  MyPortIODevBin::hexSize,
						  MyPortIODevBin::service,
						  MyPortIODevBin::serviceLength,
						  MyPortIODevBin::Key);
	}

	BOOLEAN
		KernelRead(PVOID	VirtualAddress,
				   PVOID	ReadBuffer,
				   SIZE_T	ReadSize);
	BOOLEAN
		KernelWrite(PVOID	VirtualAddress,
					PVOID	WriteBuffer,
					SIZE_T	WriteSize);

private:
	MyPortIODev() = default;

private:
	BOOLEAN
		ReadPhysicalMemory(
			PVOID	PhysicalAddress,
			SIZE_T	Size,
			PVOID	ReadBuffer);

	BOOLEAN
		WritePhysicalMemory(
			PVOID	PhysicalAddress,
			SIZE_T	Size,
			PVOID	WriteBuffe);

	PVOID
		VirtualToPhysical(PVOID VirtualAddress);

};

inline constexpr ObjectProxy<MyPortIODev> g_MyPortIODev{};