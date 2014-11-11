#include <libmd/libMobiledevice.h>

#if WIN32 

#include "itunes_private_api.h"

#include <Dbghelp.h>
//#include <Imagehlp.h>

#define Log g_logFunction

#define makeptr( Base, Increment, Typecast ) ((Typecast)( (ULONG)(Base) + (ULONG)(Increment) ))

PIMAGE_NT_HEADERS getNtHeader(HINSTANCE hInst)
{
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hInst;
	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE || pDosHeader->e_lfanew == 0)
		return NULL;
	PIMAGE_NT_HEADERS nt = makeptr(hInst, pDosHeader->e_lfanew, PIMAGE_NT_HEADERS);
	if (nt->Signature != IMAGE_NT_SIGNATURE)
		return NULL;
	return nt;
}


bool getSectionInfo(HINSTANCE hInst, char* sectionName, void** pAddr, size_t* pSize)
{
	PIMAGE_NT_HEADERS nt = getNtHeader(hInst);
	if (NULL == nt)
		return FALSE;
	USHORT SCount = nt->FileHeader.NumberOfSections;
    PIMAGE_SECTION_HEADER Sections = makeptr( nt, sizeof(*nt), PIMAGE_SECTION_HEADER );
 
    for (USHORT i = 0; i < SCount; i++ ) {
        PIMAGE_SECTION_HEADER pSect = Sections + i;
		if (0 == lstrcmpiA((char*)pSect->Name, sectionName)) {
			*pAddr = makeptr( hInst, pSect->VirtualAddress, PVOID );
			*pSize = pSect->Misc.VirtualSize;
			return TRUE;
		}
    }
    return FALSE;
}

char* findBytesInSection(HINSTANCE hInst, char* sectionName, void* bytes, size_t size = 0);

char* findBytesInSection(HINSTANCE hInst, char* sectionName, void* bytes, size_t size)
{
	if (size == 0) {
		size = 1 + lstrlenA((char*)bytes);
	}
	PVOID pSectionData;
	size_t cbSectionData;
	if (!getSectionInfo(hInst, sectionName, &pSectionData, &cbSectionData)) {
		Log(LOG_ERROR, "findBytesInSection: %s section not found!", sectionName); 
		return NULL;
	}
	//Log(LOG_DEBUG, "findBytesInSection: %s section at 0x%p, size 0x%x", sectionName, pSectionData, cbSectionData);
	for (size_t i = 0; i < cbSectionData - size; ++i) {
		char* p = makeptr(pSectionData, i, PCHAR);
		if (0 == memcmp(p, bytes, size)) {
			return p;
		}
	}
	return NULL;
}

char* findBytesNear(void* addr, BOOL fBackwards, size_t maxOffset, void* bytes, size_t size)
{
	for (size_t i = 0; i < maxOffset; ++i) {
		char* p = makeptr(addr, fBackwards ? -i : i, PCHAR);
		if (0 == memcmp(p, bytes, size)) {
			return p;
		}
	}
	return NULL;
}

#define LOCATE_FAILED(x) (((PVOID)x) == (PVOID)-1)

