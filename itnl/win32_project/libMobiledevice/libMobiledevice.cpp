// Based on iPhone_tunnel by novi (novi.mad@gmail.com ) http://novis.jimdo.com
// thanks
// http://i-funbox.com/blog/2008/09/itunesmobiledevicedll-changed-in-itunes-80/
// 2010 msftguy


// libMobiledevice.cpp : Defines the exported functions for the DLL application.
//
#include <libmd/libMobiledevice.h>

void DummyLogFn(LOG_LEVEL severity, const char* fmt...)
{

}

log_function_t g_logFunction = DummyLogFn;

#define Log g_logFunction

#ifdef WIN32

BOOL WINAPI DllMain(
   DWORD dwReason,
   LPVOID /*lpReserved*/
)
{
	return TRUE;
}

#endif //WIN32

#pragma mark Prototype definition

#define TRUE 1
#define FALSE 0
#define BUFFER_SIZE 256
#define MAX_SOCKETS 512

struct connection
{
	int from_handle;
	int to_handle;
};

void* THREADPROCATTR wait_for_device(void*);
void wait_connections();
void notification(struct am_device_notification_callback_info*);
void*THREADPROCATTR conn_forwarding_thread(void* arg);

static int threadCount = 0;

static int  sock;

static muxconn_t muxConn = 0;

//static const char* target_device_id = nil;

static am_device_t target_device = NULL;

typedef struct CB_CTX {
	void* clientCtx;
	recovery_callback_t clientCallback;
} CB_CTX;

void call_client_callback(RECOVERY_CALLBACK_REASON reason, AMRecoveryModeDevice device, void* ctx)
{
	CB_CTX* context = (CB_CTX*)ctx;
    Log(LOG_DEBUG, "call_client_callback(): pid=0x%x, ptype=0x%x", 
        AMRecoveryModeDeviceGetProductID(device),
        AMRecoveryModeDeviceGetProductType(device));

	context->clientCallback(reason, device, context->clientCtx);
}

void dfu_connect_callback(AMRecoveryModeDevice device, void* ctx)
{
	call_client_callback(CALLBACK_DFU_ENTER, device, ctx);
}

void recovery_connect_callback(AMRecoveryModeDevice device, void* ctx)
{
	call_client_callback(CALLBACK_RECOVERY_ENTER, device, ctx);
}

void dfu_disconnect_callback(AMRecoveryModeDevice device, void* ctx)
{
	call_client_callback(CALLBACK_DFU_LEAVE, device, ctx);
}

void recovery_disconnect_callback(AMRecoveryModeDevice device, void* ctx)
{
	call_client_callback(CALLBACK_RECOVERY_LEAVE, device, ctx);
}

void libmd_set_recovery_callback(recovery_callback_t callback, void* clientCtx)
{
	CB_CTX* ctx = (CB_CTX*)malloc(sizeof(CB_CTX));
	ctx->clientCallback = callback;
	ctx->clientCtx = clientCtx;
	AMRestoreRegisterForDeviceNotifications(dfu_connect_callback, recovery_connect_callback, dfu_disconnect_callback, recovery_disconnect_callback, 0, ctx);
}

LIBMD_ERROR libmd_start_mux_tunnel(int localPort, int remotePort)
{
	struct sockaddr_in saddr;
	int ret = 0;

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	saddr.sin_port = htons(localPort);     
	sock = socket(AF_INET, SOCK_STREAM, 0);

	int temp = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&temp, sizeof(temp))) {
		print_error();
		Log(LOG_ERROR, "setsockopt() failed - ignorable");
	}

	ret = bind(sock, (struct sockaddr*)&saddr, sizeof(struct sockaddr));

	if ( ret == SOCKET_ERROR ) {
		Log(LOG_ERROR, "bind error %i !", ret);
		return LIBMD_ERR_SUCCESS;
	}

	listen(sock, 0);
			
	int lpThreadId;
	pthread_t socket_thread;
	lpThreadId = pthread_create(&socket_thread, NULL, wait_for_device, (void*)remotePort);
	pthread_detach(socket_thread);

	Log(LOG_INFO, "Waiting for new TCP connection on port %hu", localPort);

	Log(LOG_INFO, "Waiting for device...");
	fflush(stdout);

	while (1) {
		am_device_callbacks_t callbacks; 
		ret = AMDeviceNotificationSubscribe(notification, 0, 0, 0, &callbacks);
		if (ret != ERR_SUCCESS) {
				Log(LOG_ERROR, "AMDeviceNotificationSubscribe = %i", ret);
				return LIBMD_ERR_GENERAL_ERROR;
		}
		
#if WIN32
		Sleep(-1);
#else
		CFRunLoopRun();
#endif
		Log(LOG_ERROR, "RUN LOOP EXIT");
		return LIBMD_ERR_GENERAL_ERROR;
	}
	return LIBMD_ERR_SUCCESS;
}

/****************************************************************************/

char* getConnectedDeviceName(struct am_device_notification_callback_info* info)
{
	static char deviceName[BUFFER_SIZE];
	*deviceName = '\0';
	CFStringRef devId = AMDeviceCopyDeviceIdentifier(info->dev);
	if (devId != nil) {
		CFStringGetCString(devId, deviceName, sizeof(deviceName), kCFStringEncodingASCII);
	}
	return deviceName;
}

