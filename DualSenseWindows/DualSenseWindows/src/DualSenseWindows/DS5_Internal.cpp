/*
	IO.cpp is part of DualSenseWindows
	https://github.com/Ohjurot/DualSense-Windows

	Contributors of this file:
	04.2022 Matthew Hall

	Licensed under the MIT License (To be found in repository root directory)
*/

#include <DualSenseWindows/DS5_Internal.h>
#include <DualSenseWindows/DS_CRC32.h>
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

void DS5W::disconnectDevice(DS5W::DeviceContext* ptrContext)
{
	// Prevent further IO calls by marking disconnected
	ptrContext->_internal.connected = false;

	// Ensure no outstanding IO calls
	CancelIo(ptrContext->_internal.deviceHandle);

	// Free in Windows
	CloseHandle(ptrContext->_internal.deviceHandle);
	ptrContext->_internal.deviceHandle = NULL;
}

void DS5W::disableAllDeviceFeatures(DS5W::DeviceContext* ptrContext)
{
	DS5OutputState os;
	DS5OutputState* ptrOutputState = &os;

	// Set all device features to off
	ZeroMemory(&os, sizeof(DS5W::DS5OutputState));
	os.leftTriggerEffect.effectType = TriggerEffectType::ReleaseAll;
	os.rightTriggerEffect.effectType = TriggerEffectType::ReleaseAll;
	os.disableLeds = true;

	// Get otuput report length
	unsigned short outputReportLength = 0;
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		// The bluetooth output report is 78 Bytes long
		outputReportLength = DS_OUTPUT_REPORT_BT_SIZE;
	}
	else {
		// The usb output report is 63 Bytes long
		outputReportLength = DS_OUTPUT_REPORT_USB_SIZE;
	}

	// Cleat all input data
	ZeroMemory(ptrContext->_internal.hidBuffer, outputReportLength);

	// Build output buffer
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		//return DS5W_E_CURRENTLY_NOT_SUPPORTED;
		// Report type
		ptrContext->_internal.hidBuffer[0x00] = DS_OUTPUT_REPORT_BT;
		ptrContext->_internal.hidBuffer[0x01] = 0x02;	// magic value?
		__DS5W::Output::createHidOutputBuffer(&ptrContext->_internal.hidBuffer[2], ptrOutputState);

		// Hash
		const UINT32 crcChecksum = __DS5W::CRC32::compute(ptrContext->_internal.hidBuffer, DS_OUTPUT_REPORT_BT_SIZE - 4);

		ptrContext->_internal.hidBuffer[0x4A] = (unsigned char)((crcChecksum & 0x000000FF) >> 0UL);
		ptrContext->_internal.hidBuffer[0x4B] = (unsigned char)((crcChecksum & 0x0000FF00) >> 8UL);
		ptrContext->_internal.hidBuffer[0x4C] = (unsigned char)((crcChecksum & 0x00FF0000) >> 16UL);
		ptrContext->_internal.hidBuffer[0x4D] = (unsigned char)((crcChecksum & 0xFF000000) >> 24UL);

	}
	else {
		// Report type
		ptrContext->_internal.hidBuffer[0x00] = DS_OUTPUT_REPORT_USB;

		// Else it is USB so call its evaluator
		__DS5W::Output::createHidOutputBuffer(&ptrContext->_internal.hidBuffer[1], ptrOutputState);
	}

	// Write to controller
	DS5W_RV err = setOutputReport(ptrContext, outputReportLength);
}

DS5W_ReturnValue DS5W::getCalibrationData(DS5W::DeviceContext* ptrContext)
{
	// set which report to read
	ptrContext->_internal.hidBuffer[0] = DS_FEATURE_REPORT_CALIBRATION;

	// get report data
	DS5W_RV err = getFeatureReport(ptrContext, DS_FEATURE_REPORT_CALIBRATION_SIZE);
	if (!DS5W_SUCCESS(err))
	{
		// Return error
		return err;
	}

	// use calibration data to calculate constant values
	__DS5W::Input::parseCalibrationData(&ptrContext->_internal.calibrationData, (short*)(&ptrContext->_internal.hidBuffer[1]));

	return DS5W_OK;
}

DS5W_ReturnValue DS5W::getInitialTimestamp(DS5W::DeviceContext* ptrContext)
{
	// Get the most recent package
	HidD_FlushQueue(ptrContext->_internal.deviceHandle);

	// Get input report length
	unsigned short inputReportLength = 0;
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		// The bluetooth input report is 78 Bytes long
		inputReportLength = DS_INPUT_REPORT_BT_SIZE;
		ptrContext->_internal.hidBuffer[0] = DS_OUTPUT_REPORT_BT;
	}
	else {
		// The usb input report is 64 Bytes long
		inputReportLength = DS_INPUT_REPORT_USB_SIZE;
		ptrContext->_internal.hidBuffer[0] = DS_INPUT_REPORT_USB;
	}

	// Get device input
	DS5W_RV err = getInputReport(ptrContext, inputReportLength, 100);
	if (!DS5W_SUCCESS(err)) {
		// Return error
		return err;
	}

	// Evaluete input buffer
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		// timestamp is offset by 2 bytes if using bluetooth
		ptrContext->_internal.timestamp = *(unsigned int*)&ptrContext->_internal.hidBuffer[2 + 0x1B];
	}
	else {
		// timestamp is offset by 1 byte if using usb
		ptrContext->_internal.timestamp = *(unsigned int*)&ptrContext->_internal.hidBuffer[1 + 0x1B];
	}

	// Return ok
	return DS5W_OK;
}

