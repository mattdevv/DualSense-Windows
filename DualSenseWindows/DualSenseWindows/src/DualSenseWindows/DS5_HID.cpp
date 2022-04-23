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
// only allow ERROR_IO_PENDING
#define CHECK_OVERLAPPED_STARTED(res) \
if (!res) {\
	DWORD err = GetLastError();\
	if (err != ERROR_IO_PENDING) {\
		return err;\
	}\
}

#define CHECK_OVERLAPPED_FINISHED(res) \
if (!res) {\
	CancelIoEx(device, ol);\
	return GetLastError();\
}\

DWORD DS5W::AwaitOverlapped(HANDLE device, OVERLAPPED * ol)
{
	DWORD bytes_passed;
	BOOL res;

	// wait/check read ended correctly with infinite wait
	res = GetOverlappedResult(device, ol, &bytes_passed, TRUE);
	CHECK_OVERLAPPED_FINISHED(res);

	/* bytes_read does not include the first byte which contains the
	   report ID. The data buffer actually contains one more byte than
	   bytes_read. */
	bytes_passed++;

	return 0;
}

DWORD DS5W::AwaitOverlappedTimeout(HANDLE device, OVERLAPPED* ol, int milliseconds)
{
	DWORD bytes_passed;
	BOOL res;

	// wait/check read ended correctly with infinite wait
	res = GetOverlappedResultEx(device, ol, &bytes_passed, milliseconds, FALSE);
	CHECK_OVERLAPPED_FINISHED(res);

	/* bytes_read does not include the first byte which contains the
	   report ID. The data buffer actually contains one more byte than
	   bytes_read. */
	bytes_passed++;

	return 0;
}

DWORD DS5W::getHIDFeatureReport(UCHAR reportID, HANDLE device, OVERLAPPED* ol, UCHAR* buffer, size_t length)
{
	buffer[0] = reportID;

	BOOL res;
	DWORD bytes_returned;

	// start read request
	ResetEvent(ol->hEvent);
	res = DeviceIoControl(device,
		IOCTL_HID_GET_FEATURE,
		buffer, length,
		buffer, length,
		&bytes_returned, ol);

	// check if request started correctly
	CHECK_OVERLAPPED_STARTED(res);

	// run request synchronously and check there were no errors
	DWORD err = AwaitOverlapped(device, ol);
	if (err) return err;

	return 0;
}

DWORD DS5W::getHIDInputReportOverlapped(UCHAR reportID, HANDLE device, OVERLAPPED* ol, UCHAR* buffer, size_t length)
{
	buffer[0] = reportID;

	DWORD bytes_read = 0;
	BOOL res;

	// Start an Overlapped I/O read
	ResetEvent(ol->hEvent);
	res = ReadFile(device, buffer, length, NULL, ol);

	// check if request started correctly
	CHECK_OVERLAPPED_STARTED(res);

	return 0;
}

DWORD DS5W::setHIDOutputReportOverlapped(UCHAR reportID, HANDLE device, OVERLAPPED* ol, UCHAR* buffer, size_t length)
{
	buffer[0] = reportID;

	BOOL res;

	// Start an overlapped write
	ResetEvent(ol->hEvent);
	res = WriteFile(device, buffer, length, NULL, ol);

	// check if request started correctly
	CHECK_OVERLAPPED_STARTED(res);

	return 0;
}
