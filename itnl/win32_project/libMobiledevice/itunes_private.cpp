#include "itunes_private.h"

#define Log g_logFunction

static bool g_exploitMode = false;

signed int getUsbDeviceName(void* thisObj, int interfaceType, char *Dest, size_t Count)
{
	const char *v4; // eax@2
	CFStringRef v5; // eax@5
	const char *v7; // ecx@9
	int v8; // [sp-4h] [bp-4h]@4
	int v9; // [sp-4h] [bp-4h]@5

	if ( interfaceType == 1 )
	{
		v4 = *(const char **)((char*)thisObj + 36);
		if ( !v4 )
		{
			if ( !*(DWORD *)((char*)thisObj + 40) )
			{
				*Dest = 0;
				return 2002;
			}
			v8 = (int)"AppleDevice::GetDeviceID: failed for DFU";
			*Dest = 0;
			return 2002;
		}
	}
	else
	{
		if ( interfaceType != 2 )
		{
			Log(LOG_ERROR, "AppleDevice::GetDeviceID: Invalid interface type: %d", interfaceType);
			*Dest = 0;
			return 2002;
		}
		v7 = *(const char **)((char*)thisObj + 40);
		if ( !v7 )
		{
			Log(LOG_ERROR, "AppleDevice::GetDeviceID: failed for iBoot");
			*Dest = 0;
			return 2002;
		}
		v4 = v7;
		if ( !v7 ) {
			Log(LOG_ERROR, "AppleDevice::GetDeviceID: failed for iBoot");	
		}
	}
	strncpy(Dest, v4, Count);
	Log(LOG_DEBUG, "getUsbDeviceName: %s", Dest);
	return 0;
}

signed int __cdecl AMRUSBGetLastError(HANDLE hDevice)
{
	DWORD v1; // esi@1
	signed int result; // eax@2
	char v3; // ST1C_1@3
	int v4; // eax@3
	int OutBuffer; // [sp+8h] [bp-Ch]@1
	DWORD BytesReturned; // [sp+Ch] [bp-8h]@1
	int InBuffer; // [sp+10h] [bp-4h]@1

	v1 = GetLastError();
	InBuffer = 0;
	OutBuffer = 0;
	BytesReturned = 0;
	if ( DeviceIoControl(hDevice, 0x2200B8u, &InBuffer, 4u, &OutBuffer, 4u, &BytesReturned, 0) )
	{
		SetLastError(v1);
		result = OutBuffer;
	}
	else
	{
		Log(LOG_ERROR, "_AMRUSBGetLastError: error %d", GetLastError());
		SetLastError(v1);
		result = -1073741824;
	}
	return result;
}


DWORD __stdcall AMDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpNumberOfBytesTransferred, LPOVERLAPPED lpOverlapped)
{
	//DWORD v8; // eax@3
	int v9; // edi@3
	//DWORD v10; // esi@3
	//char v11; // ST18_1@5
	//int v12; // eax@5
	//int v14; // eax@7
	//DWORD v15; // eax@14
	CFStringRef v16; // eax@16
	char v17; // ST1C_1@16
	int v18; // eax@18
	int v19; // eax@20
	char v20; // [sp-4h] [bp-18h]@7
	char* v21; // [sp-4h] [bp-18h]@14
	signed int cRetries; // [sp+10h] [bp-4h]@1

	cRetries = 3;
	if ( lpOverlapped )
	{
		int lastErr = 21;
		while ( 1 )
		{
			--cRetries;
			if ( lastErr == 0 )
				return lastErr;
			if ( DeviceIoControl(
				hDevice,
				dwIoControlCode,
				lpInBuffer,
				nInBufferSize,
				lpOutBuffer,
				nOutBufferSize,
				lpNumberOfBytesTransferred,
				lpOverlapped) == 1 )
				return 0;

			lastErr = GetLastError();

			if ( lastErr == 997 )
			{
				if ( WaitForSingleObject(lpOverlapped->hEvent, 0x1388u) )
				{
					*lpNumberOfBytesTransferred = 0;
					lastErr = GetLastError();
					Log(LOG_ERROR, "AMDeviceIoControl: WaitForSingleObject failed");
				}
				else
				{
					lastErr = 0;
					if ( GetOverlappedResult(hDevice, lpOverlapped, lpNumberOfBytesTransferred, 0) )
						goto LABEL_21;
					lastErr = GetLastError();
					Log(LOG_ERROR, "AMDeviceIoControl: GetOverlappedResult failed, %d", lastErr);
				}
			}
			if ( lastErr != 0 )
			{
				HRESULT usbStatus = AMRUSBGetLastError(hDevice);
				if ( lastErr == 31 && usbStatus == -1073741820 )
				{
					Log(LOG_ERROR, "AMDeviceIoControl: pipe stall");
					return lastErr;
				}
				Log(LOG_ERROR, "AMDeviceIoControl: failed, error %d, usbd status %.8x", lastErr,  usbStatus);
				Sleep(500);
			}
LABEL_21:
			if ( cRetries == 0 )
				return lastErr;
		}
	}
	while ( 1 )
	{
		--cRetries;
		if ( DeviceIoControl(
			hDevice,
			dwIoControlCode,
			lpInBuffer,
			nInBufferSize,
			lpOutBuffer,
			nOutBufferSize,
			lpNumberOfBytesTransferred,
			0) == 1 )
			return 0;
		HRESULT usbd_status = AMRUSBGetLastError(hDevice);
		int lastErr = GetLastError();
		if ( lastErr == 31 && usbd_status == 0xC0000004 )
		{
			Log(LOG_ERROR, "AMDeviceIoControl: pipe stall");
			return lastErr;
		}
		Log(LOG_ERROR, "AMDeviceIoControl: failed, error %d, usbd status %.8x", lastErr, usbd_status);
		Sleep(500);
		if ( cRetries == 0 )
			return lastErr;
	}
}


signed int __cdecl USBControlTransfer(HANDLE hDevice, USB_IO_STRUCT* usbIoStruct, const void *Dst, size_t NumberOfBytesTransferred, size_t* a5)
{
	size_t v5; // ebp@1
	DWORD v6; // edi@1
	void *v7; // esi@1
	size_t v8; // ST20_4@1
	signed int result; // eax@2
	//int v10; // edi@3
	size_t v11; // ST1C_4@4
	//int v12; // eax@5
	struct _OVERLAPPED Overlapped; // [sp+Ch] [bp-14h]@3

	v5 = NumberOfBytesTransferred;
	v6 = NumberOfBytesTransferred + 8;
	v8 = NumberOfBytesTransferred + 8;
	*(DWORD *)a5 = 0;
	v7 = malloc(v8);
	if ( v7 )
	{
		memcpy(v7, usbIoStruct, sizeof(USB_IO_STRUCT));
		memcpy((char *)v7 + 8, Dst, v5);
		Overlapped.hEvent = CreateEventW(0, 1, 0, 0);
		Overlapped.Internal = 0;
		Overlapped.InternalHigh = 0;
		Overlapped.Offset = 0;
		Overlapped.OffsetHigh = 0;
		NumberOfBytesTransferred = 0;
		int lastErr = AMDeviceIoControl(hDevice, 0x2200A0u, v7, v6, v7, v6, (LPDWORD)&NumberOfBytesTransferred, &Overlapped);
		if ( lastErr != 0 )
		{
			HRESULT usbStatus = AMRUSBGetLastError(hDevice);
			Log(LOG_ERROR, "USBControlTransfer: error %d, usbd status %.8x", lastErr, usbStatus);
			free(v7);
			result = lastErr;
		}
		else
		{
			NumberOfBytesTransferred -= 8;
			v11 = NumberOfBytesTransferred;
			*(DWORD *)a5 = NumberOfBytesTransferred;
			memcpy((void *)Dst, (char *)v7 + 8, v11);
			free(v7);
			result = 0;
		}
	}
	else
	{
		result = 8;
	}
	return result;
}


int __cdecl AMRUSBDeviceSendDeviceRequest(PAPPLEDEVICE pAppleDevice, unsigned __int8 a2, char a3, char a4, char a5, char a6, __int16 a7, unsigned __int16 a8, void *Dst)
{
	signed int v9; // ebx@1
	//char v12; // ST14_1@5
	//int v13; // eax@5
	int v14; // edi@7
	//void *v15; // ecx@7
	int v16; // eax@8
	unsigned __int16 v19; // [sp+12h] [bp-50Ah]@7
	size_t v20; // [sp+14h] [bp-508h]@7
	char usbDeviceName[MAX_PATH]; // [sp+118h] [bp-404h]@2

	v9 = 0;
	if ( pAppleDevice->hDev_offs4 == INVALID_HANDLE_VALUE )
	{
		if ( getUsbDeviceName(pAppleDevice, 2/*iboot interface*/, usbDeviceName, sizeof(usbDeviceName)) )
			return 2001;
		HANDLE hUsbDev = CreateFileA(usbDeviceName, 0xC0000000u, 3u, 0, 3u, 0x40000000u, 0);
		pAppleDevice->hDev_offs4 = hUsbDev;
		if ( hUsbDev == INVALID_HANDLE_VALUE )
		{
			Log(LOG_ERROR,"_AMRUSBDeviceSendDeviceRequest: CreateFile failed for %s, error %d", usbDeviceName, GetLastError());
			return 2002;
		}
		v9 = 1;
	}
	USB_IO_STRUCT usbIoStruct;
	usbIoStruct.byte0 = a5 & 0x1F | 0x20 * (a4 & 3 | (unsigned __int8)(4 * a3));
	usbIoStruct.byte1 = a6;
	usbIoStruct.short1 = a7;
	usbIoStruct.short2 = a2;
	usbIoStruct.short3 = a8;

	v14 = USBControlTransfer(pAppleDevice->hDev_offs4, &usbIoStruct, Dst, a8, &v20);
	if ( v14 )
	{
		v16 = AMRUSBGetLastError(pAppleDevice->hDev_offs4);
		if ( v14 != 31 || (v14 = 2008, v16 != -1073741820) )
			v14 = 2009;
	}
	if ( v9 )
	{
		CloseHandle(pAppleDevice->hDev_offs4);
		pAppleDevice->hDev_offs4 = INVALID_HANDLE_VALUE;
	}
	return v14;
}

int __cdecl sub_10031460(PAPPLEDEVICE pAppleDevice)
{
	return 0;
}

PAPPLEDEVICE __cdecl sub_10031450(PAPPLEDEVICE pAppleDevice)
{
	return pAppleDevice;
}

signed int __cdecl _AMRUSBInterfaceOpen(void* a1)
{
	signed int result; // eax@2
	HANDLE v2; // eax@3
	char v3; // ST14_1@5
	int v4; // eax@5
	char usbDeviceName[MAX_PATH]; // [sp+104h] [bp-404h]@1

	if ( getUsbDeviceName(a1, 2, usbDeviceName, sizeof(usbDeviceName)) )
	{
		result = 2001;
	}
	else
	{
		v2 = CreateFileA(usbDeviceName, 0xC0000000u, 3u, 0, 3u, 0x40000000u, 0);
		*(DWORD *)((char*)a1 + 4) = (DWORD)v2;
		if ( v2 == (HANDLE)-1 )
		{
			Log(LOG_ERROR, "_AMRUSBInterfaceOpen failed for %s: %lu", usbDeviceName, GetLastError());
			result = 2002;
		}
		else
		{
			result = 0;
		}
	}
	return result;
}

void __cdecl AMRUSBInterfaceClose(PAPPLEDEVICE pAppleDevice)
{
	int v2; // eax@4
	char v3; // ST08_1@4

	if ( pAppleDevice == NULL )
	{
		if ( pAppleDevice->hDev_offs4 != INVALID_HANDLE_VALUE )
		{
			CloseHandle(pAppleDevice->hDev_offs4);
			pAppleDevice->hDev_offs4 = INVALID_HANDLE_VALUE;
		}
	}
	else
	{
		Log(LOG_ERROR, "_AMRUSBInterfaceClose pAppleDevice == NULL");
	}
}