DS5W_ReturnValue DS5W::getInputReport(DS5W::DeviceContext* ptrContext, size_t length, int milliseconds)
{
	DWORD bytes_read = 0;
	BOOL res;

	// Start an Overlapped I/O read
	ResetEvent(ptrContext->_internal.olRead.hEvent);
	res = ReadFile(ptrContext->_internal.deviceHandle, ptrContext->_internal.hidBuffer, length, &bytes_read, &ptrContext->_internal.olRead);

	// check if request started correctly
	if (!res) {
		DWORD err = GetLastError();
		if (err == ERROR_DEVICE_NOT_CONNECTED)
		{
			disconnectDevice(ptrContext);
			return DS5W_E_DEVICE_REMOVED;
		}
		// anything but pending or 0 is an error
		else if (err != ERROR_IO_PENDING) {
			return DS5W_E_IO_FAILED_START;
		}
	}

	// wait for input report with timeout (if needed)
	if (milliseconds > 0) {
		res = WaitForSingleObject(ptrContext->_internal.olRead.hEvent, milliseconds);
		if (res != WAIT_OBJECT_0) {
			// WaitForSingleObject() timed out with no read
			// assume it failed and cleanup
			CancelIo(ptrContext->_internal.deviceHandle);
			return DS5W_E_IO_TIMEDOUT;
		}
	}

	// wait/check read ended correctly with infinite wait
	res = GetOverlappedResult(ptrContext->_internal.deviceHandle, &ptrContext->_internal.olRead, &bytes_read, TRUE);
	if (!res) {
		DWORD err = GetLastError();
		if (err == ERROR_DEVICE_NOT_CONNECTED)
		{
			disconnectDevice(ptrContext);
			return DS5W_E_DEVICE_REMOVED;
		}
		return DS5W_E_IO_FAILED;
	}

	/* bytes_read does not include the first byte which contains the
	   report ID. The data buffer actually contains one more byte than
	   bytes_read. */
	bytes_read++;

	return DS5W_OK;
}

DS5W_ReturnValue DS5W::setOutputReport(DS5W::DeviceContext* ptrContext, size_t length)
{
	DWORD bytes_written;
	BOOL res;

	// Start an overlapped write
	ResetEvent(ptrContext->_internal.olWrite.hEvent);
	res = WriteFile(ptrContext->_internal.deviceHandle, ptrContext->_internal.hidBuffer, length, &bytes_written, &ptrContext->_internal.olWrite);

	// check if request started correctly
	if (!res) {
		DWORD err = GetLastError();
		if (err == ERROR_DEVICE_NOT_CONNECTED)
		{
			disconnectDevice(ptrContext);
			return DS5W_E_DEVICE_REMOVED;
		}
		// anything but pending or 0 is an error
		else if (err != ERROR_IO_PENDING) {
			return DS5W_E_IO_FAILED_START;
		}
	}

	// wait/check write ended correctly with infinite wait
	res = GetOverlappedResult(ptrContext->_internal.deviceHandle, &ptrContext->_internal.olWrite, &bytes_written, TRUE/*wait*/);
	if (!res) {
		DWORD err = GetLastError();
		if (err == ERROR_DEVICE_NOT_CONNECTED)
		{
			disconnectDevice(ptrContext);
			return DS5W_E_DEVICE_REMOVED;
		}
		return DS5W_E_IO_FAILED;
	}

	return DS5W_OK;
}

DS5W_ReturnValue DS5W::getFeatureReport(DS5W::DeviceContext* ptrContext, size_t length)
{
	BOOL res;
	DWORD bytes_returned;
	OVERLAPPED ol;
	memset(&ol, 0, sizeof(ol));

	// start read request
	res = DeviceIoControl(ptrContext->_internal.deviceHandle,
		IOCTL_HID_GET_FEATURE,
		ptrContext->_internal.hidBuffer, length,
		ptrContext->_internal.hidBuffer, length,
		&bytes_returned, &ol);

	// check if request started correctly
	if (!res) {
		DWORD err = GetLastError();
		if (err == ERROR_DEVICE_NOT_CONNECTED)
		{
			disconnectDevice(ptrContext);
			return DS5W_E_DEVICE_REMOVED;
		}
		// anything but pending or 0 is an error
		else if (err != ERROR_IO_PENDING) {
			return DS5W_E_IO_FAILED_START;
		}
	}

	// wait/check read ended correctly with infinite wait
	res = GetOverlappedResult(ptrContext->_internal.deviceHandle, &ol, &bytes_returned, TRUE/*wait*/);
	if (!res) {
		DWORD err = GetLastError();
		if (err == ERROR_DEVICE_NOT_CONNECTED)
		{
			disconnectDevice(ptrContext);
			return DS5W_E_DEVICE_REMOVED;
		}
		return DS5W_E_IO_FAILED;
	}

	/* bytes_returned does not include the first byte which contains the
	   report ID. The data buffer actually contains one more byte than
	   bytes_returned. */
	bytes_returned++;

	return DS5W_OK;
}