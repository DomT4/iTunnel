#include <libmd/libMobiledevice.h>

void print_error(int error) 
{
	int err = error != 0 ? error :
#if WIN32
		GetLastError();
#else
		errno;
#endif
	Log(LOG_ERROR, "Error 0x%X (%i): '%s'", err, err, strerror(err));
}