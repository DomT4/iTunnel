
#pragma once

#ifndef _OPTIONS_H
#define _OPTIONS_H

extern char g_ibss[BUFSIZ];
extern char g_exploit[BUFSIZ];
extern char g_ibec[BUFSIZ];
extern char g_ramdisk[BUFSIZ];
extern char g_devicetree[BUFSIZ];
extern char g_kernelcache[BUFSIZ];
extern char g_ramdiskCmd[BUFSIZ];
extern char g_goCmd[BUFSIZ];
extern bool g_autoboot;
extern bool g_builtinApi;
extern int g_ramdiskDelay;
extern int g_logSeverity;

#endif  
