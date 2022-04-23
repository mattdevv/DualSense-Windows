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

void DS5W::setOutputStateOff(UCHAR* reportBuffer, USHORT reportLength)
{
	ZeroMemory(reportBuffer, reportLength);

	// Feature flags allow setting all device parameters
	// Enable flag to disable LEDs
	reportBuffer[0x00] = DS5W::DefaultOutputFlags & 0x00FF;
	reportBuffer[0x01] = ((DS5W::DefaultOutputFlags | (USHORT)DS5W::OutputFlags::DisableAllLED) & 0xFF00) >> 8;

	// set trigger effect to released instead of disabled
	// this will make them relax instantly
	reportBuffer[0x0A] = (UCHAR)TriggerEffectType::ReleaseAll;
	reportBuffer[0x15] = (UCHAR)TriggerEffectType::ReleaseAll;
}

void DS5W::disableAllDeviceFeatures(DS5W::DeviceContext* ptrContext)
{
	// Get output report length and build buffer
	UCHAR outputReportID;
	USHORT outputReportLength;
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		outputReportID = DS_OUTPUT_REPORT_BT;
		outputReportLength = DS_OUTPUT_REPORT_BT_SIZE;

		ptrContext->_internal.hidOutBuffer[0x01] = 0x02;	// magic value?

		// set state to off
		DS5W::setOutputStateOff(&ptrContext->_internal.hidOutBuffer[2], outputReportLength);

		// Hash
		const UINT32 crcChecksum = __DS5W::CRC32::compute(ptrContext->_internal.hidOutBuffer, DS_OUTPUT_REPORT_BT_SIZE - 4);
		ptrContext->_internal.hidOutBuffer[0x4A] = (unsigned char)((crcChecksum & 0x000000FF) >> 0UL);
		ptrContext->_internal.hidOutBuffer[0x4B] = (unsigned char)((crcChecksum & 0x0000FF00) >> 8UL);
		ptrContext->_internal.hidOutBuffer[0x4C] = (unsigned char)((crcChecksum & 0x00FF0000) >> 16UL);
		ptrContext->_internal.hidOutBuffer[0x4D] = (unsigned char)((crcChecksum & 0xFF000000) >> 24UL);
	}
	else {
		outputReportID = DS_OUTPUT_REPORT_USB;
		outputReportLength = DS_OUTPUT_REPORT_USB_SIZE;

		// set state to off
		DS5W::setOutputStateOff(&ptrContext->_internal.hidOutBuffer[1], outputReportLength);
	}

	// Write to controller
	DS5W_RV err = setOutputReport(ptrContext, outputReportID, outputReportLength);
}

void DS5W::disconnectDevice(DS5W::DeviceContext* ptrContext)
{
	// Prevent further IO calls by marking disconnected
	ptrContext->_internal.connected = false;

	// Ensure no outstanding IO calls
	CancelIoEx(ptrContext->_internal.deviceHandle, NULL);

	// Free in Windows
	CloseHandle(ptrContext->_internal.deviceHandle);
	ptrContext->_internal.deviceHandle = NULL;
}

DS5W_ReturnValue DS5W::getCalibrationData(DS5W::DeviceContext* ptrContext)
{
	DWORD err = getHIDFeatureReport(
		DS_FEATURE_REPORT_CALIBRATION,
		ptrContext->_internal.deviceHandle,
		&ptrContext->_internal.olRead,
		ptrContext->_internal.hidInBuffer,
		DS_FEATURE_REPORT_CALIBRATION_SIZE
	);

	if (err != 0)
	{
		if (err == ERROR_DEVICE_NOT_CONNECTED)	
			return DS5W_E_DEVICE_REMOVED;
		else									
			return DS5W_E_IO_FAILED;
	}

	// use calibration data to calculate constant values
	__DS5W::Input::parseCalibrationData(&ptrContext->_internal.calibrationData, (short*)(&ptrContext->_internal.hidInBuffer[1]));

	return DS5W_OK;
}

