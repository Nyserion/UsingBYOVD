#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include "Log.hpp"

struct RelocInfo
{
	PVOID	Address;
	USHORT* Item;
	ULONG	Count;
};

struct ImportFunctionInfo
{
	std::string FunctionName;
	ULONG64* FunctionAddress;
};

struct ExportFunctionInfo
{
	std::string FunctionName;
	PVOID		FunctionAddress;
};

struct ImportInfo
{
	std::string ModuleName;
	std::vector<ImportFunctionInfo> VecFunctions;
};

class PEUtils
{
public:
	PEUtils(PVOID Base) : m_Base(Base)
	{

	}

	~PEUtils() = default;

public:
	VOID	FixedRelocations(ULONG64 Delta);
	BOOLEAN FixedImports();
	BOOLEAN FixedSecurityCookie(ULONG64 MapKernelBase);

public:
	auto GetDriverImageSize() -> DWORD
	{
		auto ntHeaders = GetNtHeaders();
		if (!ntHeaders)
		{
			LOG("[-] Failed to get NT headers." << std::endl);
			return 0;
		}
		return ntHeaders->OptionalHeader.SizeOfImage;
	}

	auto GetDriverImageBase() -> ULONG64
	{
		auto ntHeaders = GetNtHeaders();
		if (!ntHeaders)
		{
			LOG("[-] Failed to get NT headers." << std::endl);
			return 0;
		}
		return ntHeaders->OptionalHeader.ImageBase;
	}

	auto GetSizeOfHeaders() -> DWORD
	{
		auto ntHeaders = GetNtHeaders();
		if (!ntHeaders)
		{
			LOG("[-] Failed to get NT headers." << std::endl);
			return 0;
		}
		return ntHeaders->OptionalHeader.SizeOfHeaders;
	}

	auto GetEntryPoint() -> DWORD
	{
		auto ntHeaders = GetNtHeaders();
		if (!ntHeaders)
		{
			LOG("[-] Failed to get NT headers." << std::endl);
			return 0;
		}
		return ntHeaders->OptionalHeader.AddressOfEntryPoint;
	}

	std::vector<IMAGE_SECTION_HEADER> GetSectionHeaders()
	{
		if (!m_ntHeaders)
		{
			GetNtHeaders();
		}
		if (!m_ntHeaders)
		{
			LOG("[-] Failed to get NT headers." << std::endl);
			return {};
		}
		if (!m_vecSectionHeaders.empty())
		{
			return m_vecSectionHeaders;
		}
		PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(m_ntHeaders);
		for (WORD i = 0; i < m_ntHeaders->FileHeader.NumberOfSections; ++i)
		{
			m_vecSectionHeaders.push_back(sectionHeader[i]);
		}
		return m_vecSectionHeaders;
	}

protected:
	std::vector<RelocInfo> GetBaseRelocations()
	{
		if (!m_ntHeaders)
		{
			GetNtHeaders();
		}
		if (!m_ntHeaders)
		{
			return {};
		}
		if (!m_vecBaseRelocations.empty())
		{
			return m_vecBaseRelocations;
		}
		DWORD relocDirRVA = m_ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
		if (!relocDirRVA)
		{
			//LOG("[-] No relocation directory found." << std::endl);
			return {};
		}

		auto size = m_ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
		PIMAGE_BASE_RELOCATION reloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(reinterpret_cast<BYTE*>(m_Base) + relocDirRVA);
		auto endOfReloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(reinterpret_cast<BYTE*>(reloc) + size);

		// Each block of base relocation contains a header (IMAGE_BASE_RELOCATION) and an array of USHORTs that specify the type and offset of each relocation entry.
		// The SizeOfBlock field in the header indicates the total size of the block, including the header and the array of USHORTs. 
		// The VirtualAddress field specifies the base address for the relocations in that block.
		// The loop continues until it reaches the end of the relocation data or encounters a block with a SizeOfBlock of zero, which indicates the end of the relocation entries.
		// The relocation entries are processed by calculating the actual address to be fixed up using the VirtualAddress and the offset specified in each USHORT entry.
		while (reloc < endOfReloc && reloc->SizeOfBlock)
		{
			RelocInfo relocInfor{};

			relocInfor.Address = reinterpret_cast<PVOID>(reinterpret_cast<BYTE*>(m_Base) + reloc->VirtualAddress);
			relocInfor.Item = reinterpret_cast<USHORT*>(reinterpret_cast<BYTE*>(reloc) + sizeof(IMAGE_BASE_RELOCATION));
			relocInfor.Count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(USHORT);

			m_vecBaseRelocations.push_back(relocInfor);

			// NEXT
			reloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(reinterpret_cast<BYTE*>(reloc) + reloc->SizeOfBlock);
		}
		return m_vecBaseRelocations;
	}


