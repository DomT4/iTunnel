#include <libmd/libMobiledevice.h>

BOOL libmd_private_api_located = FALSE;

#if WIN32

#include "itunes_private_api.h"

LIBMD_ERROR libmd_platform_init() {

	WSADATA useless;
	WSAStartup(WINSOCK_VERSION, &useless);

	WCHAR wbuf[MAX_PATH];
	DWORD cbBuf = sizeof(wbuf);

	//for necrophiles - XP support
	typedef LSTATUS
		(APIENTRY
		*RegGetValue_t) (
			__in HKEY    hkey,
			__in_opt LPCWSTR  lpSubKey,
			__in_opt LPCWSTR  lpValue,
			__in_opt DWORD    dwFlags,
			__out_opt LPDWORD pdwType,
			__out_bcount_part_opt(*pcbData,*pcbData) PVOID   pvData,
			__inout_opt LPDWORD pcbData
			);

	RegGetValue_t rgv = (RegGetValue_t)GetProcAddress(LoadLibraryW(L"advapi32"), "RegGetValueW");
	if  (rgv == nil) {
		rgv = (RegGetValue_t)GetProcAddress(LoadLibraryW(L"Shlwapi"), "SHRegGetValueW");
	}
	if  (rgv == nil) {
		Log(LOG_FATAL, "Your OS is too fucking old!\n");
		return LIBMD_ERR_OS_TOO_FUCKING_OLD;
	}
	//End XP support 
	HRESULT error = S_OK;

	error = rgv(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Apple Inc.\\Apple Application Support", L"InstallDir", RRF_RT_REG_SZ|RRF_ZEROONFAILURE, NULL, (LPBYTE)wbuf, &cbBuf);
	if (ERROR_SUCCESS != error) {
		print_error(error);
		Log(LOG_FATAL, "Could not locate 'Apple Application Support' folder path in registry: ABORTING");
		return LIBMD_ERR_REGISTRY_ERROR;
	}

	SetDllDirectoryW(wbuf);

	if (!LoadLibraryW(L"ASL")) {
		print_error();
		Log(LOG_ERROR, "WARNING: Could not load ASL from '%ws'", wbuf);
	}

	if (!LoadLibraryW(L"CoreFoundation")) {
		print_error();
		Log(LOG_FATAL, "Could not load CoreFoundation from '%ws': ABORTING", wbuf);
		return LIBMD_ERR_LOAD_ERROR;
	}

	cbBuf = sizeof(wbuf);
	error = rgv(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Apple Inc.\\Apple Mobile Device Support", L"InstallDir", RRF_RT_REG_SZ|RRF_ZEROONFAILURE, NULL, (LPBYTE)wbuf, &cbBuf);
	if (ERROR_SUCCESS != error) {
		print_error(error);
		Log(LOG_FATAL, "Could not locate 'Apple Mobile Device Support' folder path in registry: ABORTING");
		return LIBMD_ERR_REGISTRY_ERROR;
	}
	wcscat_s<MAX_PATH>(wbuf, L"\\iTunesMobileDevice.dll");
	if (!LoadLibraryW(wbuf)) {
		print_error();
		Log(LOG_FATAL, "Could not load %ws: ABORTING", wbuf);
		return LIBMD_ERR_LOAD_ERROR;
	}
	libmd_private_api_located = !!locate_AMRecoveryModeDeviceSendFileToDevice();
	return LIBMD_ERR_SUCCESS;
}
#else // OS X


#if !WIN32
static void recv_signal(int sig)
{
	Log(LOG_ERROR, "Info: Signal received. (%d)", sig);

	fflush(stdout);
	signal(sig, SIG_DFL);
	raise(sig);
}
#endif

LIBMD_API LIBMD_ERROR libmd_platform_init() {
	signal(SIGABRT, recv_signal);
	signal(SIGILL, recv_signal);
	signal(SIGINT, recv_signal);
	signal(SIGSEGV, recv_signal);
	signal(SIGTERM, recv_signal);
	// always the case since those are exported
	libmd_private_api_located = TRUE;
	return LIBMD_ERR_SUCCESS;
}
#endif