signed int __cdecl GetMuxPipeProperties(HANDLE hDevice, DWORD BytesReturned, int a3, int a4, int a5, int a6, int a7, int a8)
{
	signed int result; // eax@2
	char v9; // al@3
	int v10; // edx@3
	int v11; // ecx@3
	int v12; // eax@3
	//char v13; // ST1C_1@4
	//int v14; // eax@4
	struct {
		int InBuffer; // [sp+0h] [bp-18h]@1
		__int16 v16; // [sp+4h] [bp-14h]@3
		unsigned __int8 v17; // [sp+6h] [bp-12h]@3
		char v18; // [sp+7h] [bp-11h]@3
		char v19; // [sp+8h] [bp-10h]@3
	} muxPropIoStruct;

	muxPropIoStruct.InBuffer = (unsigned __int8)BytesReturned;
	if ( DeviceIoControl(hDevice, a3 != 1 ? 2228388 : 2228444, &muxPropIoStruct, 0x18u, &muxPropIoStruct, 0x18u, &BytesReturned, 0) )
	{
		result = 111;
		if ( BytesReturned == 24 )
		{
			v9 = muxPropIoStruct.v17;
			*(_BYTE *)a4 = muxPropIoStruct.v17 >> 7;
			v10 = a6;
			*(_BYTE *)a5 = v9 & 0x7F;
			v11 = a7;
			*(_BYTE *)v10 = muxPropIoStruct.v19;
			v12 = a8;
			*(_WORD *)v11 = muxPropIoStruct.v16;
			*(_BYTE *)v12 = muxPropIoStruct.v18;
			result = 0;
		}
	}
	else
	{
		Log(LOG_ERROR, "GetMuxPipeProperties: DeviceIoControl error %d", GetLastError());
		result = 2001;
	}
	return result;
}

#pragma pack(1)

typedef struct MUXDEVICEINFO {
	char unk[0x1E];
	char byte_offs_1E;
	char unk3[5];
} MUXDEVICEINFO;

signed int __cdecl GetMuxDeviceInfo(const char *devName, MUXDEVICEINFO* muxDeviceInfo, int a3)
{
	signed int v3; // edi@1
	HANDLE v4; // esi@1
	char v6; // ST18_1@2
	int v7; // eax@2
	char v8; // di@7
	int v9; // eax@8
	//char v10; // ST1C_1@8
	//int v11; // eax@10
	//int v12; // [sp-Ch] [bp-13Ch]@7
	//int v13; // [sp-8h] [bp-138h]@7
	HANDLE hObject; // [sp+10h] [bp-120h]@1
	DWORD NumberOfBytesTransferred; // [sp+14h] [bp-11Ch]@3
	struct _OVERLAPPED Overlapped; // [sp+18h] [bp-118h]@1

	v3 = 0;
	Overlapped.Internal = 0;
	Overlapped.InternalHigh = 0;
	Overlapped.Offset = 0;
	Overlapped.OffsetHigh = 0;
	Overlapped.hEvent = 0;
	hObject = CreateEventW(0, 0, 0, 0);
	Overlapped.hEvent = hObject;
	v4 = CreateFileA(devName, 0xC0000000u, 3u, 0, 3u, 0x40000000u, 0);
	if ( v4 == (HANDLE)-1 )
	{
		Log(LOG_ERROR, "GetMuxDeviceInfo: CreateFile failed, interface: %s: %lu", devName, GetLastError());
		return 2001;
	}
	if ( !DeviceIoControl(
		v4,
		a3 != 1 ? 2228392 : 2228440,
		muxDeviceInfo,
		0x24u,
		muxDeviceInfo,
		0x24u,
		&NumberOfBytesTransferred,
		&Overlapped) )
	{
		if ( GetLastError() != 997 )
		{
			Log(LOG_ERROR,"GetMuxDeviceInfo: DeviceIoControl error %d");
			v3 = 2004;
			goto LABEL_12;
		}
		if ( WaitForSingleObject(hObject, 0xFFFFFFFFu) )
		{
			Log(LOG_ERROR, "GetMuxDeviceInfo: WaitForSingleObject failed");
LABEL_11:
			v3 = 2004;
			goto LABEL_12;
		}
		if ( !GetOverlappedResult(v4, &Overlapped, &NumberOfBytesTransferred, 0) )
		{
			Log(LOG_ERROR, "GetMuxDeviceInfo: GetOverlappedResult error %d, usbd status %.8x", GetLastError(), AMRUSBGetLastError(v4));
			//LABEL_10:
			goto LABEL_11;
		}
	}
LABEL_12:
	CloseHandle(hObject);
	CloseHandle(v4);
	return v3;
}


signed int __cdecl sub_100323A0(void* a1, BYTE * a2, BYTE * a3, BYTE * a4, BYTE * a5, BYTE * a6)
{
	signed int result; // eax@2
	DWORD v7; // ebx@4
	int v8; // [sp+Ch] [bp-440h]@1
	int v9; // [sp+10h] [bp-43Ch]@1
	int v10; // [sp+14h] [bp-438h]@1
	int v11; // [sp+18h] [bp-434h]@4
	int v12; // [sp+1Ch] [bp-430h]@1
	int v13; // [sp+20h] [bp-42Ch]@5
	MUXDEVICEINFO muxDevInfo; // [sp+24h] [bp-428h]@3
	//unsigned __int8 v15; // [sp+42h] [bp-40Ah]@4
	char usbDevName[MAX_PATH]; // [sp+48h] [bp-404h]@1

	*(BYTE *)a2 = -1;
	*(BYTE *)a3 = -1;
	*(BYTE *)a4 = -1;
	*(BYTE *)a5 = -1;
	v12 = (int)a5;
	v9 = (int)a1;
	v10 = (int)a6;
	*(BYTE *)a6 = -1;
	if ( getUsbDeviceName(a1, 2, usbDevName, sizeof(usbDevName)) || GetMuxDeviceInfo(usbDevName, &muxDevInfo, 0) )
	{
		result = 2001;
	}
	else
	{
		v7 = 1;
		v11 = muxDevInfo.byte_offs_1E;
		if ( (signed int)muxDevInfo.byte_offs_1E >= 1 )
		{
			do
			{
				if ( GetMuxPipeProperties(
					*(HANDLE *)(v9 + 4),
					v7,
					0,
					(int)&v8,
					(int)((char *)&v8 + 3),
					(int)((char *)&v8 + 1),
					(int)&v13,
					(int)((char *)&v8 + 2)) )
					break;
				if ( BYTE1(v8) == 2 )
				{
					if ( (BYTE)v8 == 1 )
					{
						if ( *(_BYTE *)a4 == (BYTE)-1 )
							*(_BYTE *)a4 = (BYTE)v7;
					}
					else
					{
						if ( !(_BYTE)v8 )
						{
							if ( *(_BYTE *)v12 == -1 )
							{
								*(_BYTE *)v12 = (BYTE)v7;
							}
							else
							{
								if ( *(_BYTE *)v10 == (BYTE)-1 )
									*(_BYTE *)v10 = (BYTE)v7;
							}
						}
					}
				}
				else
				{
					if ( BYTE1(v8) == 3 )
					{
						if ( (_BYTE)v8 == 1 )
						{
							if ( *(_BYTE *)a2 == (BYTE)-1 )
								*(_BYTE *)a2 = (BYTE)v7;
						}
						else
						{
							if ( !(_BYTE)v8 )
							{
								if ( *(_BYTE *)a3 == (BYTE)-1 )
									*(_BYTE *)a3 = (BYTE)v7;
							}
						}
					}
				}
				++v7;
			}
			while ( (signed int)v7 <= v11 );
		}
		result = 0;
	}
	return result;
}

int sub_10033590(void* thisObj, int a2)
{
	int result; // eax@1

	result = a2;
	*(_DWORD *)((char*)thisObj + 68) = a2;
	return result;
}


signed int __cdecl sub_10032290(void* a1, BYTE* a2)
{
	signed int result; // eax@2
	DWORD v3; // ebx@4
	signed int v4; // edi@4
	int v5; // eax@5
	int v6; // eax@11
	int v7; // [sp+8h] [bp-430h]@1
	char v8; // [sp+Ch] [bp-42Ch]@5
	MUXDEVICEINFO muxDevInfo; // [sp+10h] [bp-428h]@3
	//unsigned __int8 v10; // [sp+2Eh] [bp-40Ah]@4
	char usbDevName[MAX_PATH]; // [sp+34h] [bp-404h]@1

	*a2 = -1;
	if ( getUsbDeviceName(a1, 2, usbDevName, sizeof(usbDevName)) || GetMuxDeviceInfo(usbDevName, &muxDevInfo, 1) )
	{
		result = 2001;
	}
	else
	{
		v4 = muxDevInfo.byte_offs_1E;
		v3 = 1;
		if ( (signed int)muxDevInfo.byte_offs_1E >= 1 )
		{
			do
			{
				v5 = GetMuxPipeProperties(
					*(HANDLE *)((char*)a1 + 4),
					v3,
					1,
					(int)((char *)&v7 + 1),
					(int)((char *)&v7 + 2),
					(int)&v7,
					(int)&v8,
					(int)((char *)&v7 + 3));
				if ( v5 )
					break;
				if ( (_BYTE)v7 == 2 && BYTE1(v7) == (_BYTE)v5 )
				{
					*a2 = (BYTE)v3;
					sub_10033590(a1, 1);
					break;
				}
				++v3;
			}
			while ( (signed int)v3 <= v4 );
		}
		Log(LOG_DEBUG, "interface has %d endpoints, file pipe = %d\n", v4, v3);
		result = 0;
	}
	return result;
}


int AMRUSBInterfaceGetFileTransferPipe(AMRecoveryModeDeviceInternal* rDevInt)
{
	signed int v1; // eax@1
	char v2; // ST20_1@2
	int v3; // eax@2
	int v4; // edi@3
	//void* v5; // eax@4
	int result; // eax@11
	char v7; // ST20_1@14
	int v8; // eax@14

	v1 = AMRUSBDeviceSendDeviceRequest(rDevInt->pAppleDevice_offs_x18, 1u, 0, 0, 1, 11, 0, 0, 0);
	if ( v1 )
	{
		Log(LOG_ERROR, "error setting interface's alternate setting: %d", v1);
	}
	v4 = sub_10031460(rDevInt->pAppleDevice_offs_x18);
	if ( v4 )
		goto LABEL_9;
	rDevInt->pAppleDevice_offs_x1C = sub_10031450(rDevInt->pAppleDevice_offs_x18);
	if ( rDevInt->pAppleDevice_offs_x1C == NULL )
	{
		v4 = 2002;
		goto LABEL_9;
	}
	v4 = _AMRUSBInterfaceOpen(rDevInt->pAppleDevice_offs_x1C);
	if ( v4 )
	{
LABEL_9:
		if ( rDevInt->pAppleDevice_offs_x1C != 0 )
		{
			AMRUSBInterfaceClose(rDevInt->pAppleDevice_offs_x1C);
			rDevInt->pAppleDevice_offs_x1C = 0;
		}
		return v4;
	}
	if ( rDevInt->unk_byte_x20 != 0 )
	{
		v4 = sub_100323A0(rDevInt->pAppleDevice_offs_x1C, &rDevInt->unk_byte_x21, &rDevInt->unk_byte_x22, &rDevInt->unk_byte_x23, &rDevInt->unk_byte_x25, &rDevInt->unk_byte_x24);
		if ( !v4 )
			return v4;
		goto LABEL_9;
	}
	result = sub_10032290(rDevInt->pAppleDevice_offs_x1C, &rDevInt->unk_byte_x24);
	if ( result )
	{
		Log(LOG_ERROR, "_AMRUSBInterfaceGetFileTransferPipe returned %d", result);
		rDevInt->unk_byte_x24 = -1;
		result = 0;
	}
	return result;
}

