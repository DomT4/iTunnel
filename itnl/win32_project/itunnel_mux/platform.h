#pragma once
#ifndef _PLATFORM_H
#define _PLATFORM_H

#if WIN32 
#include <winsock2.h>
#include <stdio.h>
#else 
#import <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <netinet/ip.h>
#include <mach/error.h>
#endif 

///////////////////////// WIN32 //////////////////////////////
#if WIN32 
typedef void* CFStringRef;
#define nil NULL

typedef void* pthread_t;
typedef void* pthread_attr_t;

#define THREADPROCATTR WINAPI

typedef void *(THREADPROCATTR* thread_proc_t)(void *);

inline int pthread_create(pthread_t * __restrict pHandle,
                         const pthread_attr_t * __restrict,
                         thread_proc_t threadStart,
                         void* arg) 
{
	DWORD tid;
	*pHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadStart, arg, CREATE_SUSPENDED, &tid);
	return tid;
}

#define pthread_detach ResumeThread

#define ERR_SUCCESS 0

enum CFStringBuiltInEncodings {
   //kCFStringEncodingMacRoman = 0,
   //kCFStringEncodingWindowsLatin1 = 0x0500,
   //kCFStringEncodingISOLatin1 = 0x0201,
   //kCFStringEncodingNextStepLatin = 0x0B01,
   kCFStringEncodingASCII = 0x0600,
   kCFStringEncodingUnicode = 0x0100,
   kCFStringEncodingUTF8 = 0x08000100,
   //kCFStringEncodingNonLossyASCII = 0x0BFF
};
typedef enum CFStringBuiltInEncodings CFStringBuiltInEncodings;

typedef int socklen_t;

#define strcasecmp strcmpi

#define sleep(x) Sleep (1000 * (x))

typedef unsigned char uint8_t;

#define close closesocket

typedef void* CFAllocatorRef;
typedef void* CFDictionaryRef;

#define kCFAllocatorDefault 0

extern "C" bool CFStringGetCString(CFStringRef cfStr, char* buf, size_t cbBuf, CFStringBuiltInEncodings encoding);
extern "C" const CFStringRef __CFStringMakeConstantString(const char* cStr);
extern "C" CFStringRef CFStringCreateWithFormat (
   CFAllocatorRef alloc,
   CFDictionaryRef formatOptions,
   CFStringRef format,
   ...
);
extern "C" void CFRelease(void* ptr);

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "iTunesMobileDevice")
#pragma comment(lib, "CoreFoundation")

#else ////////////////////////// OS X ///////////////////

#define THREADPROCATTR 

#define Sleep(ms) usleep(ms*1000)

#endif

#endif //_PLATFORM_H
