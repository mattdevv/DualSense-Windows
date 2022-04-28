/*
	IO.cpp is part of DualSenseWindows
	https://github.com/Ohjurot/DualSense-Windows

	Contributors of this file:
	04.2022 Matthew Hall

	Licensed under the MIT License (To be found in repository root directory)
*/

#include <DualSenseWindows/DSW_Api.h>
#include <DualSenseWindows/DS5_HID.h>

#define NOMINMAX

#include <Windows.h>
#include <malloc.h>

#include <initguid.h>
#include <Hidclass.h>
#include <SetupAPI.h>
#include <hidsdi.h>

// check result of starting an overlapped request
// ERROR_IO_PENDING is allowed, just means request did not return automatically
#define CHECK_ERROR_OL_START(res) \
if (!res) {\
	DWORD err = GetLastError();\
	if (err != ERROR_IO_PENDING) {\
		return err;\
	}\
}\

bool DS5W::CheckDeviceAttributes(HANDLE device, USHORT vendorID, USHORT productID)
{
	HIDD_ATTRIBUTES deviceAttributes;

	if (HidD_GetAttributes(device, &deviceAttributes)) {
		if (deviceAttributes.VendorID == vendorID && deviceAttributes.ProductID == productID) {
			return true;
		}
	}

	return 0;
}

USHORT DS5W::GetDeviceInputReportSize(HANDLE device)
{
	USHORT size = 0;

	PHIDP_PREPARSED_DATA ppd;
	if (HidD_GetPreparsedData(device, &ppd)) {
		// Get device capabilitys
		HIDP_CAPS deviceCaps;
		if (HidP_GetCaps(ppd, &deviceCaps) == HIDP_STATUS_SUCCESS) {
			size = deviceCaps.InputReportByteLength;
		}
		// Free preparsed data
		HidD_FreePreparsedData(ppd);
	}

	return size;
}

DWORD DS5W::AwaitOverlapped(HANDLE device, LPOVERLAPPED ol)
{
	DWORD bytes_passed;

	// wait/check read ended correctly with infinite wait
	BOOL res = GetOverlappedResult(device, ol, &bytes_passed, TRUE);
	if (!res) {
		CancelIoEx(device, ol);
		return GetLastError();
	}

	// bytes read does not include first byte (reportID)
	bytes_passed++;

	return 0;
}

DWORD DS5W::AwaitOverlappedTimeout(HANDLE device, LPOVERLAPPED ol, int milliseconds)
{
	DWORD bytes_passed;

	// wait/check read ended correctly with timeout
	BOOL res = GetOverlappedResultEx(device, ol, &bytes_passed, milliseconds, FALSE);
	if (!res) {
		CancelIoEx(device, ol);
		return GetLastError();
	}

	// bytes read does not include first byte (reportID)
	bytes_passed++;

	return 0;
}
