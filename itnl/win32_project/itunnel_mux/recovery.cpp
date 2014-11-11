#include "main.h"

ICMD_STATE g_icmdState = ICMD_ZERO;

void dfu_connect_callback(AMRecoveryModeDevice device) 
{
	Log(LOG_DEBUG, "dfu_connect_callback");
	if (*g_ibss) {
		int result = libmd_builtin_uploadFileDfu(device, g_ibss);
		Log(LOG_INFO, "iBSS %s loaded: %u", g_ibss, result);
	}
	g_icmdState = ICMD_SENT_IBSS;
}

void recovery_connect_callback_autoboot(AMRecoveryModeDevice device) 
{
	Log(LOG_DEBUG, "recovery_connect_callback_autoboot");
	libmd_set_autoboot(device, true);
}

int uploadFile(AMRecoveryModeDevice device, const char* fileName)
{
	Log(LOG_DEBUG, "uploadFile: g_builtinApi: %u; libmd_private_api_located: %u", g_builtinApi, libmd_private_api_located);	
	if (g_builtinApi || !libmd_private_api_located) {
		if (!g_builtinApi) 
			Log(LOG_ERROR, "uploadFile: falling back to builtin implementation, update iTunes or 4.1+ firmware won't be supported ..");
		return libmd_builtin_uploadFile(device, fileName);
	} else
		return call_AMRecoveryModeDeviceSendFileToDevice(device, fileName);
}

void recovery_connect_callback_icmd(AMRecoveryModeDevice device) 
{
	Log(LOG_DEBUG, "recovery_connect_callback_icmd");

	if (g_icmdState == ICMD_ZERO) {
		if (!*g_ibss) {
			g_icmdState = ICMD_SENT_IBSS;
		}
	}
	if (g_icmdState == ICMD_SENT_IBSS)  {
		if (*g_exploit) {
			libmd_builtin_uploadUsbExploit(device, g_exploit);
			Log(LOG_INFO, "Payload %s sent", g_exploit);
		}
		g_icmdState = ICMD_SENT_EXPLOIT;
	}
	if (g_icmdState == ICMD_SENT_EXPLOIT)  {
		g_icmdState = ICMD_SENT_IBEC;
		if (*g_ibec) {
			uploadFile(device, g_ibec);
			libmd_builtin_sendCommand(device, g_goCmd);
			Log(LOG_INFO, "iBEC %s loaded", g_ibec);
			return; // continue after reconnect
		}
	}
	if (g_icmdState == ICMD_SENT_IBEC) {
		if (*g_ramdisk) {
			uploadFile(device, g_ramdisk);
			Sleep(1000 * g_ramdiskDelay);
			libmd_builtin_sendCommand(device, g_ramdiskCmd);
			Sleep(1000 * g_ramdiskDelay);
			Log(LOG_INFO, "Ramdisk %s loaded", g_ramdisk);
		}
		g_icmdState = ICMD_SENT_RAMDISK;
	}
	if (g_icmdState == ICMD_SENT_RAMDISK) {
		if (*g_devicetree) {
			uploadFile(device, g_devicetree);
			libmd_builtin_sendCommand(device, "devicetree");
			Log(LOG_INFO, "Devicetree %s loaded", g_devicetree);
		}
		g_icmdState = ICMD_SENT_DEVICETREE;
	}
	if (g_icmdState == ICMD_SENT_DEVICETREE) {
		if (*g_kernelcache) {
			uploadFile(device, g_kernelcache);
			libmd_builtin_sendCommand(device, "bootx");
			Log(LOG_INFO, "Kernelcache %s loaded", g_kernelcache);
		}
		g_icmdState = ICMD_SENT_KERNELCACHE;
	}
}

void recovery_disconnect_callback(AMRecoveryModeDevice device) 
{
	Log(LOG_DEBUG, "recovery_disconnect_callback");
	if (g_icmdState == ICMD_SENT_KERNELCACHE) {
		g_icmdState = ICMD_ZERO;
	}
}

void recovery_callback(RECOVERY_CALLBACK_REASON reason, AMRecoveryModeDevice device, void* ctx) {
	switch(reason) {
	case CALLBACK_RECOVERY_ENTER:
		if (g_autoboot) 
			recovery_connect_callback_autoboot(device);
		else 
			recovery_connect_callback_icmd(device);
		break;
	case CALLBACK_RECOVERY_LEAVE:
		recovery_disconnect_callback(device);
		break;
	case CALLBACK_DFU_ENTER:
		dfu_connect_callback(device);
		break;
	case CALLBACK_DFU_LEAVE:
		Log(LOG_DEBUG, "dfu_disconnect_callback");	
		break;
	}
}
