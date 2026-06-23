#include "MyPortIODev.h"
#include "va2pa.h"



#pragma pack(push, 1)
typedef struct _READ_MAP_REQ
{
	PVOID	PhysicalAddress;  
	ULONG   MapSize;
}READ_MAP_REQ;

typedef struct _WRITE_MAP_REQ
{
	PVOID	PhysicalAddress;
	ULONG   MapSize;        
	ULONG   WriteValue;     
} WRITE_MAP_REQ, *PWRITE_MAP_REQ;
#pragma pack(pop)

BOOLEAN 
MyPortIODev::KernelRead(
	PVOID	VirtualAddress, 
	PVOID	ReadBuffer, 
	SIZE_T	ReadSize)
{
	if (!VirtualAddress || !ReadBuffer || !ReadSize || !m_bInitialized)
	{
		return FALSE;
	}

	BOOLEAN bRet{ FALSE };

	auto nCount = static_cast<ULONG>((ReadSize + 3) / 4);

	for (auto i{ 0u }; i != nCount; ++i)
	{
		PVOID pTmpVa = reinterpret_cast<PUCHAR>(VirtualAddress) + i * 4;
		auto pPhysicalAddress = VirtualToPhysical(pTmpVa);
		if (!pPhysicalAddress)
		{
			LOG(std::format("[-] Failed to translate virtual address: {} to physical address using MemoryMap. line {}, index: {}",
							pTmpVa,
							__LINE__,
							i));
			return FALSE;
		}

		if (i != (nCount - 1))
		{
			bRet = ReadPhysicalMemory(pPhysicalAddress, 4, (PUCHAR)ReadBuffer + i * 4);
			if (!bRet)
			{
				LOG("[-] ReadPhysicalMemory failed address " << pTmpVa << "line : " << __LINE__);
				break;
			}
		}
		else
		{
			// the last 
			bRet = ReadPhysicalMemory(pPhysicalAddress, ReadSize - (nCount - 1) * 4, (PUCHAR)ReadBuffer + i * 4);
			if (!bRet)
			{
				LOG("[-] ReadPhysicalMemory failed address " << pTmpVa << "line : " << __LINE__);
				break;
			}
		}
	}


	return bRet;
}

BOOLEAN 
MyPortIODev::KernelWrite(
	PVOID	VirtualAddress,
	PVOID	WriteBuffer, 
	SIZE_T	WriteSize)
{
	if (!VirtualAddress || !WriteBuffer || !WriteSize || !m_bInitialized)
	{
		LOG("[-] Invalid parameters for KernelWrite. VirtualAddress: " << VirtualAddress << ", WriteBuffer: " << WriteBuffer << ", WriteSize: " << WriteSize);
		if (!m_bInitialized)
		{
			LOG("[-] Ensure that the driver is initialized.");
		}
		return FALSE;
	}

	BOOLEAN bRet{ FALSE };

	auto nCount = static_cast<ULONG>((WriteSize + 3) / 4);

	for (auto i{0u}; i != nCount; ++i)
	{
		PVOID pTmpVa = reinterpret_cast<PUCHAR>(VirtualAddress) + i * 4;
		auto pPhysicalAddress = VirtualToPhysical(pTmpVa);
		if (!pPhysicalAddress)
		{
			LOG(std::format("[-] Failed to translate virtual address: {} to physical address using MemoryMap. line {}, index: {}",
				pTmpVa,
				__LINE__,
				i));
			return FALSE;
		}

		if (i != (nCount - 1))
		{
			bRet = WritePhysicalMemory(pPhysicalAddress, 4, (PUCHAR)WriteBuffer + i * 4);
			if (!bRet)
			{
				LOG("[-] WritePhysicalMemory failed address " << pTmpVa << "line : " << __LINE__);
				break;
			}
		}
		else
		{
			// the last 
			bRet = WritePhysicalMemory(pPhysicalAddress, WriteSize - (nCount - 1) * 4, (PUCHAR)WriteBuffer + i * 4);
			if (!bRet)
			{
				LOG("[-] WritePhysicalMemory failed address " << pTmpVa << "line : " << __LINE__);
				break;
			}
		}
	}


	
	return bRet;
}

BOOLEAN 
MyPortIODev::ReadPhysicalMemory(
	PVOID	PhysicalAddress, 
	SIZE_T	Size, 
	PVOID	ReadBuffer)
{
	BOOLEAN bRet { FALSE };

	if (Size <= 4)
	{
		READ_MAP_REQ readMapReq{};
		readMapReq.PhysicalAddress = PhysicalAddress;
		readMapReq.MapSize = 4;
		bRet = DeviceIoControl(m_hDevice,
							   IOCTL_READ_PHYSICAL,
							   &readMapReq,
							   sizeof(readMapReq),
							   &readMapReq,
							   sizeof(readMapReq));
		
		if (!bRet)
		{
			LOG("[-] ReadPhysicalMemory failed for physical address: " << reinterpret_cast<PVOID>(reinterpret_cast<ULONG_PTR>(PhysicalAddress)) << "line £º " << __LINE__);
			return bRet;
		}

		memcpy(ReadBuffer, &readMapReq, Size);
	}
	
	return bRet;
}

BOOLEAN 
MyPortIODev::WritePhysicalMemory(
	PVOID	PhysicalAddress, 
	SIZE_T	Size, 
	PVOID	WriteBuffer)
{
	BOOLEAN bRet{ FALSE };

	// write only 4 bytes
	if (Size <= 4)
	{
		WRITE_MAP_REQ writeMapReq{};

		writeMapReq.PhysicalAddress = PhysicalAddress;
		writeMapReq.MapSize = 4;
		
		memcpy(&writeMapReq.WriteValue, WriteBuffer, Size);

		ULONG output{};
		bRet = DeviceIoControl(m_hDevice,
							   IOCTL_WRITE_PHYSICAL,
							   &writeMapReq,
							   sizeof(writeMapReq),
							   &output,
							   sizeof(output));
		if (!bRet)
		{
			LOG("[-] WritePhysicalMemory failed for physical address: " << reinterpret_cast<PVOID>(reinterpret_cast<ULONG_PTR>(PhysicalAddress)));
			return bRet;
		}
	}
	
	return bRet;
}

PVOID 
MyPortIODev::VirtualToPhysical(PVOID VirtualAddress)
{
	return Va2Pa(VirtualAddress);
}