void notification(struct am_device_notification_callback_info* info)
{
	switch (info -> msg)
	{
	case ADNCI_MSG_CONNECTED:
        {
            int interfaceType = AMDeviceGetInterfaceType(info->dev);   
            int ignore = interfaceType != 1;
            Log(LOG_INFO, "Device connected: %s%s", getConnectedDeviceName(info), 
                ignore ? " - Ignoring (non-USB)" : "");
            if (!ignore) {
                target_device = info->dev;
            }
        }
 		break;
	case ADNCI_MSG_DISCONNECTED:
        Log(LOG_INFO, "Device disconnected: %s", getConnectedDeviceName(info));
        if (info->dev == target_device) {
            Log(LOG_INFO, "Clearing saved mux connection");
            target_device = nil;
            muxConn = 0;
        }
		break;
	default:
		break;
	}
}

void* THREADPROCATTR wait_for_device(void* arg)
{
	int iphonePort = (int)(intptr_t)arg;
	int ret;
	int handle = -1;
	restore_dev_t restore_dev;
			
	while (1) {
		
		if (target_device == NULL) {
			sleep(1);
			continue;
		}
								
		struct sockaddr_in sockAddrin;
		socklen_t len = sizeof(sockAddrin);
		int new_sock = accept(sock, (struct sockaddr*) &sockAddrin , &len);
		
		
		if (new_sock == -1) {
			Log(LOG_ERROR, "accept() error");
			continue;
		}
		
		Log(LOG_INFO, "Info: New connection...");
		
		if (muxConn == 0)
		{
            ret = AMDeviceConnect(target_device);
			Log(LOG_DEBUG, "AMDeviceConnect() = 0x%x", ret);
            
            if (ret == ERR_SUCCESS) {
				muxConn = AMDeviceGetConnectionID(target_device);
			} else if (ret == -402653144) { // means recovery mode .. I think
				muxconn_t mux_tmp = AMDeviceGetConnectionID(target_device);
                Log(LOG_DEBUG, "muxConnTmp = %X", mux_tmp);
                muxConn = mux_tmp;
                restore_dev = AMRestoreModeDeviceCreate(0, mux_tmp, 0);
                Log(LOG_DEBUG, "restore_dev = %p", restore_dev);
                if (restore_dev != NULL) {
                    AMRestoreModeDeviceReboot(restore_dev);
                    sleep(5);
                } 
			} else if (ret == -402653083) { // after we call 'reboot', api host is down
                muxconn_t mux_tmp = AMDeviceGetConnectionID(target_device);
                Log(LOG_DEBUG, "muxConnTmp = %X", mux_tmp);
                muxConn = mux_tmp;
            } else {
				Log(LOG_ERROR, "AMDeviceConnect = %i", ret);
				goto error_connect;
			}
		}                               
		Log(LOG_INFO, "Device connected");
						
		ret = USBMuxConnectByPort(muxConn, htons(iphonePort), &handle);
		if (ret != ERR_SUCCESS) {
			Log(LOG_ERROR, "USBMuxConnectByPort = %x, handle=%x", ret, handle);
			goto error_service;
		}
						
		Log(LOG_INFO, "USBMuxConnectByPort OK");
		
		struct connection* connection1;
		struct connection* connection2;
		
		connection1 = new connection;
		if (!connection1) {
			Log(LOG_ERROR, "Malloc failed!");
			continue;
		}
		connection2 = new connection;    
		if (!connection2) {
			Log(LOG_ERROR, "Malloc failed!");
			continue;
		}
		
		connection1->from_handle = new_sock;
		connection1->to_handle = handle;
		connection2->from_handle = handle;
		connection2->to_handle = new_sock;
		
		Log(LOG_DEBUG, "sock handle newsock:%d iphone:%d", new_sock, handle);

		
		int lpThreadId;
		int lpThreadId2;
		pthread_t thread1;
		pthread_t thread2;
		
		lpThreadId = pthread_create(&thread1, NULL, conn_forwarding_thread, (void*)connection1);
		lpThreadId2 = pthread_create(&thread2, NULL, conn_forwarding_thread, (void*)connection2);
		
		pthread_detach(thread2);
		pthread_detach(thread1);

		Sleep(100);
		
		continue;

error_connect:
		Log(LOG_ERROR, "Error: Device Connect");
		AMDeviceDisconnect(target_device);
		sleep(1);
		
		continue;
		
error_service:
		Log(LOG_ERROR, "Error: Device Service");
		AMDeviceDisconnect(target_device);
		sleep(1);
		continue;
		
	}
	return NULL;
}


/****************************************************************************/

void* THREADPROCATTR conn_forwarding_thread(void* arg)
{
	connection* con = (connection*)arg;
	uint8_t buffer[BUFFER_SIZE];
	int bytes_recv, bytes_send;
	
	threadCount++;
	Log(LOG_DEBUG, "threadcount=%d",threadCount);
	
	while (1) {
		bytes_recv = recv(con->from_handle, (char*)buffer, BUFFER_SIZE, 0);
		
		bytes_send = send(con->to_handle, (char*)buffer, bytes_recv, 0);
		
		if (bytes_recv == 0 || bytes_recv == SOCKET_ERROR || bytes_send == 0 || bytes_send == SOCKET_ERROR) {
			threadCount--;
			Log(LOG_DEBUG, "threadcount=%d\n", threadCount);
			
			close(con->from_handle);
			close(con->to_handle);
									
			delete con;
			
			break;
		}
	}
	return nil;
}

void libmd_set_autoboot(AMRecoveryModeDevice device, bool autoboot) 
{
	AMRecoveryModeDeviceSetAutoBoot(device, true);
	AMRecoveryModeDeviceReboot(device);	
}


