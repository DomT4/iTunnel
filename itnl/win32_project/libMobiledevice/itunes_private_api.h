#ifndef _ITUNES_PRIVATE_API_H
#define _ITUNES_PRIVATE_API_H

#include <libmd/libMobiledevice.h>

typedef int (_cdecl* AMRecoveryModeDeviceSendFileToDevice_t)(PVOID rDevInt, CFStringRef cfstrFilename);

AMRecoveryModeDeviceSendFileToDevice_t locate_AMRecoveryModeDeviceSendFileToDevice();

//int call_AMRecoveryModeDeviceSendFileToDevice(PVOID rDevInt, CFStringRef cfstrFilename);

#endif _ITUNES_PRIVATE_API_H