int sub_100335A0(void* thisObj)
{
	return *(_DWORD *)((BYTE*)thisObj + 68);
}


int __cdecl AMRUSBInterfaceWritePipe(PAPPLEDEVICE pAppleDevice, unsigned __int8 a2, LPVOID lpOutBuffer, DWORD nOutBufferSize)
{
	//DWORD v4; // edi@1
	int v6; // eax@2
	char v7; // ST24_1@2
	int v8; // esi@3
	DWORD v9; // ecx@3
	int v10; // esi@4
	HANDLE v11; // ebx@6
	int v12; // esi@6
	//void *v13; // ST00_4@8
	DWORD v14; // esi@9
	//int v15; // eax@12
	int v16; // eax@14
	struct _OVERLAPPED Overlapped; // [sp+Ch] [bp-14h]@8

	if ( pAppleDevice == NULL)
	{
		Log(LOG_ERROR, "_AMRUSBInterfaceWritePipe pAppleDevice == NULL");
		return 2003;
	}
	v8 = a2;
	DWORD NumberOfBytesTransferred = 0;
	if ( sub_100335A0(pAppleDevice) == 1 )
		v10 = 4 * v8 + 432;
	else
		v10 = 4 * v8 + 64;
	v12 = v10 | 0x220002;
	v11 = CreateEventW(0, 1, 0, 0);
	if ( !v11 )
		return GetLastError() != 0 ? 0x7D3 : 0;
	Overlapped.hEvent = 0;
	Overlapped.Internal = 0;
	Overlapped.InternalHigh = 0;
	Overlapped.Offset = 0;
	Overlapped.OffsetHigh = 0;
	Overlapped.hEvent = v11;
	NumberOfBytesTransferred = 0;
	if ( !DeviceIoControl(pAppleDevice->hDev_offs4, v12, lpOutBuffer, nOutBufferSize, lpOutBuffer, nOutBufferSize, 0, &Overlapped) )
	{
		v14 = GetLastError();
		int usbErr = AMRUSBGetLastError(pAppleDevice->hDev_offs4);
		if ( v14 == 997 )
		{
			v14 = 0;
			if ( GetOverlappedResult(pAppleDevice->hDev_offs4, &Overlapped, &NumberOfBytesTransferred, 1) )
				goto LABEL_15;
			v14 = GetLastError();
			Log(LOG_ERROR, "_AMRUSBInterfaceWritePipe: error %d, usbd status %.8x", v14, AMRUSBGetLastError(pAppleDevice->hDev_offs4));

		}
		if ( v14 )
		{
			Log(LOG_ERROR, "_AMRUSBInterfaceWritePipe: error %d, usbd status %.8x", v14, usbErr);
		}
		goto LABEL_15;
	}
	v14 = 0;
LABEL_15:
	CloseHandle(v11);
	return v14 != 0 ? 0x7D3 : 0;
}

signed int __cdecl AMRUSBInterfaceReadPipe(PAPPLEDEVICE pAppleDevice, unsigned __int8 a2, LPVOID lpOutBuffer, size_t *pCbOut)
{
	DWORD v4; // esi@1
	int v6; // eax@2
	char v7; // ST20_1@2
	HANDLE v8; // ebp@3
	char v9; // ST1C_1@4
	//int v10; // eax@4
	//void *v11; // ecx@5
	DWORD v12; // eax@7
	signed int v13; // esi@7
	//char v14; // si@9
	int v15; // eax@9
	int v16; // eax@11
	DWORD NumberOfBytesTransferred; // [sp+Ch] [bp-18h]@1
	struct _OVERLAPPED Overlapped; // [sp+10h] [bp-14h]@5

	v4 = *pCbOut;
	NumberOfBytesTransferred = 0;
	if ( pAppleDevice == NULL )
	{
		Log(LOG_ERROR, "_AMRUSBInterfaceReadPipe pAppleDevice == NULL");
		return 2004;
	}
	v8 = CreateEventW(0, 1, 0, 0);
	if ( !v8 )
	{
		Log(LOG_ERROR, "_AMRUSBInterfaceReadPipe: CreateEvent error %d", GetLastError());
		return 2001;
	}
	//v11 = *(void **)(a1 + 4);
	Overlapped.Internal = 0;
	Overlapped.InternalHigh = 0;
	Overlapped.Offset = 0;
	Overlapped.OffsetHigh = 0;
	Overlapped.hEvent = v8;
	NumberOfBytesTransferred = 0;
	if ( DeviceIoControl(pAppleDevice->hDev_offs4, (4 * a2 + 32) | 0x220001, lpOutBuffer, v4, lpOutBuffer, v4, 0, &Overlapped) )
	{
		CloseHandle(v8);
		return 0;
	}
	v12 = GetLastError();
	v13 = v12;
	if ( v12 == 997 )
	{
		v13 = 0;
		if ( !GetOverlappedResult(pAppleDevice->hDev_offs4, &Overlapped, &NumberOfBytesTransferred, 1) )
		{
			Log(LOG_ERROR, "_AMRUSBInterfaceReadPipe: GetOverlappedResult error %d, usbd status %.8x", GetLastError(), AMRUSBGetLastError(pAppleDevice->hDev_offs4));
			CloseHandle(v8);
			return 2001;
		}
	}
	else
	{
		if ( v12 )
		{ 
			Log(LOG_ERROR, "_AMRUSBInterfaceReadPipe: DeviceIoControl failed: %d, usbd status %.8x", v12, AMRUSBGetLastError(pAppleDevice->hDev_offs4));
			v13 = 2004;
		}
	}
	CloseHandle(v8);
	return v13;
}


int AMRUSBInterfaceWriteReadPipe(AMRecoveryModeDeviceInternal* rDevInt, void *a2, size_t a3, char a4, const void *a5, size_t* pCbOut)
{
	//int v6; // esi@1
	int result; // eax@4
	LPVOID lpOutBuffer; // [sp+8h] [bp-14h]@1
	struct OUTBUF {
		char OutBuffer; // [sp+Ch] [bp-10h]@1
		char v10; // [sp+Dh] [bp-Fh]@1
		__int16 v11; // [sp+Eh] [bp-Eh]@1
		char Dst[8]; // [sp+10h] [bp-Ch]@2
	} outbuf;
	//v6 = (int)a1;
	lpOutBuffer = a2;
	outbuf.OutBuffer = a4;
	outbuf.v11 = 4660;
	outbuf.v10 = a3;
	if ( a3 )
		memcpy(outbuf.Dst, a5, a3);
	if ( rDevInt->pAppleDevice_offs_x1C != 0 || (result = AMRUSBInterfaceGetFileTransferPipe(rDevInt), !result) )
		result = AMRUSBInterfaceWritePipe(rDevInt->pAppleDevice_offs_x1C, rDevInt->unk_byte_x22, &outbuf, a3 + 4);
	if ( !result )
	{
		if ( rDevInt->pAppleDevice_offs_x1C != 0 || (result = AMRUSBInterfaceGetFileTransferPipe(rDevInt), !result) )
			result = AMRUSBInterfaceReadPipe(rDevInt->pAppleDevice_offs_x1C, rDevInt->unk_byte_x21, lpOutBuffer, pCbOut);
	}
	return result;
}


int sub_10037EB0(int a1, AMRecoveryModeDeviceInternal* rDevInt, int a2)
{
	int result; // eax@1
	int v4; // eax@3
	char v5; // ST0C_1@3
	int v6; // [sp+4h] [bp-1Ch]@1
	struct {
		int v7; // [sp+8h] [bp-18h]@1
		int v8; // [sp+Ch] [bp-14h]@1
		char v9[4]; // [sp+10h] [bp-10h]@1
		int v10; // [sp+14h] [bp-Ch]@4
	} pipeIoStruct;

	pipeIoStruct.v7 = a2;
	v6 = 12;
	pipeIoStruct.v8 = 150994944;
	result = AMRUSBInterfaceWriteReadPipe(rDevInt, pipeIoStruct.v9, 8u, 5, &pipeIoStruct.v7, (size_t*)&v6);
	if ( !result )
	{
		if ( v6 )
		{
			*(_DWORD *)a1 = pipeIoStruct.v10;
			result = 0;
		}
		else
		{
			Log(LOG_ERROR, "reply length to file transfer control packet was 0");
			result = 10;
		}
	}
	return result;
}


signed int sub_10037C10(AMRecoveryModeDeviceInternal* rDevInt, CFStringRef cfstrCmd)
{
	char *v2; // eax@1
	//int v3; // esi@1
	char v4; // cl@2
	int v5; // eax@3
	signed int result; // eax@4
	__int16 v7; // bx@5
	int v8; // edi@5
	signed int v9; // ebp@7
	int v10; // eax@10
	DWORD v11; // ebx@13
	int v12; // eax@15
	LPVOID lpOutBuffer; // [sp+4h] [bp-424h]@1
	int v14; // [sp+8h] [bp-420h]@10
	int v15; // [sp+Ch] [bp-41Ch]@10
	__int16 v16; // [sp+10h] [bp-418h]@10
	char v17; // [sp+12h] [bp-416h]@10
	char v18; // [sp+13h] [bp-415h]@10
	//char v19[4]; // [sp+14h] [bp-414h]@16
	struct {
		size_t cbRead; // [sp+18h] [bp-410h]@10
		DWORD nOutBufferSize; // [sp+1Ch] [bp-40Ch]@13
	} ioStruct;
	char Dst[1024]; // [sp+24h] [bp-404h]@1

	CFStringGetCString(cfstrCmd, Dst, 1024, kCFStringEncodingUTF8);
	strcat_s(Dst, sizeof(Dst), "\n");
	v2 = Dst;
	do
	v4 = *v2++;
	while ( v4 );
	v5 = v2 - &Dst[1];
	if ( (unsigned int)v5 < 0x400 )
	{
		v8 = (v5 + 15) & 0xFFFFFFF0;
		v7 = 0;
		if ( (unsigned int)v8 > (unsigned int)v5 )
			memset(&Dst[v5], 0, v8 - v5);
		v9 = 0;
		lpOutBuffer = Dst;
		if ( (unsigned int)v8 > 0 )
		{
			while ( 1 )
			{
				v16 = v7;
				v17 = (char)v7;
				v18 = (char)v7;
				v14 = 12;
				v15 = v8;
				v10 = AMRUSBInterfaceWriteReadPipe(rDevInt, &ioStruct, 8u, 3, &v15, (size_t*)&v14);
				if ( v10 )
				{
					v9 = v10;
					goto LABEL_24;
				}
				if ( !v14 )
					break;
				if ( ioStruct.cbRead != 8 )
				{
					v9 = 7;
					goto LABEL_24;
				}
				v11 = ioStruct.nOutBufferSize;
				if ( ioStruct.nOutBufferSize )
				{
					if ( rDevInt->pAppleDevice_offs_x1C != NULL || (v12 = AMRUSBInterfaceGetFileTransferPipe(rDevInt), !v12) )
					{
						//v19[0] = rDevInt->unk_byte_x25;
						v12 = AMRUSBInterfaceWritePipe(rDevInt->pAppleDevice_offs_x1C, rDevInt->unk_byte_x25, lpOutBuffer, v11);
					}
					v9 = v12;
					if ( v12 )
						goto LABEL_24;
					v8 -= v11;
					lpOutBuffer = (char *)lpOutBuffer + v11;
				}
				if ( !v8 )
					goto LABEL_24;
				v7 = 0;
			}
			v9 = -1;
		}
LABEL_24:
		result = v9;
	}
	else
	{
		result = 1;
	}
	return result;
}

