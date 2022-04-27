/*
	IO.cpp is part of DualSenseWindows
	https://github.com/Ohjurot/DualSense-Windows

	Contributors of this file:
	04.2022 Matthew Hall

	Licensed under the MIT License (To be found in repository root directory)
*/

#include <DualSenseWindows/DS5_Internal.h>
#include <DualSenseWindows/DS_CRC32.h>
#include <DualSenseWindows/DS5_HID.h>
#include <DualSenseWindows/DS5_Input.h>
#include <DualSenseWindows/DS5_Output.h>

#include <MurmurHash3/MurmurHash3.h>

#define NOMINMAX

#include <Windows.h>
#include <malloc.h>

#include <initguid.h>
#include <Hidclass.h>
#include <SetupAPI.h>
#include <hidsdi.h>

#define CHECK_FOR_IO_ERROR(err) \
if (err != 0) {\
	if		(err == WAIT_TIMEOUT)					return DS5W_E_IO_TIMEDOUT;\
	else if (err == ERROR_DEVICE_NOT_CONNECTED)		return DS5W_E_DEVICE_REMOVED;\
	else if (err == ERROR_NOT_FOUND)				return DS5W_E_IO_NOT_FOUND;\
	else											return DS5W_E_IO_FAILED;\
}\

DS5W_ReturnValue DS5W::disableAllDeviceFeatures(DS5W::DeviceContext* ptrContext)
{
	// Get output report length and build buffer
	int outputReportLength = __DS5W::Output::createHIDOutputReportDisabled(ptrContext);

	// Write to controller
	DS5W_RV err = setOutputReport(ptrContext, outputReportLength, IO_TIMEOUT_MILLISECONDS);

	if (!DS5W_SUCCESS(err))
		return err;

	return DS5W_OK;
}

void DS5W::disconnectDevice(DS5W::DeviceContext* ptrContext)
{
	// Prevent further API IO calls by marking disconnected
	// internal IO calls are still allowed
	ptrContext->_internal.connected = false;

	// Ensure no outstanding IO calls
	CancelIoEx(ptrContext->_internal.deviceHandle, NULL);

	// Free in Windows
	CloseHandle(ptrContext->_internal.deviceHandle);
	ptrContext->_internal.deviceHandle = NULL;
}

DS5W_ReturnValue DS5W::getCalibrationData(DS5W::DeviceContext* ptrContext)
{
	// Make request for report
	ptrContext->_internal.hidInBuffer[0] = DS_FEATURE_REPORT_CALIBRATION;
	DWORD err = getHIDFeatureReportOverlapped(
		ptrContext->_internal.deviceHandle,
		&ptrContext->_internal.olRead,
		ptrContext->_internal.hidInBuffer,
		DS_FEATURE_REPORT_CALIBRATION_SIZE
	);

	CHECK_FOR_IO_ERROR(err);

	// make overlapped call synchronous by waiting here
	const int waitTime = 1000; // milliseconds
	err = AwaitOverlappedTimeout(
		ptrContext->_internal.deviceHandle,
		&ptrContext->_internal.olRead,
		waitTime);

	CHECK_FOR_IO_ERROR(err);

	// use calibration data to calculate constant values
	__DS5W::Input::parseCalibrationData(&ptrContext->_internal.calibrationData, (short*)(&ptrContext->_internal.hidInBuffer[1]));

	return DS5W_OK;
}