	PIMAGE_NT_HEADERS64 GetNtHeaders()
	{
		if (m_ntHeaders)
		{
			return m_ntHeaders;
		}
		if (!m_Base)
		{
			LOG("[-] Base address is null." << std::endl);
			return nullptr;
		}

		PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(m_Base);
		if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		{
			LOG("[-] Invalid DOS signature." << std::endl);
			return nullptr;
		}
		m_ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS64>(reinterpret_cast<BYTE*>(m_Base) + dosHeader->e_lfanew);
		if (m_ntHeaders->Signature != IMAGE_NT_SIGNATURE)
		{
			LOG("[-] Invalid NT signature." << std::endl);
			return nullptr;
		}
		return m_ntHeaders;
	}


	std::vector<ImportInfo> GetImportDescriptors()
	{
		if (!m_ntHeaders)
		{
			GetNtHeaders();
		}
		if (!m_ntHeaders)
		{
			LOG("[-] Failed to get NT headers." << std::endl);
			return {};
		}
		if (!m_vecImportDescriptors.empty())
		{
			return m_vecImportDescriptors;
		}

		DWORD importDirRVA = m_ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
		if (!importDirRVA)
		{
			LOG("[-] No import directory found." << std::endl);
			return {};
		}

		auto importDescriptor = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(reinterpret_cast<BYTE*>(m_Base) + importDirRVA);
		while (importDescriptor->FirstThunk)
		{
			ImportInfo importInfo{};
			importInfo.ModuleName = std::string(reinterpret_cast<char*>(reinterpret_cast<ULONG64>(m_Base) + importDescriptor->Name));


			auto currentFirstThunk = reinterpret_cast<PIMAGE_THUNK_DATA64>(reinterpret_cast<ULONG64>(m_Base) + importDescriptor->FirstThunk);
			auto currentOriginalFirstThunk = reinterpret_cast<PIMAGE_THUNK_DATA64>(reinterpret_cast<ULONG64>(m_Base) + importDescriptor->OriginalFirstThunk);
			while (currentOriginalFirstThunk->u1.Function)
			{
				ImportFunctionInfo impFuncInfo{};

				auto thunkData = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(reinterpret_cast<ULONG64>(m_Base) + currentOriginalFirstThunk->u1.AddressOfData);
				impFuncInfo.FunctionName = std::string(thunkData->Name);
				impFuncInfo.FunctionAddress = &currentFirstThunk->u1.Function;

				importInfo.VecFunctions.push_back(impFuncInfo);

				++currentOriginalFirstThunk;
				++currentFirstThunk;
			}

			m_vecImportDescriptors.push_back(importInfo);
			importDescriptor++;
		}
		return m_vecImportDescriptors;
	}



private:
	PVOID								m_Base{ nullptr };
	PIMAGE_NT_HEADERS64					m_ntHeaders{ nullptr };
	std::vector<IMAGE_SECTION_HEADER>	m_vecSectionHeaders{};
	std::vector<ImportInfo>				m_vecImportDescriptors{};
	std::vector<RelocInfo>				m_vecBaseRelocations{};
};