signed int sub_10037D90(AMRecoveryModeDeviceInternal* rDevInt, CFStringRef cfstrCmd, char opt)
{
	char *v2; // eax@1
	//int v3; // esi@1
	//char v4; // cl@2
	signed int v5; // esi@3
	signed int result; // eax@4
	int v7; // eax@4
	char Dst[1024]; // [sp+4h] [bp-404h]@1

	CFStringGetCString(cfstrCmd, Dst, sizeof(Dst), kCFStringEncodingUTF8);
	//v2 = &Dst;
	//do
	//  v4 = *v2++;
	//while ( v4 );
	v5 = AMRUSBDeviceSendDeviceRequest(
		rDevInt->pAppleDevice_offs_x18,
		0,
		0,
		2,
		0,
		opt != 0,
		0,
		strlen(Dst) + 1,
		Dst);
	if ( v5 != 0 ) {
		Log(LOG_ERROR, "command device request for '%s' failed: %d", Dst, v5);
		result = 21;
		if (v5 != 2008)
			result = v5;
	}
	return result;
}


signed int sub_10037E50(AMRecoveryModeDeviceInternal* rDevInt, CFStringRef cfstrCmd, char opt)
{
	signed int result; // eax@2

	if ( rDevInt->unk_byte_x20 == 1 )
		result = sub_10037D90(rDevInt, cfstrCmd, opt);
	else
		result = sub_10037C10(rDevInt, cfstrCmd);
	return result;
}


int sendFileToDeviceByPipe(AMRecoveryModeDeviceInternal* rDevInt, CFStringRef cfstrFileName, char a3)
{
	signed int result; // eax@2
	DWORD v5; // esi@2
	int v6; // eax@2
	DWORD v7; // esi@4
	int v8; // eax@4
	int v9; // eax@8
	//int v10; // edx@8
	size_t v11; // ebp@8
	int v12; // esi@10
	int v13; // eax@12
	void *v14; // ebx@12
	DWORD v15; // ebp@18
	int v16; // eax@19
	//DWORD v17; // edx@20
	unsigned __int8 v18; // zf@22
	unsigned __int8 v19; // sf@22
	DWORD v20; // ST24_4@27
	int v21; // eax@27
	int v22; // ebx@27
	size_t v23; // [sp+8h] [bp-47Ch]@1
	DWORD v24; // [sp+Ch] [bp-478h]@12
	DWORD NumberOfBytesRead; // [sp+14h] [bp-470h]@17
	//int v27; // [sp+18h] [bp-46Ch]@20
	DWORD nNumberOfBytesToRead; // [sp+30h] [bp-454h]@5
	char v29; // [sp+4Ch] [bp-438h]@3
	char wfileName[MAX_PATH];

	CFStringGetCString(cfstrFileName, (char*)wfileName, sizeof(wfileName), kCFStringEncodingASCII);
	HANDLE hFile = (void *)CreateFileA(wfileName, 1u, 0, 0, 3u, 0x80u, 0);
	if ( hFile == INVALID_HANDLE_VALUE )
	{
		Log(LOG_ERROR, "%s: CreateFileW failed: %lu", "_sendFileToDeviceByPipe", GetLastError());
		return 11;
	}
	else
	{
		WIN32_FILE_ATTRIBUTE_DATA fattr;
		if (GetFileAttributesExA(wfileName, GetFileExInfoStandard, &fattr))
		{
			nNumberOfBytesToRead = fattr.nFileSizeLow;
			if ( nNumberOfBytesToRead <= 0x2000000 )
			{
				v23 = 0;
				if ( a3 )
				{
					v9 = sub_10037EB0((int)&v23, rDevInt, nNumberOfBytesToRead);
					v11 = v23;
				}
				else
				{
					v11 = 0x8000;
					v23 = 0x8000;
					v9 = AMRUSBDeviceSendDeviceRequest(rDevInt->pAppleDevice_offs_x18, 0, 0, 2, 1, 0, 0, 0, 0);
				}
				v12 = v9;
				if ( v9 )
				{
					CloseHandle(hFile);
				}
				else
				{
					v14 = malloc(v11);
					v13 = nNumberOfBytesToRead;
					v24 = nNumberOfBytesToRead;
					if ( (signed int)nNumberOfBytesToRead > 0 )
					{
						while ( 1 )
						{
							if ( (size_t)v13 >= v11 )
								v13 = v11;
							if ( !ReadFile(hFile, v14, v13, &NumberOfBytesRead, 0) )
								break;
							v15 = NumberOfBytesRead;
							if ( rDevInt->pAppleDevice_offs_x1C != NULL || (v16 = AMRUSBInterfaceGetFileTransferPipe(rDevInt), !v16) )
							{
								//v17 = rDevInt->pAppleDevice_offs_x1C;
								v16 = AMRUSBInterfaceWritePipe(rDevInt->pAppleDevice_offs_x1C, rDevInt->unk_byte_x24, v14, v15);
							}
							v12 = v16;
							if ( v16
								|| (v18 = v24 == NumberOfBytesRead,
								v19 = (signed int)(v24 - NumberOfBytesRead) < 0,
								v24 -= NumberOfBytesRead,
								v19 | v18) )
								goto LABEL_25;
							v11 = v23;
							v13 = v24;
						}
						v12 = 13;
					}
LABEL_25:
					CloseHandle(hFile);
					free(v14);
					if ( a3 == 1 )
					{
						if ( !v24 )
						{
							v20 = nNumberOfBytesToRead;
							char buf[BUFSIZ];
							_snprintf(buf, sizeof(buf), "setenv filesize %d", v20);

							CFStringRef filesize_cmd = __CFStringMakeConstantString(buf);
							v12 = sub_10037E50(rDevInt, filesize_cmd, 0);
							CFRelease(filesize_cmd);
						}
					}
				}
				result = v12;
			}
			else
			{
				CloseHandle(hFile);
				result = 1;
			}
		}
		else
		{
			Log(LOG_ERROR, "_sendFileToDeviceByPipe: GetFileAttributesExA failed: %lu:", GetLastError());
			CloseHandle(hFile);
			result = 11;			
		}
	}
	return result;
}

//send command
signed int __cdecl sub_100395A0(AMRecoveryModeDeviceInternal* rDevInt, CFStringRef cfstrCmd)
{
	signed int result; // eax@2

	if ( rDevInt->unk_byte_x20 == 1 )
		result = sub_10037D90(rDevInt, cfstrCmd, 1);
	else
		result = sub_10037C10(rDevInt, cfstrCmd);
	return result;
}


//_AMDFUGetDevicePath
signed int __cdecl sub_100318A0(PAPPLEDEVICE pAppleDevice, char *Dest, size_t Count)
{
	signed int result; // eax@2
	int v4; // eax@3
	char v5; // ST08_1@3

	if ( pAppleDevice != NULL )
	{
		result = getUsbDeviceName(pAppleDevice, 1, Dest, Count);
	}
	else
	{
		Log(LOG_ERROR, "_AMDFUGetDevicePath: pAppleDevice == NULL");
		*Dest = 0;
		result = 2002;
	}
	return result;
}

//WinDFU::OpenDeviceByPath
signed int __stdcall sub_1003B150(const char *Source, LPVOID lpOutBuffer, void* a3)
{
	HANDLE v3; // esi@1
	char v4; // ST24_1@1
	int v5; // eax@1
	char v6; // ST20_1@2
	int v7; // eax@2
	HANDLE v8; // edi@3
	DWORD v9; // eax@4
	int v10; // eax@6
	DWORD v12; // eax@8
	const char* v13; // [sp-8h] [bp-138h]@5
	char v14; // [sp-4h] [bp-134h]@5
	DWORD NumberOfBytesTransferred; // [sp+10h] [bp-120h]@1
	void *v16; // [sp+14h] [bp-11Ch]@1
	struct _OVERLAPPED Overlapped; // [sp+18h] [bp-118h]@3

	v16 = (void *)a3;

	Log(LOG_DEBUG, "WinDFU::OpenDeviceByPath: %s", Source);

	v3 = CreateFileA(Source, 0xC0000000u, 3u, 0, 3u, 0x40000000u, 0);
	if ( v3 != (HANDLE)-1 )
	{
		Overlapped.Internal = 0;
		Overlapped.InternalHigh = 0;
		Overlapped.Offset = 0;
		Overlapped.OffsetHigh = 0;
		Overlapped.hEvent = 0;
		v8 = CreateEventW(0, 0, 0, 0);
		Overlapped.hEvent = v8;
		if ( lpOutBuffer
			&& (v9 = AMDeviceIoControl(v3, 0x220004u, 0, 0, lpOutBuffer, 0x12u, &NumberOfBytesTransferred, &Overlapped)) != 0 )
		{
			v14 = v9;
			v13 = "OpenDeviceByPath, AMDeviceIoControl for pdescrDevice failed: %d";
		}
		else
		{
			if ( !v16
				|| (v12 = AMDeviceIoControl(v3, 0x220008u, 0, 0, v16, 0x1Bu, &NumberOfBytesTransferred, &Overlapped), !v12) )
			{
				CloseHandle(v8);
				return (signed int)v3;
			}
			v14 = v12;
			v13 = "OpenDeviceByPath, AMDeviceIoControl for pdescrConfig failed: %d";
		}
		Log(LOG_ERROR, v13, v14);
		CloseHandle(v8);
		CloseHandle(v3);
		return -1;
	}
	Log(LOG_ERROR, "OpenDeviceByPath: CreateFile failed for %s, error %d", Source, GetLastError());
	return (signed int)v3;
}

//GetDFUConfiguration
signed int __cdecl sub_10032220(HANDLE hDevice, const void *Dst, size_t NumberOfBytesTransferred)
{
	signed int result; // eax@1
	signed int v4; // esi@1
	char v5; // ST10_1@2
	int v6; // eax@2
	int v7; // [sp+4h] [bp-8h]@1
	__int16 v8; // [sp+8h] [bp-4h]@1
	__int16 v9; // [sp+Ah] [bp-2h]@1

	USB_IO_STRUCT usbIoStruct;
	usbIoStruct.int1 = 0x2000680;
	usbIoStruct.short2 = 0;
	usbIoStruct.short3 = 0x100;

	result = USBControlTransfer(hDevice, &usbIoStruct, Dst, NumberOfBytesTransferred, &NumberOfBytesTransferred);
	v4 = result;
	if ( result )
	{
		Log(LOG_ERROR, "GetDFUConfiguration: USBControlTransfer failed: %d", result);
		result = v4;
	}
	return result;
}

//WinDFU::OpenPipe
HANDLE __stdcall sub_1003B2D0(int pipeNum, const char* pathBase)
{
	char Dest[0x100]; // [sp+0h] [bp-104h]@1

	sprintf(Dest, "%s\\PIPE%d", pathBase, pipeNum);
	return CreateFileA(Dest, 0xC0000000u, 3u, 0, 3u, 0x40000000u, 0);
}


