#pragma once

#ifndef _RECOVERY_H
#define _RECOVERY_H

typedef enum ICMD_STATE {
	ICMD_ZERO,
	ICMD_SENT_IBSS,
	ICMD_SENT_EXPLOIT,
	ICMD_SENT_IBEC,
	ICMD_SENT_RAMDISK,
	ICMD_SENT_DEVICETREE,
	ICMD_SENT_KERNELCACHE,
} ICMD_STATE;

extern ICMD_STATE g_icmdState;

void recovery_callback(RECOVERY_CALLBACK_REASON reason, AMRecoveryModeDevice device, void* ctx);

#endif
