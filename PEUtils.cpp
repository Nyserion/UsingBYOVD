#include "PEUtils.hpp"
#include "KernelUtils.hpp"

//************************************
// Method:    FixedRelocations
// FullName:  PEUtils::FixedRelocations
// Access:    public 
// Returns:   VOID
// Qualifier:
// Parameter: ULONG64 Delta
// Delta : MapperDriverBase(Using ExAllocatePoolWithTag allocate driver size) - 
// OriginalImageBase(NtHeaders.OptionalHeader.ImageBase)
//************************************
VOID PEUtils::FixedRelocations(ULONG64 Delta)
{
	auto baseRelocations = GetBaseRelocations();
	if (!baseRelocations.size())
	{
		//LOG("[*] No relocation directory found, skipping relocation fixing." << std::endl);
		return;
	}

	for (auto& reloc : baseRelocations)
	{
		for (auto i{ 0 }; i != reloc.Count; ++i)
		{
			unsigned short type = reloc.Item[i] >> 12;
			unsigned short offset = reloc.Item[i] & 0xFFF;

			if (type == IMAGE_REL_BASED_DIR64)
			{
				*reinterpret_cast<ULONG64*>(reinterpret_cast<BYTE*>(reloc.Address) + offset) += Delta;
				//LOG("[+] Fixed relocation at address: " << std::hex << reinterpret_cast<ULONG64>(reloc.Address) + offset << std::dec << " with delta: " << std::hex << Delta << std::dec << std::endl);
			}
		}
	}
}

BOOLEAN PEUtils::FixedImports()
{
	auto importDescriptors = GetImportDescriptors();
	if (!importDescriptors.size())
	{
		//LOG("[*] No import directory found, skipping import fixing." << std::endl);
		return FALSE;
	}

	for (auto& importInfo : importDescriptors)
	{
		auto moduleBase = g_KernelUtils->GetKernelModuleBase(importInfo.ModuleName);
		if (!moduleBase)
		{
			LOG("[-] Failed to get kernel module base for " << importInfo.ModuleName << std::endl);
			return FALSE;
		}
		/*	else
			{
				LOG("[+] Found kernel module: " << importInfo.ModuleName << " at base address: " << std::hex << moduleBase << std::dec << std::endl);
			}*/

		for (auto& funcInfo : importInfo.VecFunctions)
		{
			auto funcOffsets = g_KernelUtils->GetExportFunctionOffset(funcInfo.FunctionName);
			if (!funcOffsets)
			{
				LOG("[-] Failed to get export function offset for: " << funcInfo.FunctionName << std::endl);
				return FALSE;
			}
			*funcInfo.FunctionAddress = funcOffsets + (ULONG64)moduleBase;

		}
	}

	return TRUE;
}

BOOLEAN PEUtils::FixedSecurityCookie(ULONG64 MapKernelBase)
{
	if (!MapKernelBase)
	{
		return FALSE;
	}

	// find load config table
	if (!m_ntHeaders)
	{
		GetNtHeaders();
	}

	auto loadConfigVa = m_ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress;
	if (!loadConfigVa)
	{
		// not to fixed it
		return TRUE;
	}

	auto loadConfigData = reinterpret_cast<PIMAGE_LOAD_CONFIG_DIRECTORY>(reinterpret_cast<PUCHAR>(m_Base) + loadConfigVa);
	if (!loadConfigData)
	{
		return TRUE;
	}
	auto SecurityCookie = loadConfigData->SecurityCookie;
	if (!SecurityCookie)
	{
		return TRUE;
	}

	SecurityCookie += reinterpret_cast<ULONG64>(m_Base) - MapKernelBase;

	if (*(ULONG64*)(SecurityCookie) != 0x2B992DDFA232LL)
	{
		return FALSE;
	}

	ULONG64 newCookie = 0x2B992DDFA232LL ^ GetCurrentProcessId() ^ GetCurrentThreadId();
	if (newCookie == 0x2B992DDFA232LL)
	{
		newCookie = 0x2B992DDFA233LL;
	}

	*(ULONG64*)(SecurityCookie) = newCookie;

	return TRUE;
}