//WinDFU::OpenDFUDevice
signed int sub_1003B5E0(void *thisObj, const char *Source)
{
	void *v2; // eax@1
	void *v3; // esi@1
	//char v4; // ST20_1@1
	int v5; // eax@1
	int v6; // eax@3
	signed int result; // eax@3
	char v8; // ST20_1@3
	unsigned __int16 v9; // ax@6
	__int16 v10; // ax@6
	unsigned int v11; // edx@9
	size_t v12; // ST20_4@9
	void *v13; // eax@9
	int v14; // eax@10
	char v15; // ST20_1@10
	HANDLE v16; // eax@11
	int v17; // eax@12
	char v18; // ST20_1@12
	int v19; // [sp-4h] [bp-314h]@2
	char arr20[0x100]; // [sp+Ch] [bp-304h]@1
	//unsigned __int8 v21; +0x14 // [sp+20h] [bp-2F0h]@9
	//unsigned __int8 v22; +0x17 // [sp+23h] [bp-2EDh]@6
	//char v23[0xE8]; +0x18 // [sp+24h] [bp-2ECh]@6
	char Dst[0x100]; // [sp+10Ch] [bp-204h]@1
	//char Dest[0x100]; // [sp+20Ch] [bp-104h]@1

	v3 = thisObj;

	Log(LOG_DEBUG, "WinDFU::OpenDFUDevice: path: %s", Source);

	memset(Dst, 0, sizeof(Dst));
	memset(arr20, 0, sizeof(arr20));
	v2 = (void *)sub_1003B150(Source, Dst, arr20);
	*((_DWORD *)v3 + 10) = (DWORD)v2;
	if ( v2 == (void *)-1 )
	{
		Log(LOG_ERROR, "WinDFU::OpenDFUDevice: OpenDeviceByPath failed");
		//LABEL_3:
		return -1;
	}
	if ( sub_10032220(v2, &arr20, 0x100u) )
	{
		Log(LOG_ERROR, "WinDFU::OpenDFUDevice: GetDFUConfiguration failed");
		return -1;
	}
	LOBYTE(v10) = 0;
	HIBYTE(v10) = arr20[0x18];
	v9 = arr20[0x17] | v10;
	*((_WORD *)v3 + 12) = v9;
	if ( !v9 || v9 > 0x1000u )
		*((_WORD *)v3 + 12) = 4096;
	v11 = arr20[0x14];
	*((_DWORD *)v3 + 13) = ((unsigned int)arr20[0x14] >> 3) & 1;
	v12 = *((_WORD *)v3 + 12) + 8;
	*((_DWORD *)v3 + 12) = (v11 >> 2) & 1;
	v13 = malloc(v12);
	*((_DWORD *)v3 + 7) = (DWORD)v13;
	if ( v13 )
	{
		v16 = sub_1003B2D0(0, Source);
		*((_DWORD *)v3 + 11) = (DWORD)v16;
		if ( v16 == INVALID_HANDLE_VALUE )
		{
			Log(LOG_ERROR, "WinDFU::OpenDFUDevice: OpenPipe failed");
			result = -2;
		}
		else
		{
			result = 0;
		}
	}
	else
	{
		Log(LOG_ERROR, "WinDFU::UploadFile: malloc bufferUSB failed");
		result = -5;
	}
	return result;
}


//WinDFU::InitUpdate
signed int sub_1003BBC0(void *thisObj, const char* cFilename, const char *Source)
{
	int v3; // edi@1
	void *v4; // esi@1
	signed int result; // eax@2
	int v6; // eax@2
	HANDLE v7; // eax@3
	int v8; // eax@4
	char v9; // ST18_1@4

	v4 = thisObj;
	v3 = sub_1003B5E0(thisObj, Source);
	if ( v3 )
	{
		Log(LOG_ERROR, "WinDFU::InitUpdate: OpenDFUDevice failed, error: %d, path: %s", v3, Source);
		result = v3;
	}
	else
	{
		v7 = CreateFileA(cFilename, 0x80000000u, 1u, 0, 3u, 0x80u, 0);
		*((_DWORD *)v4 + 9) = (DWORD)v7;
		if ( v7 == (HANDLE)-1 )
		{
			Log(LOG_ERROR, "WinDFU::InitUpdate: CreateFile failed");
			result = -3;
		}
		else
		{
			result = 0;
		}
	}
	return result;
}

//report_progress
int __cdecl sub_10039BF0(int a1, int a2, int a3)
{
	int result; // eax@1
	int v4; // edi@1
	int v5; // eax@1

	v4 = *(_DWORD *)(a1 + 16);
	Log(LOG_DEBUG, "operation %d progress %d", v4, a3);

	result = *(_DWORD *)(a1 + 8);
	if ( result )
		result = ((int (__cdecl *)(int, int, int, _DWORD))result)(a1, v4, a3, *(_DWORD *)(a1 + 12));
	*(_DWORD *)(a1 + 16) = v4;
	*(_DWORD *)(a1 + 20) = a3;
	return result;
}

//report progress #2
int __cdecl sub_1003AEA0(int a1, int a2, int a3)
{
	int result; // eax@1
	int v4; // eax@1

	Log(LOG_DEBUG, "operation %d progress %d", a2, a3);
	result = *(_DWORD *)(a1 + 8);
	if ( result )
		result = ((int (__cdecl *)(int, int, int, _DWORD))result)(a1, a2, a3, *(_DWORD *)(a1 + 12));
	*(_DWORD *)(a1 + 20) = a3;
	*(_DWORD *)(a1 + 16) = a2;
	return result;
}

//WinDFU_ProcessOverlappedIoRequest
DWORD __cdecl sub_1003B420(HANDLE hHandle, HANDLE hDevice, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred)
{
	DWORD v4; // esi@1
	DWORD v5; // eax@4
	int v6; // eax@6
	//char v7; // ST0C_1@6
	int v8; // eax@8
	const char* v10; // [sp-4h] [bp-14h]@4

	v4 = GetLastError();
	if ( v4 == 997 )
	{
		if ( WaitForSingleObject(hHandle, 0x1388u) )
		{
			*lpNumberOfBytesTransferred = 0;
			v5 = GetLastError();
			v10 = "ProcessOverlappedIoRequest: WaitForSingleObject failed";
		}
		else
		{
			v4 = 0;
			if ( GetOverlappedResult(hDevice, lpOverlapped, lpNumberOfBytesTransferred, 0) )
				return v4;
			v5 = GetLastError();
			v10 = "ProcessOverlappedIoRequest: GetOverlappedResult failed";
		}
		v4 = v5;
		Log(LOG_ERROR, v10);
	}
	if ( v4 )
	{
		Log(LOG_ERROR, "ProcessOverlappedIoRequest: failed, error %d, usbd status %.8x", v4, AMRUSBGetLastError(hDevice));
	}
	return v4;
}


//WinDFU_ReadPipe
signed int __stdcall sub_1003BA60(HANDLE hDevice, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, int a4)
{
	signed int result; // eax@2
	int v5; // eax@2
	char v6; // ST1C_1@2
	signed int v7; // ebp@3
	HANDLE v8; // edi@4
	signed int v9; // esi@4
	int v10; // eax@8
	char v11; // ST10_1@8
	int v12; // eax@9
	DWORD v13; // eax@10
	int v14; // eax@11
	DWORD NumberOfBytesTransferred; // [sp+4h] [bp-18h]@4
	struct _OVERLAPPED Overlapped; // [sp+8h] [bp-14h]@4

	if ( a4 )
	{
		v7 = 5;
		while ( 1 )
		{
			--v7;
			Overlapped.Internal = 0;
			Overlapped.InternalHigh = 0;
			Overlapped.Offset = 0;
			Overlapped.OffsetHigh = 0;
			Overlapped.hEvent = 0;
			NumberOfBytesTransferred = 0;
			v8 = CreateEventW(0, 1, 0, 0);
			v9 = 0;
			Overlapped.hEvent = v8;
			if ( !ReadFile(hDevice, lpBuffer, nNumberOfBytesToRead, &NumberOfBytesTransferred, &Overlapped) )
			{
				if ( sub_1003B420(v8, hDevice, &Overlapped, &NumberOfBytesTransferred) )
				{
					if ( Overlapped.Internal == 259 )
					{
						CancelIo(hDevice);
						if ( WaitForSingleObject(Overlapped.hEvent, 0) )
						{
							Log(LOG_ERROR, "ReadPipe, the I/O may not be cancelled");
						}
					}
					Log(LOG_ERROR, "ReadPipe, ProcessOverlappedIoRequest failed, nAtt: %d", v7);
					v9 = -16;
				}
			}
			v13 = NumberOfBytesTransferred;
			if ( nNumberOfBytesToRead != NumberOfBytesTransferred )
			{
				Log(LOG_ERROR, "ReadPipe: failed to read, cbBuffer: %d, cbRead: %d, nAtt: %d", nNumberOfBytesToRead, NumberOfBytesTransferred, v7);
				v13 = NumberOfBytesTransferred;
				v9 = -16;
			}
			*(_DWORD *)a4 = v13;
			CloseHandle(v8);
			if ( !v9 )
				break;
			if ( !v7 )
				return v9;
		}
		result = 0;
	}
	else
	{
		Log(LOG_ERROR, "ReadPipe: DFUERROR_INVALID_PARAM");
		result = -19;
	}
	return result;
}

//WinDFU_WritePipe
signed int __stdcall sub_1003B900(HANDLE hDevice, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, size_t* cbRead)
{
	signed int result; // eax@2
	int v5; // eax@2
	char v6; // ST1C_1@2
	signed int v7; // ebx@3
	HANDLE v8; // edi@4
	signed int v9; // esi@4
	int v10; // eax@8
	char v11; // ST10_1@8
	int v12; // eax@9
	DWORD v13; // eax@10
	int v14; // eax@11
	DWORD NumberOfBytesTransferred; // [sp+4h] [bp-18h]@4
	struct _OVERLAPPED Overlapped; // [sp+8h] [bp-14h]@4

	if ( cbRead != NULL )
	{
		v7 = 5;
		while ( 1 )
		{
			--v7;
			Overlapped.Internal = 0;
			Overlapped.InternalHigh = 0;
			Overlapped.Offset = 0;
			Overlapped.OffsetHigh = 0;
			Overlapped.hEvent = 0;
			v8 = CreateEventW(0, 1, 0, 0);
			v9 = 0;
			Overlapped.hEvent = v8;
			if ( !WriteFile(hDevice, lpBuffer, nNumberOfBytesToWrite, &NumberOfBytesTransferred, &Overlapped) )
			{
				if ( sub_1003B420(v8, hDevice, &Overlapped, &NumberOfBytesTransferred) )
				{
					if ( Overlapped.Internal == 259 )
					{
						CancelIo(hDevice);
						if ( WaitForSingleObject(Overlapped.hEvent, 0) )
						{
							Log(LOG_ERROR, "WritePipe, the I/O may not be cancelled");
						}
					}
					Log(LOG_ERROR, "WritePipe: ProcessOverlappedIoRequest failed, nAtt: %d", v7);
					v9 = -17;
				}
			}
			v13 = NumberOfBytesTransferred;
			if ( nNumberOfBytesToWrite != NumberOfBytesTransferred )
			{
				Log(LOG_ERROR, "WritePipe: failed to write, cbBuffer: %d, cbWritten: %d, nAtt: %d", nNumberOfBytesToWrite, NumberOfBytesTransferred, v7);
				v13 = NumberOfBytesTransferred;
				v9 = -17;
			}
			*cbRead = v13;
			CloseHandle(v8);
			if ( !v9 )
				break;
			if ( !v7 )
				return v9;
		}
		result = 0;
	}
	else
	{
		Log(LOG_ERROR, "WritePipe: DFUERROR_INVALID_PARAM");
		result = -19;
	}
	return result;
}


int __stdcall sub_1003BDB0(HANDLE hDevice, void *Dst, size_t Size, __int16 a4)
{
	int v4; // edi@1
	int v6; // [sp+Ch] [bp-1008h]@1
	struct {
		char Buffer; // [sp+10h] [bp-1004h]@1
		char v8; // [sp+11h] [bp-1003h]@1
		__int16 v9; // [sp+12h] [bp-1002h]@1
		__int16 v10; // [sp+14h] [bp-1000h]@1
		__int16 v11; // [sp+16h] [bp-FFEh]@1
		char Src[0x1000 - 8]; // [sp+18h] [bp-FFCh]@2
	} bufStruct;

	bufStruct.v10 = a4;
	bufStruct.Buffer = -95;
	bufStruct.v8 = 3;
	bufStruct.v9 = 0;
	bufStruct.v11 = Size;
	v4 = sub_1003BA60(hDevice, &bufStruct, Size + 8, (int)&v6);
	if ( v4 )
		memset(Dst, 255, Size);
	else
		memcpy(Dst, bufStruct.Src, Size);
	return v4;
}

