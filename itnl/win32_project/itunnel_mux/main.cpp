// iTunnel mod with restore mode comm support
// Based on iPhone_tunnel by novi (novi.mad@gmail.com ) http://novis.jimdo.com
// thanks
// http://i-funbox.com/blog/2008/09/itunesmobiledevicedll-changed-in-itunes-80/
// 2010 msftguy

#include "main.h"

enum exit_status {
	EXIT_SUCCESS_I			= 0x00,
	EXIT_DISCONNECTED		= 0x01,
	EXIT_CONNECT_ERROR		= 0x02,
	EXIT_SERVICE_ERROR		= 0x03,
	EXIT_BIND_ERROR			= 0x04,
	EXIT_GENERAL_ERROR		= 0x05,
	EXIT_OS_TOO_FUCKING_OLD = 0x06,
	EXIT_REGISTRY_ERROR		= 0x07,
	EXIT_LOAD_ERROR			= 0x08,
	EXIT_BAD_OPTIONS		= 0x09,
	EXIT_BAD_OPTION_COMBO	= 0x0A,
};

typedef enum PROGRAM_MODE {
	MODE_NONE,
	MODE_TUNNEL, 
	MODE_AUTOBOOT,
	MODE_ICMD,
} PROGRAM_MODE;

static PROGRAM_MODE g_programMode = MODE_NONE;

typedef char OPTION_T [BUFSIZ];


#pragma mark Prototype definition

#define TRUE 1
#define FALSE 0
#define BUFFER_SIZE 256
#define MAX_SOCKETS 512

#define SOCKET_ERROR -1

struct connection
{
	int from_handle;
	int to_handle;
};

void* THREADPROCATTR wait_for_device(void*);
void wait_connections();
void notification(struct am_device_notification_callback_info*);
void*THREADPROCATTR conn_forwarding_thread(void* arg);

#pragma mark Main function

#if WIN32 
	const unsigned short default_local_port = 22;
#else
	const unsigned short default_local_port = 2022;
#endif

int g_iphone_port = 22;
int g_local_port = default_local_port;

int parse_args(int argc, char *argv [])
{
	char** pArg;
	for (pArg = argv + 1; pArg < argv + argc; ++pArg) {
		const char* arg = *pArg;
		OPTION_T* pOpt = NULL;
		int* pIntOpt = NULL;
		PROGRAM_MODE newMode = MODE_NONE;
		if (!strcmp(arg, "--tunnel")) {
			newMode = MODE_TUNNEL;
		} if (!strcmp(arg, "--ibss")) {
			pOpt = &g_ibss; newMode = MODE_ICMD;
		} else if (!strcmp(arg, "--builtin")) {
			g_builtinApi = true; newMode = MODE_ICMD;
		} else if (!strcmp(arg, "--exploit")) {
			pOpt = &g_exploit; newMode = MODE_ICMD;
		} else if (!strcmp(arg, "--ibec")) {
			pOpt = &g_ibec; newMode = MODE_ICMD;
		} else if (!strcmp(arg, "--ramdisk")) {
			pOpt = &g_ramdisk; newMode = MODE_ICMD;
		} else if (!strcmp(arg, "--ramdisk-command")) {
			pOpt = &g_ramdiskCmd; newMode = MODE_ICMD;
		} else if (!strcmp(arg, "--go-command")) {
			pOpt = &g_goCmd; newMode = MODE_ICMD;
		} else if (!strcmp(arg, "--ramdisk-delay")) {
			pIntOpt = &g_ramdiskDelay; newMode = MODE_ICMD;
		} else if (!strcmp(arg, "--devicetree")) {
			pOpt = &g_devicetree; newMode = MODE_ICMD;
		} else if (!strcmp(arg, "--kernelcache")) {
			pOpt = &g_kernelcache; newMode = MODE_ICMD;
		} else if (!strcmp(arg, "--autoboot")) {
			g_autoboot = true; newMode = MODE_AUTOBOOT;
		} else if (!strcmp(arg, "--iport")) {
			pIntOpt = &g_iphone_port; newMode = MODE_TUNNEL;
		} else if (!strcmp(arg, "--lport")) {
			pIntOpt = &g_local_port; newMode = MODE_TUNNEL;
		} else if (!strcmp(arg, "--verbose")) {
			pIntOpt = &g_logSeverity;
		} else {
			return EXIT_BAD_OPTIONS;
		}

		if (newMode != MODE_NONE && newMode != g_programMode) {
			if (g_programMode == MODE_NONE) 
				g_programMode = newMode;
			else
				return EXIT_BAD_OPTION_COMBO;
		}
		if (pOpt != NULL || pIntOpt != NULL) {
			if (pArg + 1 >= argv + argc) {
				return EXIT_BAD_OPTIONS;
			}
			++pArg;
			if (pOpt) {
				strncpy(*pOpt,  *pArg, sizeof(OPTION_T));
			} else if (pIntOpt) {
				if (sscanf(*pArg, "%i", pIntOpt) != 1) {
					return EXIT_BAD_OPTIONS;
				}
			}
		}
	}
	return 0;
}


void usage()
{
printf(
			"\niphone_tunnel v2.0 for Win/Mac\n"
			"Created by novi. (novi.mad@gmail.com)\n"
			"Restore mode hack by msft.guy ((rev 5))\n"
			"\n"
			"Usage: iphone_tunnel --tunnel [--iport <iPhone port>] [--lport <Local port>]\n"
			"OR: iphone_tunnel --autoboot\tto kick out of the recovery mode\n"
			"OR: iphone_tunnel [--ibss <iBSS file>] [--exploit <iBSS USB exploit payload>]\n"
			"\t[--ibec <iBEC file>] [--ramdisk <ramdisk file>]\n"
			"\t[--devicetree <devicetree file>] [--kernelcache <kernelcache file>]\n"
			"\t[--ramdisk-command <command to execute after sending ramdisk, default is 'ramdisk'>]\n"
			"\t[--ramdisk-delay <delay before loading ramdisk, in seconds, default is 5>]\n"
			"\t[--go-command <command to execute after sending iBEC, default is 'go'>]\n"
			"\t[--verbose <verbose level>]\n"
			"\n"
			
			"Examples:\n"
			"\tiphone_tunnel --lport 2022\n"
			"\tiphone_tunnel --ibec iBEC_file_from_custom_FW --ramdisk created_ramdisk.dmg.ssh --devicetree DevicetreeXXX.img3 --kernelcache kernelcache_file_from_custom_FW --ramdisk-command \"ramdisk 0x90000000\"\n"
			"\n"
			"Default ports are 22(iPhone) <-> %hu(this machine)\n", default_local_port
			);
}

int main (int argc, char *argv [])
{
	g_logFunction = LogPrintf;

	int ret = parse_args(argc, argv);
	if (ret != 0 || argc == 1  || g_programMode == MODE_NONE) {
		usage();
		return ret;
	}

	LIBMD_ERROR err = libmd_platform_init();
	if (err != LIBMD_ERR_SUCCESS) {
		return err;
	}


	switch (g_programMode) {
	case MODE_AUTOBOOT:
	case MODE_ICMD:
		Log(LOG_INFO, "Waiting for a device in Recovery mode to connect..");
 		libmd_set_recovery_callback(recovery_callback, NULL);
#ifndef WIN32
            CFRunLoopRun();
#endif
            break;
	case MODE_TUNNEL:
		libmd_start_mux_tunnel(g_local_port, g_iphone_port);
		break;
	}

	return 0;
}
