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

DS5W_ReturnValue DS5W::convertSystemErrorCode(DWORD err)
{
	if		(err == WAIT_TIMEOUT)					return DS5W_E_IO_TIMEDOUT;
	else if (err == ERROR_DEVICE_NOT_CONNECTED)		return DS5W_E_DEVICE_REMOVED;
	else if (err == ERROR_NOT_FOUND)				return DS5W_E_IO_NOT_FOUND;
	else                                            return DS5W_E_UNKNOWN;
}

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
	// need to set ID for report to request
	ptrContext->_internal.hidInBuffer[0] = DS_FEATURE_REPORT_CALIBRATION;

	// Start read request for report
	DWORD bytes_returned;
	ResetEvent(ptrContext->_internal.olRead.hEvent);
	BOOL res = DeviceIoControl(
		ptrContext->_internal.deviceHandle,
		IOCTL_HID_GET_FEATURE,
		ptrContext->_internal.hidInBuffer, 
		DS_FEATURE_REPORT_CALIBRATION_SIZE,
		ptrContext->_internal.hidInBuffer, 
		DS_FEATURE_REPORT_CALIBRATION_SIZE,
		&bytes_returned, 
		&ptrContext->_internal.olRead);

	// check if request was not fulfilled instantly
	if (!res) {
		// check whether request is running in background or failed
		DWORD err = GetLastError();
		if (err != ERROR_IO_PENDING) {
			return convertSystemErrorCode(err);
		}

		// else must be running in background
		// wait here for request to finish
		const int waitTime = 1000; // milliseconds, unsure how long it needs
		err = AwaitOverlappedTimeout(
			ptrContext->_internal.deviceHandle,
			&ptrContext->_internal.olRead,
			waitTime);

		// check request finished correctly
		if (err) {
			return convertSystemErrorCode(err);
		}
	}

	// use calibration data to calculate constant values
	__DS5W::Input::parseCalibrationData(&ptrContext->_internal.calibrationData, (short*)(&ptrContext->_internal.hidInBuffer[1]));

	return DS5W_OK;
}

DS5W_ReturnValue DS5W::getInitialTimestamp(DS5W::DeviceContext* ptrContext)
{
	DS5W_RV err;
	const int waitTime = 500; // milliseconds, this is much more than needed

	// Get a device input report to read the current time
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		ptrContext->_internal.hidInBuffer[0] = DS_INPUT_REPORT_BT;
		err = getInputReport(ptrContext, DS_INPUT_REPORT_BT_SIZE, waitTime);
	}
	else {
		ptrContext->_internal.hidInBuffer[0] = DS_INPUT_REPORT_USB;
		err = getInputReport(ptrContext, DS_INPUT_REPORT_USB_SIZE, waitTime);
	}

	// check report was successfully read
	if (DS5W_FAILED(err)) {
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
	DS5W_ReturnValue res = startInputRequest(ptrContext, reportLen);

	// result could have failed, or be running async
	if (DS5W_FAILED(res)) {

		// check if failed by error
		if (res != DS5W_E_IO_PENDING) {
			return res;
		}

		// else must be async, await here
		DWORD err = AwaitOverlappedTimeout(
			ptrContext->_internal.deviceHandle,
			&ptrContext->_internal.olRead,
			waitTime);

		// check async read was successful
		if (err) {
			return convertSystemErrorCode(err);
		}
	}

	// OK
	return DS5W_OK;
}

DS5W_ReturnValue DS5W::setOutputReport(DS5W::DeviceContext* ptrContext, USHORT reportLen, int waitTime)
{
	DS5W_ReturnValue res = startOutputRequest(ptrContext, reportLen);

	// result could have failed, or be running async
	if (DS5W_FAILED(res)) {

		// check if failed by error
		if (res != DS5W_E_IO_PENDING) {
			return res;
		}

		// else must be async, await here
		DWORD err = AwaitOverlappedTimeout(
			ptrContext->_internal.deviceHandle,
			&ptrContext->_internal.olWrite,
			waitTime);

		// check async read was successful
		if (err) {
			return convertSystemErrorCode(err);
		}
	}

	// OK
	return DS5W_OK;
}

DS5W_ReturnValue DS5W::startInputRequest(DS5W::DeviceContext* ptrContext, USHORT reportLen)
{
	// Get the most recent package
	// This maybe should be removed? 
	// It increases average BT waiting time by 25%
	HidD_FlushQueue(ptrContext->_internal.deviceHandle);

	// Start an overlapped read
	ResetEvent(ptrContext->_internal.olRead.hEvent);
	BOOL res = ReadFile(
		ptrContext->_internal.deviceHandle,
		ptrContext->_internal.hidInBuffer,
		reportLen,
		NULL,
		&ptrContext->_internal.olRead);

	// check if request was not fulfilled instantly
	if (!res) {
		// check whether request is running in background or failed
		DWORD err = GetLastError();
		if (err == ERROR_IO_PENDING) {
			return DS5W_E_IO_PENDING;
		}
		else {
			return convertSystemErrorCode(err);
		}
	}

	// Read was not overlapped and has finished
	return DS5W_OK;
}

DS5W_ReturnValue DS5W::startOutputRequest(DS5W::DeviceContext* ptrContext, USHORT reportLen)
{
	// Start an overlapped write
	ResetEvent(ptrContext->_internal.olWrite.hEvent);
	BOOL res = WriteFile(
		ptrContext->_internal.deviceHandle, 
		ptrContext->_internal.hidOutBuffer,
		reportLen,
		NULL, 
		&ptrContext->_internal.olWrite);

	// check if request was not fulfilled
	if (!res) {
		// check whether request is running in background or failed
		DWORD err = GetLastError();
		if (err == ERROR_IO_PENDING) {
			return DS5W_E_IO_PENDING;
		}
		else {
			return convertSystemErrorCode(err);
		}
	}

	return DS5W_OK;
}

DS5W_ReturnValue DS5W::awaitIORequest(DS5W::DeviceContext* ptrContext, LPOVERLAPPED ol, int waitTime)
{
	// make overlapped call synchronous by waiting here
	DWORD err = AwaitOverlappedTimeout(
		ptrContext->_internal.deviceHandle,
		ol,
		waitTime);

	if (err) {
		return convertSystemErrorCode(err);
	}

	// OK
	return DS5W_OK;
}