//UploadSmallDataPackets
int sub_1003BC90(void *thisObj, void *a2, int a3, unsigned __int16 a4, __int16 *a5)
{
	const void *v5; // ebx@1
	int v6; // esi@1
	signed __int16 v7; // ax@2
	__int16 v8; // cx@2
	int v9; // eax@4
	int v10; // edi@4
	int v11; // edx@5
	int result; // eax@6
	char v13; // ST0C_1@8
	int v14; // eax@8
	void *v15; // [sp+10h] [bp-218h]@1
	HANDLE hDevice; // [sp+14h] [bp-214h]@1
	size_t v17; // [sp+18h] [bp-210h]@4
	struct{
		char Buffer; // [sp+1Ch] [bp-20Ch]@2
		char v19; // [sp+1Dh] [bp-20Bh]@2
		__int16 v20; // [sp+1Eh] [bp-20Ah]@2
		__int16 v21; // [sp+20h] [bp-208h]@2
		__int16 size;
		char* buf[0x200]; // [sp+22h] [bp-206h]@4

	}iobuf;
	//unsigned int v23; // [sp+224h] [bp-4h]@1
	//v23 = (unsigned int)&v15 ^ dword_10116C9C;
	hDevice = a2;
	v5 = (const void *)(a3 + 8);
	v15 = thisObj;
	v6 = a4;
	if ( a4 )
	{
		while ( 1 )
		{
			v8 = *a5;
			iobuf.Buffer = 33;
			iobuf.v19 = 1;
			iobuf.v20 = v8;
			iobuf.v21 = 0;
			v7 = v6;
			if ( (_WORD)v6 >= 0x200u )
				v7 = 512;
			iobuf.size = v7;
			memcpy(&iobuf.buf, v5, (unsigned __int16)v7);
			v9 = sub_1003B900(hDevice, &iobuf, v7 + 8, &v17);
			v10 = v9;
			if ( v9 )
				break;
			v11 = iobuf.size;
			v6 -= iobuf.size;
			++*a5;
			v5 = (char *)v5 + v11;
			Sleep(1u);
			if ( !(_WORD)v6 )
				goto LABEL_6;
		}
		v13 = v9;
		Log(LOG_ERROR, "UploadSmallDataPackets failed, error: %d", v13);
		result = v10;
	}
	else
	{
LABEL_6:
		result = 0;
	}
	return result;
}


int __stdcall sub_1003BE60(HANDLE hDevice)
{
	size_t v2; // [sp+0h] [bp-1008h]@1
	struct {
		char Buffer; // [sp+4h] [bp-1004h]@1
		char v4; // [sp+5h] [bp-1003h]@1
		__int16 v5; // [sp+6h] [bp-1002h]@1
		__int16 v6; // [sp+8h] [bp-1000h]@1
		__int16 v7; // [sp+Ah] [bp-FFEh]@1
	} bufStruct;

	bufStruct.v5 = 0;
	bufStruct.v6 = 0;
	bufStruct.v7 = 0;
	bufStruct.Buffer = 33;
	bufStruct.v4 = 4;
	return sub_1003B900(hDevice, &bufStruct, 8u, &v2);
}

LPCVOID __stdcall sub_1003BED0(void* thisObj, HANDLE hDevice, LPCVOID lpBuffer, int a3, int a4)
{
	LPCVOID result; // eax@1
	int v5; // ebp@1
	int v6; // edi@1
	LPCVOID v7; // esi@1

	v5 = a4;
	v7 = lpBuffer;
	v6 = a3;
	*(_BYTE *)lpBuffer = 33;
	*((_BYTE *)v7 + 1) = 1;
	*((_WORD *)v7 + 1) = *(_WORD *)v5;
	*((_WORD *)v7 + 2) = 0;
	*((_WORD *)v7 + 3) = v6;
	result = (LPCVOID)sub_1003B900(hDevice, v7, v6 + 8, (size_t*)&lpBuffer);
	lpBuffer = result;
	if ( result )
	{
		if ( v6 )
			result = (LPCVOID)sub_1003BC90(thisObj, hDevice, (int)v7, v6, (short*)v5);
	}
	else
	{
		++*(_WORD *)v5;
		Sleep(1u);
		result = lpBuffer;
	}
	return result;
}


int dfuCryptArr[0x100] = { 0, 0x77073096,0x0EE0E612C,0x990951BA,0x76DC419,0x706AF48F
	,0x0E963A535,0x9E6495A3,0x0EDB8832,0x79DCB8A4,0x0E0D5E91E
	,0x97D2D988,0x9B64C2B,0x7EB17CBD,0x0E7B82D07,0x90BF1D91,0x1DB71064
	,0x6AB020F2,0x0F3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB
	,0x0F4D4B551,0x83D385C7,0x136C9856,0x646BA8C0,0x0FD62F97A
	,0x8A65C9EC,0x14015C4F,0x63066CD9,0x0FA0F3D63,0x8D080DF5
	,0x3B6E20C8,0x4C69105E,0x0D56041E4,0x0A2677172,0x3C03E4D1
	,0x4B04D447,0x0D20D85FD,0x0A50AB56B,0x35B5A8FA,0x42B2986C
	,0x0DBBBC9D6,0x0ACBCF940,0x32D86CE3,0x45DF5C75,0x0DCD60DCF
	,0x0ABD13D59,0x26D930AC,0x51DE003A,0x0C8D75180,0x0BFD06116
	,0x21B4F4B5,0x56B3C423,0x0CFBA9599,0x0B8BDA50F,0x2802B89E
	,0x5F058808,0x0C60CD9B2,0x0B10BE924,0x2F6F7C87,0x58684C11
	,0x0C1611DAB,0x0B6662D3D,0x76DC4190,0x1DB7106,0x98D220BC
	,0x0EFD5102A,0x71B18589,0x6B6B51F,0x9FBFE4A5,0x0E8B8D433
	,0x7807C9A2,0x0F00F934,0x9609A88E,0x0E10E9818,0x7F6A0DBB
	,0x86D3D2D,0x91646C97,0x0E6635C01,0x6B6B51F4,0x1C6C6162,0x856530D8
	,0x0F262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0x0F50FC457
	,0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0x0FCB9887C,0x62DD1DDF
	,0x15DA2D49,0x8CD37CF3,0x0FBD44C65,0x4DB26158,0x3AB551CE
	,0x0A3BC0074,0x0D4BB30E2,0x4ADFA541,0x3DD895D7,0x0A4D1C46D
	,0x0D3D6F4FB,0x4369E96A,0x346ED9FC,0x0AD678846,0x0DA60B8D0
	,0x44042D73,0x33031DE5,0x0AA0A4C5F,0x0DD0D7CC9,0x5005713C
	,0x270241AA,0x0BE0B1010,0x0C90C2086,0x5768B525,0x206F85B3
	,0x0B966D409,0x0CE61E49F,0x5EDEF90E,0x29D9C998,0x0B0D09822
	,0x0C7D7A8B4,0x59B33D17,0x2EB40D81,0x0B7BD5C3B,0x0C0BA6CAD
	,0x0EDB88320,0x9ABFB3B6,0x3B6E20C,0x74B1D29A,0x0EAD54739
	,0x9DD277AF,0x4DB2615,0x73DC1683,0x0E3630B12,0x94643B84,0x0D6D6A3E
	,0x7A6A5AA8,0x0E40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1
	,0x0F00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0x0F762575D
	,0x806567CB,0x196C3671,0x6E6B06E7,0x0FED41B76,0x89D32BE0
	,0x10DA7A5A,0x67DD4ACC,0x0F9B9DF6F,0x8EBEEFF9,0x17B7BE43
	,0x60B08ED5,0x0D6D6A3E8,0x0A1D1937E,0x38D8C2C4,0x4FDFF252
	,0x0D1BB67F1,0x0A6BC5767,0x3FB506DD,0x48B2364B,0x0D80D2BDA
	,0x0AF0A1B4C,0x36034AF6,0x41047A60,0x0DF60EFC3,0x0A867DF55
	,0x316E8EEF,0x4669BE79,0x0CB61B38C,0x0BC66831A,0x256FD2A0
	,0x5268E236,0x0CC0C7795,0x0BB0B4703,0x220216B9,0x5505262F
	,0x0C5BA3BBE,0x0B2BD0B28,0x2BB45A92,0x5CB36A04,0x0C2D7FFA7
	,0x0B5D0CF31,0x2CD99E8B,0x5BDEAE1D,0x9B64C2B0,0x0EC63F226
	,0x756AA39C,0x26D930A,0x9C0906A9,0x0EB0E363F,0x72076785,0x5005713
	,0x95BF4A82,0x0E2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B
	,0x0E5D5BE0D,0x7CDCEFB7,0x0BDBDF21,0x86D3D2D4,0x0F1D4E242
	,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0x0F6B9265B,0x6FB077E1
	,0x18B74777,0x88085AE6,0x0FF0F6A70,0x66063BCA,0x11010B5C
	,0x8F659EFF,0x0F862AE69,0x616BFFD3,0x166CCF45,0x0A00AE278
	,0x0D70DD2EE,0x4E048354,0x3903B3C2,0x0A7672661,0x0D06016F7
	,0x4969474D,0x3E6E77DB,0x0AED16A4A,0x0D9D65ADC,0x40DF0B66
	,0x37D83BF0,0x0A9BCAE53,0x0DEBB9EC5,0x47B2CF7F,0x30B5FFE9
	,0x0BDBDF21C,0x0CABAC28A,0x53B39330,0x24B4A3A6,0x0BAD03605
	,0x0CDD70693,0x54DE5729,0x23D967BF,0x0B3667A2E,0x0C4614AB8
	,0x5D681B02,0x2A6F2B94,0x0B40BBE37,0x0C30C8EA1,0x5A05DF1B
	,0x2D02EF8D};

int g_dword_10108AF8 = -1;

