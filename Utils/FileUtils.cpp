#include "FileUtils.hpp"
#include <vector>
#include <fstream>
#include <array>
#include <random>
#include "Utils.hpp"

#include "Log.hpp"

#if _DEBUG
#include <iostream>
#endif 

static
std::string 
CreateRandomFileName(const size_t length = 16)
{
	static const char letters[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	static std::random_device rd;
	static std::mt19937 gen(rd());                  
	std::uniform_int_distribution<> dis(0, sizeof(letters) - 2);

	std::string name;
	name.reserve(length);

	for (size_t i = 0; i < length; ++i)
	{
		name += letters[dis(gen)];
	}

	return name;
}

static 
std::string 
CreateDriver(HANDLE& hFile)
{
	namespace fs = std::filesystem;

	try
	{
		auto tempPath = fs::temp_directory_path();
		if (tempPath.empty())
		{
			LOG("[-] Failed to get temporary directory path");
			return {};
		}

		std::string randomName = CreateRandomFileName(12);
		if (randomName.empty())
		{
			randomName = "tempfile_" + std::to_string(std::time(nullptr));
		}

		fs::path fullPath = tempPath / randomName;
		fullPath.replace_extension(".sys");

		std::wstring win32Path = fullPath.wstring();

		std::wstring ntPath = L"\\??\\" + win32Path;

		UNICODE_STRING uniPath {};
		RtlInitUnicodeString(&uniPath, ntPath.c_str());

		OBJECT_ATTRIBUTES objAttr {};
		InitializeObjectAttributes(&objAttr, &uniPath,
								   OBJ_CASE_INSENSITIVE,
								   nullptr, nullptr);

		IO_STATUS_BLOCK ioStatus{};
		HANDLE hTmpFile { nullptr };

		NTSTATUS status = Utils::NtCreateFile(&hTmpFile,
											  FILE_GENERIC_WRITE | SYNCHRONIZE | DELETE,
											  &objAttr,
											  &ioStatus,
											  nullptr,
											  FILE_ATTRIBUTE_NORMAL,
											  FILE_SHARE_READ | FILE_SHARE_WRITE,
											  FILE_CREATE,
											  FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
											  nullptr,
											  0);

		if (!NT_SUCCESS(status))
		{
			LOG("[-] SC_NtCreateFile failed: 0x" << std::hex << status << std::dec);
			return {};
		}

		//LOG("[+] Driver file created successfully, Handle: 0x" << std::hex << hTmpFile << std::dec);
		hFile = hTmpFile;
		return fullPath.string();
	}
	catch (const std::exception& e)
	{
#if _DEBUG
		LOG("[-] Exception in CreateDriverFile: " << e.what());
#endif
		return {};
	}
}
static
bool 
CreateFileFromMemory(
	const HANDLE&			fileHandle,
	const char*				data, 
	const size_t			size, 
	const unsigned char		key)
{
	if (!fileHandle || data == nullptr || size == 0)
	{
		LOG("[-] Invalid parameters for CreateFileFromMemory");
		return false;
	}

	std::vector<char> buffer(data, data + size);

	for (auto& vecData : buffer)
	{
		vecData ^= key;
	}

	IO_STATUS_BLOCK ioStatus{};
	auto status = Utils::NtWriteFile(fileHandle,
									 nullptr,
									 nullptr,
									 nullptr,
									 &ioStatus,
									 buffer.data(),
									 buffer.size(),
									 nullptr,
									 nullptr);
	if (!NT_SUCCESS(status))
	{
		LOG("[-] NtWriteFile failed: 0x" << std::hex << status << std::dec);
		return false;
	}

	return true;
}

bool FileUtils::CreateDriverFile(
	std::string&		driverFullPath,
	const char*			filedata, 
	const size_t		size, 
	const unsigned char key)
{
	HANDLE hFile{ nullptr };
	auto driverPath = CreateDriver(hFile);
	if (!hFile || driverPath.empty())
	{
		LOG("[-] Failed to create driver file	");
		return false;
	}
	
	driverFullPath = driverPath;
	auto bResult = CreateFileFromMemory(hFile, filedata, size, key);

	if (hFile)
	{
		SC_NtClose(hFile);
		hFile = nullptr;
	}

	return bResult;
}

std::string 
FileUtils::GetServiceName(
	const unsigned char*	data, 
	const size_t			size, 
	const unsigned char		key)
{
	if (data == nullptr || size == 0)
	{
		return {};
	}

	std::vector<unsigned char> buffer(data, data + size);
	for (auto& vecData : buffer)
	{
		vecData ^= key;
	}
	return std::string(buffer.begin(), buffer.end());
}
