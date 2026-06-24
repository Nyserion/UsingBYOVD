#include "HWAudioX64.h"

BOOLEAN HWAudioX64::KillProcess(ULONG Pid)
{
	return DeviceIoControl(m_hDevice,
						   IOCTL_KILL_PROCESS,
						   &Pid,
						   sizeof(Pid),
						   nullptr,
						   0);
}