DS5W_ReturnValue DS5W::getInitialTimestamp(DS5W::DeviceContext* ptrContext)
{
	UCHAR inputReportID;
	USHORT inputReportLength;

	// get report info
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		inputReportID = DS_INPUT_REPORT_BT;
		inputReportLength = DS_INPUT_REPORT_BT_SIZE;
	}
	else {
		inputReportID = DS_INPUT_REPORT_USB;
		inputReportLength = DS_INPUT_REPORT_USB_SIZE;
	}

	// Get device input
	const int ONE_SECOND = 1000;
	DS5W_RV err = getInputReport(ptrContext, inputReportID, inputReportLength, ONE_SECOND);
	if (!DS5W_SUCCESS(err)) {
		// Return error
		return err;
	}

	// Evaluete input buffer
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		// timestamp is offset by 2 bytes if using bluetooth
		ptrContext->_internal.timestamp = *(unsigned int*)&ptrContext->_internal.hidInBuffer[2 + 0x1B];
	}
	else {
		// timestamp is offset by 1 byte if using usb
		ptrContext->_internal.timestamp = *(unsigned int*)&ptrContext->_internal.hidInBuffer[1 + 0x1B];
	}

	// Return ok
	return DS5W_OK;
}

DS5W_ReturnValue DS5W::getInputReport(DS5W::DeviceContext* ptrContext, UCHAR reportID, USHORT reportLen, int waitTime)
{
	// Get the most recent package
	// This maybe should be removed? 
	// It increases average BT waiting time by 25%
	HidD_FlushQueue(ptrContext->_internal.deviceHandle);

	// start an overlapped read
	DWORD err = getHIDInputReportOverlapped(
		reportID, ptrContext->_internal.deviceHandle, 
		&ptrContext->_internal.olRead, 
		ptrContext->_internal.hidInBuffer, 
		reportLen);

	// check for errors
	if (err != 0) {
		if (err == ERROR_DEVICE_NOT_CONNECTED)	return DS5W_E_DEVICE_REMOVED;
		else if (err == WAIT_TIMEOUT)			return DS5W_E_IO_TIMEDOUT;
		else                                    return DS5W_E_IO_FAILED;
	}
	
	// make overlapped call synchronous by waiting here
	err = AwaitOverlappedTimeout(
		ptrContext->_internal.deviceHandle, 
		&ptrContext->_internal.olRead, 
		waitTime);

	// check for errors
	if (err != 0) {
		if (err == ERROR_DEVICE_NOT_CONNECTED)	return DS5W_E_DEVICE_REMOVED;
		else if (err == WAIT_TIMEOUT)			return DS5W_E_IO_TIMEDOUT;
		else                                    return DS5W_E_IO_FAILED;
	}

	// OK
	return DS5W_OK;
}

DS5W_ReturnValue DS5W::setOutputReport(DS5W::DeviceContext* ptrContext, UCHAR reportID, USHORT reportLen)
{
	// start IO request and check it began correctly
	DWORD err = setHIDOutputReportOverlapped(
		reportID,
		ptrContext->_internal.deviceHandle,
		&ptrContext->_internal.olWrite,
		ptrContext->_internal.hidOutBuffer,
		reportLen);
	
	// Check for errors
	if (err != 0) {
		if (err == ERROR_DEVICE_NOT_CONNECTED)	return DS5W_E_DEVICE_REMOVED;
		else									return DS5W_E_IO_FAILED;
	}

	// run request synchronously and check there were no errors
	err = AwaitOverlapped(
		ptrContext->_internal.deviceHandle,
		&ptrContext->_internal.olWrite);

	// Check for errors
	if (err != 0) {
		if (err == ERROR_DEVICE_NOT_CONNECTED)	return DS5W_E_DEVICE_REMOVED;
		else									return DS5W_E_IO_FAILED;
	}

	return DS5W_OK;
}