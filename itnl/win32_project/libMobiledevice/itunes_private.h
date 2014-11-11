
#include <tchar.h>
#include <stdio.h>

#include <libmd/libMobiledevice.h>

#undef LOBYTE
#undef HIWORD
#undef HIBYTE

#define LOBYTE(x) (((BYTE*)(&x))[0])

#define BYTE1(x) (((BYTE*)(&x))[1])

#define HIBYTE(x) (((BYTE*)(&x))[1])

#define HIWORD(x) (((WORD*)(&x))[1])

#define _BYTE BYTE

#define _WORD WORD

#define _DWORD DWORD

signed int __cdecl sub_100323A0(void* a1, BYTE * a2, BYTE * a3, BYTE * a4, BYTE * a5, BYTE * a6);

typedef struct APPLEDEVICE {
	BYTE unk1[4];
	HANDLE hDev_offs4;
} APPLEDEVICE, *PAPPLEDEVICE;

typedef struct AMRecoveryModeDeviceInternal {
	BYTE unk1[0x18];
	PAPPLEDEVICE pAppleDevice_offs_x18;
	PAPPLEDEVICE pAppleDevice_offs_x1C;
	BYTE unk_byte_x20;
	BYTE unk_byte_x21;
	BYTE unk_byte_x22;
	BYTE unk_byte_x23;
	BYTE unk_byte_x24;
	BYTE unk_byte_x25;
} AMRecoveryModeDeviceInternal;

#pragma pack(push)
#pragma pack(1)

typedef struct {
	union {
		struct {
		BYTE byte0;
		BYTE byte1;
		__int16 short1;
		};
		__int32 int1;
	};
	__int16 short2; // [sp+10h] [bp-50Ch]@7
	__int16 short3; // [sp+12h] [bp-50Ah]@7
} USB_IO_STRUCT;

#pragma pack(pop)
