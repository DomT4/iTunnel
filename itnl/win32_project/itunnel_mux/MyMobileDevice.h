
#pragma once

#ifndef _MY_MOBILE_DEVICE_H
#define _MY_MOBILE_DEVICE_H

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

typedef void (*am_restore_device_notification_callback)(AMRecoveryModeDevice device);

typedef void* am_device_callbacks_t;

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

CFStringRef AMDeviceCopyDeviceIdentifier(am_device_t device);

muxconn_t AMDeviceGetConnectionID(am_device_t device);
muxconn_t AMRestoreModeDeviceGetDeviceID(restore_dev_t restore_device);
int AMRestoreModeDeviceReboot(restore_dev_t restore_device);
int USBMuxConnectByPort(muxconn_t muxConn, short netPort, int* sockHandle);


restore_dev_t AMRestoreModeDeviceCreate(int arg1_is_0, int connId, int arg3_is_0);

#ifdef __cplusplus
}
#endif

#endif //_MY_MOBILE_DEVICE_H
