#include "common.h"
#include <hfs/hfsplus.h>

#if WIN32 


struct HfsContext {
	io_func* io;
	Volume* volume;	
};


#define DLLEXPORT __declspec(dllexport)

#ifdef __cplusplus
extern "C" {
#endif

DLLEXPORT void _cdecl hfslib_close(HfsContext* ctx);
DLLEXPORT HfsContext* _cdecl hfslib_open(char* fileName);
DLLEXPORT BOOL _cdecl hfslib_untar(HfsContext* ctx, char* tarFile);
DLLEXPORT uint64_t _cdecl hfslib_getsize(HfsContext* ctx);
DLLEXPORT BOOL _cdecl hfslib_extend(HfsContext* ctx, uint64_t newSize);


#ifdef __cplusplus
}
#endif

#endif