AMRecoveryModeDeviceSendFileToDevice_t locate_AMRecoveryModeDeviceSendFileToDevice()
{
	static AMRecoveryModeDeviceSendFileToDevice_t pfn = 0;
	if (LOCATE_FAILED(pfn))  {
		return NULL;
	}
	if (pfn == NULL) {
		pfn = (AMRecoveryModeDeviceSendFileToDevice_t)-1;

		WCHAR* mod_iTunesMobileDevice = L"iTunesMobileDevice";
		HMODULE hInst = GetModuleHandleW(mod_iTunesMobileDevice);
		if (NULL == hInst) 
			return NULL;
		char* marker = "unable to open device interface for file transfer: %d";
		char* pMarker = findBytesInSection(hInst, ".rdata", marker);
		if (NULL == pMarker) {
			Log(LOG_ERROR, "locate_AMRecoveryModeDeviceSendFileToDevice: Could not locate marker string!");
			return NULL;
		}
		char* pMarkerRef = findBytesInSection(hInst, ".text", (char*)&pMarker, sizeof(pMarker));
		if (NULL == pMarkerRef) {
			Log(LOG_ERROR, "locate_AMRecoveryModeDeviceSendFileToDevice: Could not locate marker xref!");
			return NULL;
		}
	
	//.text:1003EBB0 56                                      push    esi
	//.text:1003EBB1 8B 74 24 08                             mov     esi, [esp+4+arg_0]
		unsigned char prologPattern[] = {0x56, 0x8B, 0x74, 0x24, 0x08, 0x83, 0x7E, 0x1C, 0x00};
		//FIXME: range check..
		char* fn = findBytesNear(pMarkerRef, TRUE/*backwards*/, 0x1000, prologPattern, sizeof(prologPattern));
		if (NULL == fn) {
			Log(LOG_ERROR, "locate_AMRecoveryModeDeviceSendFileToDevice: Could not locate function prolog!");
			return NULL;
		}
		Log(LOG_DEBUG, "locate_AMRecoveryModeDeviceSendFileToDevice: Located at %p", fn);
		pfn = (AMRecoveryModeDeviceSendFileToDevice_t)fn;
	}
	return pfn;
}

LIBMD_API int call_AMRecoveryModeDeviceSendFileToDevice(AMRecoveryModeDevice device, const char* fileName)
{
	AMRecoveryModeDeviceSendFileToDevice_t pfn = locate_AMRecoveryModeDeviceSendFileToDevice();
	if (pfn == NULL)
		return -1;
	CFStringRef cfstrFilename = __CFStringMakeConstantString(fileName);
	return pfn(device, cfstrFilename);
}

#else //MAC OS

extern "C" {
	int AMRecoveryModeDeviceSendFileToDevice(AMRecoveryModeDevice device, CFStringRef fileName);
	int AMRecoveryModeDeviceSendCommandToDevice(AMRecoveryModeDevice device, CFStringRef command);
}


LIBMD_API int call_AMRecoveryModeDeviceSendFileToDevice(AMRecoveryModeDevice device, const char* fileName)
{
	CFStringRef cfstrFilename = __CFStringMakeConstantString(fileName);
	return AMRecoveryModeDeviceSendFileToDevice(device, cfstrFilename);
}


LIBMD_API int libmd_builtin_uploadFile(AMRecoveryModeDevice device, const char* fileName)
{
	return call_AMRecoveryModeDeviceSendFileToDevice(device, fileName);
}

LIBMD_API int libmd_builtin_sendCommand(AMRecoveryModeDevice device, const char* cmd)
{
	CFStringRef cfstrCmd = __CFStringMakeConstantString(cmd);
	return AMRecoveryModeDeviceSendCommandToDevice(device, cfstrCmd);	
}

LIBMD_API int libmd_builtin_uploadFileDfu(AMRecoveryModeDevice device, const char* fileName)
{
	struct xxxx {
		unsigned char x1C[0x1C];
		unsigned int atx1C;
		unsigned char atx20;
		unsigned char filler1[3];
		unsigned char atx24;
		unsigned char filler2[3];
	};
	xxxx* px = (xxxx*)device;
	px->atx1C = -1;
	px->atx20 = 1;
	px->atx24 = 0xFF;
	assert(sizeof(int) == 4);
	Log(LOG_DEBUG, "libmd_builtin_uploadFileDfu: flags patched; sizeof(int)=%u", sizeof(int));
	return call_AMRecoveryModeDeviceSendFileToDevice(device, fileName);	
}

LIBMD_API int libmd_builtin_uploadUsbExploit(AMRecoveryModeDevice device, const char* fileName)
{
	return -1;
}

#endif //WIN32