DS5W_ReturnValue DS5W::getInitialTimestamp(DS5W::DeviceContext* ptrContext)
{
	// Get a device input report to read the current time
	DS5W_RV err;
	const int waitTime = 500; // milliseconds
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		ptrContext->_internal.hidInBuffer[0] = DS_INPUT_REPORT_BT;
		err = getInputReport(ptrContext, DS_INPUT_REPORT_BT_SIZE, waitTime);
	}
	else {
		ptrContext->_internal.hidInBuffer[0] = DS_INPUT_REPORT_USB;
		err = getInputReport(ptrContext, DS_INPUT_REPORT_USB_SIZE, waitTime);
	}

	// check report was successfully read
	if (!DS5W_SUCCESS(err)) {
		return err;
	}

	// Evaluete input buffer
	int currentTime;
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		currentTime = *(unsigned int*)&ptrContext->_internal.hidInBuffer[0x1B + 2]; // BT buffer is offset by 2
	}
	else {
		currentTime = *(unsigned int*)&ptrContext->_internal.hidInBuffer[0x1B + 1]; // USB buffer is offset by 1
	}
	ptrContext->_internal.timestamp = currentTime;

	// OK
	return DS5W_OK;
}

DS5W_ReturnValue DS5W::getInputReport(DS5W::DeviceContext* ptrContext, USHORT reportLen, int waitTime)
{
	// Get the most recent package
	// This maybe should be removed? 
	// It increases average BT waiting time by 25%
	HidD_FlushQueue(ptrContext->_internal.deviceHandle);

	// start an overlapped read
	DWORD err = getHIDInputReportOverlapped(
		ptrContext->_internal.deviceHandle, 
		&ptrContext->_internal.olRead, 
		ptrContext->_internal.hidInBuffer, 
		reportLen);

	CHECK_FOR_IO_ERROR(err);
	
	// make overlapped call synchronous by waiting here
	err = AwaitOverlappedTimeout(
		ptrContext->_internal.deviceHandle, 
		&ptrContext->_internal.olRead, 
		waitTime);

	CHECK_FOR_IO_ERROR(err);

	// OK
	return DS5W_OK;
}

DS5W_ReturnValue DS5W::setOutputReport(DS5W::DeviceContext* ptrContext, USHORT reportLen, int waitTime)
{
	// start IO request and check it began correctly
	DWORD err = setHIDOutputReportOverlapped(
		ptrContext->_internal.deviceHandle,
		&ptrContext->_internal.olWrite,
		ptrContext->_internal.hidOutBuffer,
		reportLen);
	
	CHECK_FOR_IO_ERROR(err);

	// run request synchronously and check there were no errors
	err = AwaitOverlappedTimeout(
		ptrContext->_internal.deviceHandle,
		&ptrContext->_internal.olWrite,
		waitTime);

	CHECK_FOR_IO_ERROR(err);

	return DS5W_OK;
}

DS5W_ReturnValue DS5W::getInputReportOverlapped(DS5W::DeviceContext* ptrContext, USHORT reportLen)
{
	// Get the most recent package
	// This maybe should be removed? 
	// It increases average BT waiting time by 25%
	HidD_FlushQueue(ptrContext->_internal.deviceHandle);

	// start an overlapped read
	DWORD err = getHIDInputReportOverlapped(
		ptrContext->_internal.deviceHandle,
		&ptrContext->_internal.olRead,
		ptrContext->_internal.hidInBuffer,
		reportLen);

	CHECK_FOR_IO_ERROR(err);

	// OK
	return DS5W_OK;
}

DS5W_ReturnValue DS5W::setOutputReportOverlapped(DS5W::DeviceContext* ptrContext, USHORT reportLen)
{
	// start IO request and check it began correctly
	DWORD err = setHIDOutputReportOverlapped(
		ptrContext->_internal.deviceHandle,
		&ptrContext->_internal.olWrite,
		ptrContext->_internal.hidOutBuffer,
		reportLen);

	CHECK_FOR_IO_ERROR(err);

	return DS5W_OK;
}

DS5W_ReturnValue DS5W::awaitOverlappedIO(DS5W::DeviceContext* ptrContext, LPOVERLAPPED ol, int waitTime)
{
	// make overlapped call synchronous by waiting here
	DWORD err = AwaitOverlappedTimeout(
		ptrContext->_internal.deviceHandle,
		&ptrContext->_internal.olRead,
		waitTime);

	CHECK_FOR_IO_ERROR(err);

	// OK
	return DS5W_OK;
}