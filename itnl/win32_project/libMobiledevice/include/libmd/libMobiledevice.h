#ifndef _LIBMOBILEDEVICE_H
#define _LIBMOBILEDEVICE_H

#ifdef WIN32

#define LIBMD_API __declspec(dllimport)

#ifndef _LIBMD_BUILD
#pragma comment(lib, "libMobileDevice.lib")
#else
#ifdef _WINDLL 
#undef LIBMD_API 
#define LIBMD_API __declspec(dllexport)
#endif //_WINDLL

#endif //_LIBMD_BUILD

#else //WIN32

#ifdef _LIBMD_BUILD
#define LIBMD_API __attribute__ ((visibility("default")))
#else //_LIBMD_BUILD
#define LIBMD_API
#endif //_LIBMD_BUILD
#endif //WIN32

#include <libmd/platform.h>
#include <libmd/error.h>

#define ADNCI_MSG_CONNECTED     1
#define ADNCI_MSG_DISCONNECTED  2
#define ADNCI_MSG_UNKNOWN       3

typedef void* restore_dev_t;
typedef void* AMRecoveryModeDevice;
typedef void* afc_conn_t;
typedef void* am_device_t;
typedef int muxconn_t;

struct am_device_notification_callback_info
{
        am_device_t dev;  /* 0    device */
        unsigned int msg;       /* 4    one of ADNCI_MSG_* */
};

typedef void (*am_device_notification_callback_t)(struct am_device_notification_callback_info *);

typedef void (*am_restore_device_notification_callback)(AMRecoveryModeDevice device, void* ctx);

typedef void* am_device_callbacks_t;

enum LOG_LEVEL 
{
	LOG_FATAL,
	LOG_ERROR,
	LOG_INFO,
	LOG_DEBUG,
};

typedef void (*log_function_t)(LOG_LEVEL severity, const char* fmt...);

#define Log g_logFunction

#ifdef __cplusplus
extern "C"  {
#endif
        
int AMDeviceNotificationSubscribe(am_device_notification_callback_t notificationCallback, int , int, int , am_device_callbacks_t *callbacks);
int AMDeviceConnect(am_device_t am_device);
int AMDeviceIsPaired(am_device_t am_device);
int AMDeviceValidatePairing(am_device_t am_device);
int AMDeviceStartSession(am_device_t am_device);
int AMDeviceStartService(am_device_t am_device, CFStringRef service_name, int *handle, unsigned int *unknown );
int AFCConnectionOpen(int handle, unsigned int io_timeout, afc_conn_t* afc_connection);
int AMDeviceDisconnect(am_device_t am_device);
int AMDeviceStopSession(am_device_t am_device);

int AMRestoreRegisterForDeviceNotifications(
    am_restore_device_notification_callback dfu_connect_callback,
    am_restore_device_notification_callback recovery_connect_callback,
    am_restore_device_notification_callback dfu_disconnect_callback,
    am_restore_device_notification_callback recovery_disconnect_callback,
    unsigned int unknown0,
    void *ctx);

int AMRecoveryModeDeviceReboot(AMRecoveryModeDevice device);
int AMRecoveryModeDeviceSetAutoBoot(AMRecoveryModeDevice device, bool autoboot);
int AMRecoveryModeDeviceGetTypeID(AMRecoveryModeDevice device);
int AMRecoveryModeDeviceGetProductID(AMRecoveryModeDevice device);
int AMRecoveryModeDeviceGetProductType(AMRecoveryModeDevice device);


CFStringRef AMDeviceCopyDeviceIdentifier(am_device_t device);
int AMDeviceGetInterfaceType(am_device_t device);

muxconn_t AMDeviceGetConnectionID(am_device_t device);
muxconn_t AMRestoreModeDeviceGetDeviceID(restore_dev_t restore_device);
int AMRestoreModeDeviceReboot(restore_dev_t restore_device);
int USBMuxConnectByPort(muxconn_t muxConn, short netPort, int* sockHandle);


restore_dev_t AMRestoreModeDeviceCreate(int arg1_is_0, int connId, int arg3_is_0);

// itunes private api

LIBMD_API int libmd_builtin_uploadFile(AMRecoveryModeDevice device, const char* fileName);

LIBMD_API int libmd_builtin_sendCommand(AMRecoveryModeDevice device, const char* cmd);

LIBMD_API int libmd_builtin_uploadFileDfu(AMRecoveryModeDevice device, const char* fileName);

LIBMD_API int libmd_builtin_uploadUsbExploit(AMRecoveryModeDevice device, const char* fileName);

//

LIBMD_API int call_AMRecoveryModeDeviceSendFileToDevice(AMRecoveryModeDevice device, const char* fileName);

extern LIBMD_API BOOL libmd_private_api_located;
// High level

typedef enum RECOVERY_CALLBACK_REASON {
	CALLBACK_RECOVERY_ENTER,
	CALLBACK_RECOVERY_LEAVE,
	CALLBACK_DFU_ENTER,
	CALLBACK_DFU_LEAVE,
} RECOVERY_CALLBACK_REASON;

typedef void (_cdecl *recovery_callback_t)(RECOVERY_CALLBACK_REASON reason, AMRecoveryModeDevice device, void* ctx);

LIBMD_API extern log_function_t g_logFunction;

LIBMD_API void libmd_set_recovery_callback(recovery_callback_t callback, void* ctx);

LIBMD_API LIBMD_ERROR libmd_start_mux_tunnel(int localPort, int remotePort);

LIBMD_API void libmd_set_autoboot(AMRecoveryModeDevice device, bool autoboot);

#ifdef __cplusplus
}
#endif

#endif //_LIBMOBILEDEVICE_H