int sub_1003BF50(void* thisObj)
{
	signed int v1; // ebx@1
	int v2; // esi@1
	DWORD v3; // edx@3
	int v4; // ebp@3
	int v5; // eax@3
	void *v6; // ST00_4@3
	int v7; // eax@4
	int v8; // ecx@4
	DWORD v9; // edx@5
	int v10; // eax@12
	signed int v11; // edx@12
	int v12; // ecx@12
	int v13; // eax@13
	int v14; // eax@13
	int v15; // eax@13
	int v16; // eax@13
	int v17; // eax@13
	size_t v18; // edi@15
	char v19; // ST10_1@17
	int v20; // eax@17
	int v21; // eax@19
	unsigned int v22; // edi@20
	int v23; // eax@21
	signed int v24; // edi@26
	int v25; // eax@27
	char v26; // bl@28
	char v27; // bl@29
	int result; // eax@34
	int v29; // eax@34
	char v30; // ST10_1@34
	int v31; // eax@36
	//char v32; // ST10_1@36
	char v33; // ST10_1@37
	int v34; // eax@37
	char v35; // ST10_1@38
	int v36; // eax@38
	int v37; // eax@39
	DWORD NumberOfBytesRead; // [sp+10h] [bp-20h]@1
	int v39; // [sp+14h] [bp-1Ch]@1
	int v40; // [sp+18h] [bp-18h]@1
	DWORD v41; // [sp+1Ch] [bp-14h]@1
	DWORD v42; // [sp+20h] [bp-10h]@1
	struct{
		char Dst; // [sp+24h] [bp-Ch]@27
		unsigned __int8 v44; // [sp+25h] [bp-Bh]@30
		unsigned __int8 v45; // [sp+26h] [bp-Ah]@30
		unsigned __int8 v46; // [sp+27h] [bp-9h]@30
		char v47; // [sp+28h] [bp-8h]@29
		char n6;
	} dstStruct;
	//unsigned int v48; // [sp+2Ch] [bp-4h]@1

	//v48 = (unsigned int)&NumberOfBytesRead ^ dword_10116C9C;
	v2 = (int)thisObj;
	v1 = 16;
	v42 = GetFileSize(*(HANDLE *)((char*)thisObj + 36), 0);
	v41 = v42;
	v40 = -1;
	v39 = 16;
	while ( 2 )
	{
		v3 = *(_WORD *)(v2 + 24);
		v5 = *(_DWORD *)(v2 + 28);
		v6 = *(void **)(v2 + 36);
		NumberOfBytesRead = 0;
		ReadFile(v6, (LPVOID)(v5 + 8), v3, &NumberOfBytesRead, 0);
		v4 = NumberOfBytesRead;
		if ( NumberOfBytesRead )
		{
			v8 = v40;
			v7 = *(_DWORD *)(v2 + 28) + 8;
			if ( NumberOfBytesRead )
			{
				v9 = NumberOfBytesRead;
				do
				{
					v8 = dfuCryptArr[(unsigned __int8)v8 ^ *(_BYTE *)v7++] ^ ((unsigned int)v8 >> 8);
					--v9;
				}
				while ( v9 );
				v1 = v39;
			}
			v41 -= NumberOfBytesRead;
			v40 = v8;
		}
		if ( NumberOfBytesRead < *(_WORD *)(v2 + 24) )
		{
			if ( v1 )
			{
				if ( v1 == 16 )
				{
					v10 = v40;
					v12 = v2 + 2;
					v11 = 2;
					do
					{
						v13 = dfuCryptArr[(unsigned __int8)v10 ^ *(_BYTE *)(v12 - 2)] ^ ((unsigned int)v10 >> 8);
						v14 = dfuCryptArr[(unsigned __int8)v13 ^ *(_BYTE *)(v12 - 1)] ^ ((unsigned int)v13 >> 8);
						v15 = dfuCryptArr[(unsigned __int8)v14 ^ *(_BYTE *)v12] ^ ((unsigned int)v14 >> 8);
						v16 = dfuCryptArr[(unsigned __int8)v15 ^ *(_BYTE *)(v12 + 1)] ^ ((unsigned int)v15 >> 8);
						v17 = dfuCryptArr[(unsigned __int8)v16 ^ *(_BYTE *)(v12 + 2)] ^ ((unsigned int)v16 >> 8);
						v10 = dfuCryptArr[(unsigned __int8)v17 ^ *(_BYTE *)(v12 + 3)] ^ ((unsigned int)v17 >> 8);
						v12 += 6;
						--v11;
					}
					while ( v11 );
					v1 = v39;
					v40 = v10;
					*(_DWORD *)(v2 + 12) = v10;
				}
				v18 = *(_WORD *)(v2 + 24) - v4;
				if ( v18 >= v1 )
					v18 = v1;
				memcpy((void *)(v4 + 8 + *(_DWORD *)(v2 + 28)), (const void *)(v2 - v1 + 16), v18);
				NumberOfBytesRead += v18;
				v19 = NumberOfBytesRead;
				v39 = v1 - v18;
				Log(LOG_DEBUG, "WinDFU::UploadData: EOF, cbRead: %d", v19);
				v4 = NumberOfBytesRead;
			}
		}
		if ( v4 )
		{
			v21 = (int)sub_1003BED0(thisObj, *(HANDLE *)(v2 + 44), *(LPCVOID *)(v2 + 28), v4, v2 + 32);
			if ( v21 )
			{
				v33 = v21;
				Log(LOG_ERROR, "WinDFU::UploadData: UploadDataPacket failed, error %d", v33);
				result = -17;
			}
			else
			{
				v22 = 100 * (v42 - v41) / v42;
				if ( v22 != g_dword_10108AF8 )
				{
					v23 = *(_DWORD *)(v2 + 16);
					if ( v23 )
					{
						if ( v23 == 1 )
							sub_10039BF0(*(_DWORD *)(v2 + 20), 0, 100 * (v42 - v41) / v42);
					}
					else
					{
						sub_1003AEA0(*(_DWORD *)(v2 + 20), 0, 100 * (v42 - v41) / v42);
					}
					g_dword_10108AF8 = v22;
				}
				v24 = 10;
				do
				{
					--v24;
					v25 = sub_1003BDB0(*(HANDLE *)(v2 + 44), &dstStruct, 6u, 0);
					if ( v25 )
					{
						v35 = v25;
						Log(LOG_ERROR, "WinDFU::UploadData: GetStatus failed 1, error: %d", v35);
						return -6;
					}
					v26 = dstStruct.Dst;
					if ( dstStruct.Dst != 0 )
					{
						sub_1003BE60(*(HANDLE *)(v2 + 44));
						Log(LOG_ERROR, "WinDFU::UploadData: GetStatus failed 2, status: %d", v26);
						return -7;
					}
					v27 = dstStruct.v47;
					if ( dstStruct.v47 == 4 )
					{
						Sleep(dstStruct.v44 + ((dstStruct.v45 + (dstStruct.v46 << 8)) << 8));
					}
					else
					{
						if ( dstStruct.v47 == 5 )
							goto LABEL_2;
					}
				}
				while ( v24 );
				if ( v27 == 5 )
				{
LABEL_2:
					v1 = v39;
					continue;
				}
				Log(LOG_ERROR, "WinDFU::UploadData: GetStatus failed, DFUERROR_TIMEOUT_GET_STATUS", v30);
				result = v24 - 12;
			}
		}
		else
		{
			Log(LOG_ERROR, "WinDFU::UploadData: ZLP");
			result = 0;
		}
		return result;
	}
}

int execUsbExploit(HANDLE hDevice)
{
	OVERLAPPED Overlapped = {0};
	USB_IO_STRUCT usbIoStruct;
	usbIoStruct.byte0 = 0x21;
	usbIoStruct.byte1 = 2;
	usbIoStruct.short1 = 0;
	usbIoStruct.short2 = 0;
	usbIoStruct.short3 = 0;
	size_t cbOut = 0;
	int err = 0;
	if (!DeviceIoControl(hDevice, 0x2200A0, &usbIoStruct, sizeof(usbIoStruct), &usbIoStruct, sizeof(usbIoStruct), (LPDWORD)&cbOut, &Overlapped)) {
		err = GetLastError();
		if (err == ERROR_IO_PENDING) {
			CancelIo(hDevice);
			err = 0;
		} else {
			Log(LOG_ERROR, "execUsbExploit, DeviceIoControl(0x21... failed: %d", err);
		}
	}
	return err;
}


signed int __stdcall WinDFU__ResetDevice(HANDLE hDevice)
{
	DWORD v1; // eax@1
	void *v2; // edi@1
	signed int v3; // esi@1
	int v4; // eax@1
	HANDLE v5; // eax@1
	char v6; // ST1C_1@2
	int v7; // eax@2
	DWORD NumberOfBytesTransferred; // [sp+Ch] [bp-18h]@1
	struct _OVERLAPPED Overlapped; // [sp+10h] [bp-14h]@1

	v3 = 0;
	if (g_exploitMode) {
		Log(LOG_DEBUG, "execUsbExploit..");
		v3 = execUsbExploit(hDevice);
	} else {
		Overlapped.Internal = 0;
		Overlapped.InternalHigh = 0;
		Overlapped.Offset = 0;
		Overlapped.OffsetHigh = 0;
		Log(LOG_DEBUG, "WinDFU::ResetDevice: resetting...");
		v5 = CreateEventW(0, 0, 0, 0);
		v2 = v5;
		Overlapped.hEvent = v5;
		v1 = AMDeviceIoControl(hDevice, 0x22000Cu, 0, 0, 0, 0, &NumberOfBytesTransferred, &Overlapped);
		if ( v1 )
		{
			v6 = v1;
			Log(LOG_ERROR, "ResetDevice, AMDeviceIoControl failed: %d", v6);
			v3 = -18;
		}
		CloseHandle(v2);
	}

	return v3;
}


//WinDFU::ProcessUpdateState
signed int  sub_1003B760(void *thisObj, char a2[6], int* a3, int* a4, size_t* a5)
{
	void *v5; // edi@1
	int v6; // eax@2
	signed int result; // eax@3
	int v8; // eax@3
	char v9; // ST08_1@3
	DWORD v10; // edi@4
	int v11; // ST08_4@4
	int v12; // eax@4
	int v13; // eax@6
	char v14; // ST08_1@6
	int v15; // eax@7
	char v16; // ST08_1@7
	int v17; // eax@9
	char v18; // ST08_1@9
	int v19; // eax@11
	char v20; // ST08_1@11
	int v21; // [sp-4h] [bp-10h]@8

	v5 = thisObj;
	switch ( *(_BYTE *)(a2 + 4) )
	{
	case 4:
		Log(LOG_DEBUG, "WinDFU::ProcessUpdateState: status.bState == DFU_STATE_DNBUSY");
		Sleep(*(_BYTE *)(a2 + 1) + ((*(_BYTE *)(a2 + 2) + (*(_BYTE *)(a2 + 3) << 8)) << 8));
		--*a5;
		if ( *a5 > 0 )
			goto LABEL_5;
		Log(LOG_ERROR, "WinDFU::ProcessUpdateState: DFU_STATE_DNBUSY, timeout");
		result = -7;
		*a4 = 1;
		break;
	case 7:
		v10 = *(_BYTE *)(a2 + 1) + ((*(_BYTE *)(a2 + 2) + (*(_BYTE *)(a2 + 3) << 8)) << 8);
		v11 = *(_BYTE *)(a2 + 1) + ((*(_BYTE *)(a2 + 2) + (*(_BYTE *)(a2 + 3) << 8)) << 8);
		Log(LOG_ERROR, "WinDFU::ProcessUpdateState: status.bState == DFU_STATE_MANIFEST, PollTimeout: %d", v11);
		Sleep(v10);
		*a3 = 1;
LABEL_5:
		result = 0;
		break;
	case 8:
		Log(LOG_DEBUG, "WinDFU::ProcessUpdateState: status.bState == DFU_STATE_MANIFEST_WAIT_RESET");
		WinDFU__ResetDevice(*((HANDLE *)v5 + 10));
		*a4 = 1;
		result = 0;
		break;
	case 2:
		Log(LOG_DEBUG, "WinDFU::ProcessUpdateState: status.bState == DFU_STATE_DFU_IDLE");
		result = 0;
		*a4 = 1;
		break;
	case 6:
		Log(LOG_DEBUG, "WinDFU::ProcessUpdateState: status.bState == DFU_STATE_MANIFEST_SYNC");
		goto LABEL_9;
	case 5:
		Log(LOG_DEBUG, "WinDFU::ProcessUpdateState: status.bState == DFU_STATE_DNLOAD_IDLE");
LABEL_9:
		result = 0;
		break;
	default:
		Log(LOG_ERROR, "WinDFU::ProcessUpdateState: DFUERROR_FAIL_INVALID_STATE");
		result = -8;
		*a4 = 1;
		break;
	}
	return result;
}

//WinDFU::FinalizeDfuUpdate
int sub_1003C2A0(void* thisObj)
{
	int v1; // edi@1
	int v2; // esi@1
	int v3; // eax@1
	void *v4; // ecx@1
	const char* v5; // eax@3
	//char v6; // bl@6
	int v7; // edi@6
	char v8; // ST08_1@6
	int v9; // eax@6
	int v10; // eax@8
	int v11; // eax@11
	int v12; // eax@12
	char v14; // [sp-4h] [bp-2Ch]@3
	size_t v15; // [sp+10h] [bp-18h]@1
	int v16; // [sp+14h] [bp-14h]@4
	int v17; // [sp+18h] [bp-10h]@8
	char Dst[8]; // [sp+1Ch] [bp-Ch]@6
	//unsigned int v19; // [sp+24h] [bp-4h]@1

	//v19 = (unsigned int)&v15 ^ dword_10116C9C;
	v2 = (int)thisObj;
	memset(*(void **)((char*)thisObj + 28), 0, *(_WORD *)((char*)thisObj + 24) + 8);
	v3 = *(_DWORD *)(v2 + 28);
	v4 = *(void **)(v2 + 44);
	*(_BYTE *)v3 = 33;
	*(_BYTE *)(v3 + 1) = 1;
	*(_WORD *)(v3 + 2) = *(_WORD *)(v2 + 32);
	*(_WORD *)(v3 + 4) = 0;
	*(_WORD *)(v3 + 6) = 0;
	v1 = sub_1003B900(v4, (LPCVOID)v3, 8u, &v15);
	if ( v1 )
	{
		v14 = v1;
		v5 = "WinDFU::FinalizeDfuUpdate: UploadDataPacket failed: error: %d";
	}
	else
	{
		++*(_WORD *)(v2 + 32);
		Sleep(1u);
		v16 = 0;
		v15 = 10;
		while ( !v1 )
		{
			//v6 = *Dst;
			v7 = sub_1003BDB0(*(HANDLE *)(v2 + 44), Dst, 6u, 0);
			v8 = *Dst;
			Log(LOG_ERROR, "WinDFU::FinalizeDfuUpdate: GetStatus: status: %d, state: %d", v8, Dst[4]);
			if ( v7 )
			{
				Log(LOG_ERROR, "WinDFU::FinalizeDfuUpdate: GetStatus, error != DFUERROR_SUCCESS, ZLP disconnect, reporting success");
				v1 = 0;
				v5 = "WinDFU::FinalizeDfuUpdate: success";
				goto LABEL_14;
			}
			if ( *Dst != 0 )
			{
				sub_1003BE60(*(HANDLE *)(v2 + 44));
				Log(LOG_ERROR, "WinDFU::FinalizeDfuUpdate: GetStatus status.bStatus != DFU_STATUS_OK, ZLP disconnect, reporting success");
				v1 = 0;
				v5 = "WinDFU::FinalizeDfuUpdate: success";
				goto LABEL_14;
			}
			v10 = sub_1003B760(thisObj, Dst, &v17, &v16, &v15);
			v1 = v10;
			if ( v16 )
			{
				if ( !v10 )
				{
					v5 = "WinDFU::FinalizeDfuUpdate: success";
					goto LABEL_14;
				}
				break;
			}
		}
		v14 = v1;
		v5 = "WinDFU::FinalizeDfuUpdate: failed with error: %d";
	}
LABEL_14:
	Log(LOG_ERROR, v5, v14);
	return v1;
}


//WinDFU::UploadFile
int sub_1003C420(void *thisObj, const char* cFileName, char *Source)
{
	int v3; // eax@1
	void *v4; // edi@1
	int v5; // esi@1
	int result; // eax@2
	char v7; // ST08_1@2
	int v8; // eax@2
	int v9; // eax@3
	int v10; // esi@3
	char v11; // ST08_1@4
	int v12; // eax@4
	int v13; // eax@5
	int v14; // esi@5
	char v15; // ST08_1@6
	int v16; // eax@6
	char v17; // [sp+0h] [bp-20h]@1
	char *v18; // [sp+10h] [bp-10h]@1
	int v19; // [sp+1Ch] [bp-4h]@1

	v18 = &v17;
	v4 = thisObj;
	v19 = 0;
	v3 = sub_1003BBC0(thisObj, cFileName, Source);
	v5 = v3;
	if ( v3 )
	{
		Log(LOG_ERROR, "WinDFU::UploadFile: InitUpdate failed, error: %d", v3);
		result = v5;
	}
	else
	{
		v9 = sub_1003BF50(v4);
		v10 = v9;
		if ( v9 )
		{
			Log(LOG_ERROR, "WinDFU::UploadFile: UploadData failed, error: %d", v9);
			result = v10;
		}
		else
		{
			v13 = sub_1003C2A0(v4);
			v14 = v13;
			if ( v13 )
			{
				Log(LOG_ERROR, "WinDFU::UploadFile: FinalizeDfuUpdate failed, error: %d", v13);
			}
			result = v14;
		}
	}
	return result;
}


//WinDFUUpload
signed int __cdecl sub_1003C550(AMRecoveryModeDeviceInternal* rDevInt, int a2, char *Source, const char* cFileName)
{
	int v4; // eax@1
	//char v5; // ST08_1@2
	int v6; // eax@2
	signed int result; // eax@10
	struct {
		__int16 v8; // [sp+8h] [bp-3Ch]@1
		__int16 v9; // [sp+Ah] [bp-3Ah]@1
		__int16 v10; // [sp+Ch] [bp-38h]@1
		__int16 v11; // [sp+Eh] [bp-36h]@1
		char v12; // [sp+10h] [bp-34h]@1
		char v13; // [sp+11h] [bp-33h]@1
		char v14; // [sp+12h] [bp-32h]@1
		char v15; // [sp+13h] [bp-31h]@1
		int filler0; // +14h
		int v16; // [sp+18h] [bp-2Ch]@1
		AMRecoveryModeDeviceInternal* v17; // [sp+1Ch] [bp-28h]@1
		int filler1; //+20h
		void *Memory; // [sp+24h] [bp-20h]@1
		__int16 v19; // [sp+28h] [bp-1Ch]@1
		__int16 v19_align; //+2Ah
		void *v20; // [sp+2Ch] [bp-18h]@1
		void *v21; // [sp+30h] [bp-14h]@1
		HANDLE hObject; // [sp+34h] [bp-10h]@1
		int v23; // [sp+38h] [bp-Ch]@1
		int v24; // [sp+3Ch] [bp-8h]@1
	} dfuStruct;

	dfuStruct.v8 = -1;
	dfuStruct.v9 = -1;
	dfuStruct.v16 = a2;
	dfuStruct.Memory = 0;
	dfuStruct.v20 = (void *)-1;
	dfuStruct.v21 = (void *)-1;
	dfuStruct.hObject = (HANDLE)-1;
	dfuStruct.v23 = 0;
	dfuStruct.v24 = 0;
	dfuStruct.v10 = 1452;
	dfuStruct.v11 = 256;
	dfuStruct.v12 = 85;
	dfuStruct.v13 = 70;
	dfuStruct.v14 = 68;
	dfuStruct.v15 = 16;
	dfuStruct.v17 = rDevInt;
	dfuStruct.v19 = 0;
	v4 = sub_1003C420(&dfuStruct, cFileName, Source);
	if ( v4 )
	{
		//v5 = v4;
		Log(LOG_ERROR, "WinDFUUpload: UploadFile failed, error: %d", v4);
		//AMRLog(3, v6, v5);
		if ( dfuStruct.hObject != (HANDLE)-1 )
			CloseHandle(dfuStruct.hObject);
		if ( dfuStruct.v21 != (void *)-1 )
			CloseHandle(dfuStruct.v21);
		if ( dfuStruct.v20 != (void *)-1 )
			CloseHandle(dfuStruct.v20);
		if ( dfuStruct.Memory )
			free(dfuStruct.Memory);
		result = 2005;
	}
	else
	{
		if ( dfuStruct.hObject != (HANDLE)-1 )
			CloseHandle(dfuStruct.hObject);
		if ( dfuStruct.v21 != (void *)-1 )
			CloseHandle(dfuStruct.v21);
		if ( dfuStruct.v20 != (void *)-1 )
			CloseHandle(dfuStruct.v20);
		if ( dfuStruct.Memory )
			free(dfuStruct.Memory);
		result = 0;
	}
	return result;
}


int dfuSendFile(AMRecoveryModeDeviceInternal* rDevInt, CFStringRef cfstrFilename)
{
	int v2; // edi@1
	int result; // eax@2
	int v4; // eax@2
	char Source[MAX_PATH]; // [sp+4h] [bp-814h]@1
	char cFileName[MAX_PATH]; // [sp+404h] [bp-414h]@1
	CFStringGetCString(cfstrFilename, cFileName, sizeof(cFileName), kCFStringEncodingUTF8);

	v2 = sub_100318A0(rDevInt->pAppleDevice_offs_x18, Source, sizeof(Source));
	if ( v2 )
	{
		Log(LOG_ERROR, "_AMDFUGetDevicePath failed (result = %d)", v2);
		result = v2;
	}
	else
	{
		result = sub_1003C550(rDevInt, 1, Source, cFileName);
		if ( result )
			result = 13;
	}
	return result;
}


//upload file
int sub_100382E0(AMRecoveryModeDeviceInternal* rDevInt, CFStringRef cfstrFilename)
{
	int v2; // edi@1
	int v3; // esi@1
	char v4; // ST08_1@1
	int v5; // eax@1
	int result; // eax@3

	//Log(LOG_DEBUG, "sending file: %@");
	//AMRLog(8, v5, v4);
	if ( rDevInt->unk_byte_x20 == 1 )
	{
		if ( rDevInt->unk_byte_x24 == (BYTE)-1 ) {
			result = dfuSendFile(rDevInt, cfstrFilename);
		} else
			result = sendFileToDeviceByPipe(rDevInt, cfstrFilename, 0);
	}
	else
	{
		result = sendFileToDeviceByPipe(rDevInt, cfstrFilename, 1);
	}
	return result;
}


typedef int (__cdecl*sub_10039590_t)(AMRecoveryModeDeviceInternal* rDevInt, CFStringRef cfstrFilename);

sub_10039590_t sub_10039590_ptr()
{
	return (sub_10039590_t)(((char*)GetModuleHandleA("iTunesMobileDevice")) + 0x39590);
}

int libmd_builtin_uploadFileDfu(AMRecoveryModeDevice device, const char* fileName)
{
	CFStringRef cfstrFilename = __CFStringMakeConstantString(fileName);
	return dfuSendFile((AMRecoveryModeDeviceInternal*)device, cfstrFilename);
}

int libmd_builtin_uploadUsbExploit(AMRecoveryModeDevice device, const char* fileName)
{
	CFStringRef cfstrFilename = __CFStringMakeConstantString(fileName);
	AMRecoveryModeDeviceInternal* rDevInt = (AMRecoveryModeDeviceInternal*)device;
	g_exploitMode = true;
	int err = dfuSendFile(rDevInt, cfstrFilename);
	g_exploitMode = false;
	return err;
}


int libmd_builtin_uploadFile(AMRecoveryModeDevice device, const char* fileName)
{
	CFStringRef cfstrFilename = __CFStringMakeConstantString(fileName);
	return sub_100382E0((AMRecoveryModeDeviceInternal*)device, cfstrFilename);
	//return sub_10039590_ptr()((AMRecoveryModeDeviceInternal*)device, cfstrFilename);
}



typedef signed int (__cdecl* sub_100395D0_t)(AMRecoveryModeDeviceInternal* rDevInt, CFStringRef cfstrCmd);

sub_100395D0_t sub_100395D0_ptr()
{
	return (sub_100395D0_t)(((char*)GetModuleHandleA("iTunesMobileDevice")) + 0x395D0);

}

int libmd_builtin_sendCommand(AMRecoveryModeDevice device, const char* cmd)
{
	CFStringRef cfstrCmd = __CFStringMakeConstantString(cmd);
	return sub_100395A0((AMRecoveryModeDeviceInternal*)device, cfstrCmd);	
	//return sub_100395D0_ptr()((AMRecoveryModeDeviceInternal*)device, cfstrCmd);
}
