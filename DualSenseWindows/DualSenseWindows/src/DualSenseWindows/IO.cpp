/*
	IO.cpp is part of DualSenseWindows
	https://github.com/Ohjurot/DualSense-Windows

	Contributors of this file:
	11.2020 Ludwig Füchsl
	11.2021 Matthew Hall

	Licensed under the MIT License (To be found in repository root directory)
*/

#include <DualSenseWindows/IO.h>
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

DS5W_API DS5W_ReturnValue DS5W::enumDevices(void* ptrBuffer, unsigned int inArrLength, unsigned int* requiredLength, bool pointerToArray) {
	// Check for invalid non expected buffer
	if (inArrLength && !ptrBuffer) {
		inArrLength = 0;
	}

	// Get all hid devices from devs
	HANDLE hidDiHandle = SetupDiGetClassDevs(&GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if (!hidDiHandle || (hidDiHandle == INVALID_HANDLE_VALUE)) {
		return DS5W_E_EXTERNAL_WINAPI;
	}

	// Index into input array
	unsigned int inputArrIndex = 0;
	bool inputArrOverflow = false;

	// Enumerate over hid device
	DWORD devIndex = 0;
	SP_DEVINFO_DATA hidDiInfo;
	hidDiInfo.cbSize = sizeof(SP_DEVINFO_DATA);
	while (SetupDiEnumDeviceInfo(hidDiHandle, devIndex, &hidDiInfo)) {
		
		// Enumerate over all hid device interfaces
		DWORD ifIndex = 0;
		SP_DEVICE_INTERFACE_DATA ifDiInfo;
		ifDiInfo.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		while (SetupDiEnumDeviceInterfaces(hidDiHandle, &hidDiInfo, &GUID_DEVINTERFACE_HID, ifIndex, &ifDiInfo)) {

			// Query device path size
			DWORD requiredSize = 0;
			SetupDiGetDeviceInterfaceDetailW(hidDiHandle, &ifDiInfo, NULL, 0, &requiredSize, NULL);

			// Check size
			if (requiredSize > (260 * sizeof(wchar_t))) {
				SetupDiDestroyDeviceInfoList(hidDiHandle);
				return DS5W_E_EXTERNAL_WINAPI;
			}

			// Allocate memory for path on the stack
			SP_DEVICE_INTERFACE_DETAIL_DATA_W* devicePath = (SP_DEVICE_INTERFACE_DETAIL_DATA_W*)_malloca(requiredSize);
			if (!devicePath) {
				SetupDiDestroyDeviceInfoList(hidDiHandle);
				return DS5W_E_STACK_OVERFLOW;
			}
			
			// Get device path
			devicePath->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
			SetupDiGetDeviceInterfaceDetailW(hidDiHandle, &ifDiInfo, devicePath, requiredSize, NULL, NULL);

			// Check if input array has space
			// Check if device is reachable
			HANDLE deviceHandle = CreateFileW(
				devicePath->DevicePath, 
				NULL,
				FILE_SHARE_READ | FILE_SHARE_WRITE, 
				NULL, 
				OPEN_EXISTING, 
				NULL, 
				NULL);

			// Check if device is reachable
			if (deviceHandle && (deviceHandle != INVALID_HANDLE_VALUE)) {
				// Get vendor and product id
				unsigned int vendorId = 0;
				unsigned int productId = 0;
				HIDD_ATTRIBUTES deviceAttributes;
				if (HidD_GetAttributes(deviceHandle, &deviceAttributes)) {
					vendorId = deviceAttributes.VendorID;
					productId = deviceAttributes.ProductID;
				}

				// Check if ids match
				if (vendorId == SONY_CORP_VENDOR_ID && productId == DUALSENSE_CONTROLLER_PROD_ID) {
					// Create unique identifier from device path
					const UINT32 SEED = 0xABABABAB;
					UINT32 hashedPath;
					MurmurHash3_x86_32((void*)devicePath->DevicePath, requiredSize, SEED, &hashedPath);

					// Get pointer to target
					DS5W::DeviceEnumInfo* ptrInfo = nullptr;
					if (inputArrIndex < inArrLength) {
						if (pointerToArray) {
							ptrInfo = &(((DS5W::DeviceEnumInfo*)ptrBuffer)[inputArrIndex]);
						}
						else {
							ptrInfo = (((DS5W::DeviceEnumInfo**)ptrBuffer)[inputArrIndex]);
						}
					}

					// Copy path
					if (ptrInfo) {
						wcscpy_s(ptrInfo->_internal.path, 260, (const wchar_t*)devicePath->DevicePath);
						ptrInfo->_internal.uniqueID = hashedPath;
					}

					// Get preparsed data
					PHIDP_PREPARSED_DATA ppd;
					if (HidD_GetPreparsedData(deviceHandle, &ppd)) {
						// Get device capcbilitys
						HIDP_CAPS deviceCaps;
						if (HidP_GetCaps(ppd, &deviceCaps) == HIDP_STATUS_SUCCESS) {
							// Check for device connection type
							if (ptrInfo) {
								// Check if controller matches USB specifications
								if (deviceCaps.InputReportByteLength == DS_INPUT_REPORT_USB_SIZE) {
									ptrInfo->_internal.connection = DS5W::DeviceConnection::USB;

									// Device found and valid -> Increment index
									inputArrIndex++;
								}
								// Check if controler matches BT specifications
								else if(deviceCaps.InputReportByteLength == DS_INPUT_REPORT_BT_SIZE) {
									ptrInfo->_internal.connection = DS5W::DeviceConnection::BT;

									// Device found and valid -> Increment index
									inputArrIndex++;
								}
							}
						}

						// Free preparsed data
						HidD_FreePreparsedData(ppd);
					}
				}

				// Close device
				CloseHandle(deviceHandle);
			}

			// Increment index
			ifIndex++;

			// Free device from stack
			_freea(devicePath);
		}

		// Increment index
		devIndex++;
	}

	// Close device enum list
	SetupDiDestroyDeviceInfoList(hidDiHandle);
	
	// Set required size if exists
	if (requiredLength) {
		*requiredLength = inputArrIndex;
	}

	// Check if array was suficient
	if (inputArrIndex <= inArrLength) {
		return DS5W_OK;
	}
	// Else return error
	else {
		return DS5W_E_INSUFFICIENT_BUFFER;
	}
}

DS5W_API DS5W_ReturnValue DS5W::initDeviceContext(DS5W::DeviceEnumInfo* ptrEnumInfo, DS5W::DeviceContext* ptrContext) {
	// Check if pointers are valid
	if (!ptrEnumInfo || !ptrContext) {
		return DS5W_E_INVALID_ARGS;
	}

	// Check len
	if (wcslen(ptrEnumInfo->_internal.path) == 0) {
		return DS5W_E_INVALID_ARGS;
	}

	// Connect to device
	HANDLE deviceHandle = CreateFileW(
		ptrEnumInfo->_internal.path,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, 
		NULL,
		OPEN_EXISTING, 
		FILE_FLAG_OVERLAPPED, 
		NULL);

	if (!deviceHandle || (deviceHandle == INVALID_HANDLE_VALUE)) {
		CloseHandle(deviceHandle);
		return DS5W_E_DEVICE_REMOVED;
	}

	// Write to context
	ptrContext->_internal.connected = true;
	ptrContext->_internal.connectionType = ptrEnumInfo->_internal.connection;
	ptrContext->_internal.deviceHandle = deviceHandle;
	ptrContext->_internal.uniqueID = ptrEnumInfo->_internal.uniqueID;
	wcscpy_s(ptrContext->_internal.devicePath, 260, ptrEnumInfo->_internal.path);

	// create overlapped struct for reading
	memset(&(ptrContext->_internal.olRead), 0, sizeof(OVERLAPPED));
	ptrContext->_internal.olRead.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// create overlapped struct for writing
	memset(&(ptrContext->_internal.olWrite), 0, sizeof(OVERLAPPED));
	ptrContext->_internal.olWrite.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	// get calibration data
	_DS5W_ReturnValue err = getCalibrationReport(ptrContext);
	if (!DS5W_SUCCESS(err))
	{
		// Close handle and set error state
		DisconnectController(ptrContext);

		// Return error
		return err;
	}

	// get timestamp so deltatime is valid
	err = getInitialTimestamp(ptrContext);
	if (!DS5W_SUCCESS(err))
	{
		// Close handle and set error state
		DisconnectController(ptrContext);

		// Return error
		return err;
	}

	// Return OK
	return DS5W_OK;
}

DS5W_API void DS5W::freeDeviceContext(DS5W::DeviceContext* ptrContext) {
	// Check if handle is existing
	if (ptrContext->_internal.connected) {
		// Send zero output report to disable all onging outputs
		DS5W::DS5OutputState os;
		ZeroMemory(&os, sizeof(DS5W::DS5OutputState));
		os.leftTriggerEffect.effectType = TriggerEffectType::ReleaseAll;
		os.rightTriggerEffect.effectType = TriggerEffectType::ReleaseAll;
		os.disableLeds = true;

		DS5W::setDeviceOutputState(ptrContext, &os);

		DisconnectController(ptrContext);
	}

	CloseHandle(ptrContext->_internal.olRead.hEvent);
	CloseHandle(ptrContext->_internal.olWrite.hEvent);
	
	// Unset bool
	ptrContext->_internal.connected = false;

	// Unset string
	ptrContext->_internal.devicePath[0] = 0x0;
}

DS5W_API DS5W_ReturnValue DS5W::reconnectDevice(DS5W::DeviceContext* ptrContext) {	
	// Check len
	if (wcslen(ptrContext->_internal.devicePath) == 0) {
		return DS5W_E_INVALID_ARGS;
	}

	// Connect to device
	HANDLE deviceHandle = CreateFileW(ptrContext->_internal.devicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	if (!deviceHandle || (deviceHandle == INVALID_HANDLE_VALUE)) {
		return DS5W_E_DEVICE_REMOVED;
	}

	// Write to context
	ptrContext->_internal.connected = true;
	ptrContext->_internal.deviceHandle = deviceHandle;

	// refresh previous timestamp
	_DS5W_ReturnValue err = getInitialTimestamp(ptrContext);
	if (!DS5W_SUCCESS(err))
	{
		// Close handle and set error state
		DisconnectController(ptrContext);

		// Return error
		return err;
	}

	// Return ok
	return DS5W_OK;
}

DS5W_API DS5W_ReturnValue DS5W::getDeviceInputState(DS5W::DeviceContext* ptrContext, DS5W::DS5InputState* ptrInputState) {
	// Check pointer
	if (!ptrContext || !ptrInputState) {
		return DS5W_E_INVALID_ARGS;
	}

	// Check for connection
	if (!ptrContext->_internal.connected) {
		return DS5W_E_DEVICE_REMOVED;
	}

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
	const int timeoutMS = 50;
	DS5W_RV err = getInputReport(ptrContext, inputReportLength, timeoutMS);

	if (!DS5W_SUCCESS(err)) {
		// Close handle and set error state
		DisconnectController(ptrContext);

		// Return error
		return err;
	}

	// Evaluete input buffer
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		// Call bluetooth evaluator if connection is qual to BT
		__DS5W::Input::evaluateHidInputBuffer(&ptrContext->_internal.hidBuffer[2], ptrInputState, ptrContext);
	} else {
		// Else it is USB so call its evaluator
		__DS5W::Input::evaluateHidInputBuffer(&ptrContext->_internal.hidBuffer[1], ptrInputState, ptrContext);
	}
	
	// Return ok
	return DS5W_OK;
}

DS5W_API DS5W_ReturnValue DS5W::setDeviceOutputState(DS5W::DeviceContext* ptrContext, DS5W::DS5OutputState* ptrOutputState) {
	// Check pointer
	if (!ptrContext || !ptrOutputState) {
		return DS5W_E_INVALID_ARGS;
	}

	// Check for connection
	if (!ptrContext->_internal.connected) {
		return DS5W_E_DEVICE_REMOVED;
	}

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
	if (!DS5W_SUCCESS(err)) {
		// Close handle and set error state
		DisconnectController(ptrContext);

		// Return error
		return err;
	}

	// OK 
	return DS5W_OK;
}

void DS5W::DisconnectController(DS5W::DeviceContext* ptrContext)
{
	CloseHandle(ptrContext->_internal.deviceHandle);
	ptrContext->_internal.deviceHandle = NULL;
	ptrContext->_internal.connected = false;
}

DS5W_ReturnValue DS5W::getCalibrationReport(DS5W::DeviceContext* ptrContext)
{
	// Check pointer
	if (!ptrContext) {
		return DS5W_E_INVALID_ARGS;
	}

	// Check for connection
	if (!ptrContext->_internal.connected) {
		return DS5W_E_DEVICE_REMOVED;
	}

	// set which report to read
	ptrContext->_internal.hidBuffer[0] = DS_FEATURE_REPORT_CALIBRATION;

	// get report data
	DS5W_RV err = getFeatureReport(ptrContext, DS_FEATURE_REPORT_CALIBRATION_SIZE);
	if (!DS5W_SUCCESS(err))
	{
		// Close handle and set error state
		DisconnectController(ptrContext);

		// Return error
		return err;
	}

	// use calibration data to calculate constant values
	__DS5W::Input::parseCalibrationData(&ptrContext->_internal.calibrationData, (short*)(&ptrContext->_internal.hidBuffer[1]));

	return DS5W_OK;
}

DS5W_ReturnValue DS5W::getInitialTimestamp(DS5W::DeviceContext* ptrContext)
{
	// Check pointer
	if (!ptrContext) {
		return DS5W_E_INVALID_ARGS;
	}

	// Check for connection
	if (!ptrContext->_internal.connected) {
		return DS5W_E_DEVICE_REMOVED;
	}

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
		// Close handle and set error state
		DisconnectController(ptrContext);

		// Return error
		return err;
	}

	// Evaluete input buffer
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		// offset by 2 bytes if using bluetooth
		ptrContext->_internal.lastTimestamp = *(unsigned int*)&ptrContext->_internal.hidBuffer[2 + 0x1B];
	}
	else {
		// offset by 1 bytes if using usb
		ptrContext->_internal.lastTimestamp = *(unsigned int*)&ptrContext->_internal.hidBuffer[1 + 0x1B];
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

	if (!res) {
		if (GetLastError() != ERROR_IO_PENDING) {
			// ReadFile() has failed
			// Clean up and return error
			CancelIo(ptrContext->_internal.deviceHandle);
			return DS5W_E_IO_FAILED;
		}
	}
	
	if (milliseconds >= 0) {
		// check if a report was found
		res = WaitForSingleObject(ptrContext->_internal.olRead.hEvent, milliseconds);
		if (res != WAIT_OBJECT_0) {
			// WaitForSingleObject() timed out with no read
			// assume it failed and cleanup
			CancelIo(ptrContext->_internal.deviceHandle);
			return DS5W_E_IO_TIMEOUT;
		}
	}

	/* Either WaitForSingleObject() told us that ReadFile has completed, or
	   we are in non-blocking mode. Get the number of bytes read. The actual
	   data has been copied to the data[] array which was passed to ReadFile(). */
	res = GetOverlappedResult(ptrContext->_internal.deviceHandle, &ptrContext->_internal.olRead, &bytes_read, TRUE);

	if (!res) {
		return DS5W_E_IO_FAILED;
	}

	return DS5W_OK;
}

DS5W_ReturnValue DS5W::setOutputReport(DS5W::DeviceContext* ptrContext, size_t length)
{
	DWORD bytes_written;
	BOOL res;

	ResetEvent(ptrContext->_internal.olWrite.hEvent);
	res = WriteFile(ptrContext->_internal.deviceHandle, ptrContext->_internal.hidBuffer, length, NULL, &ptrContext->_internal.olWrite);

	if (!res) {
		if (GetLastError() != ERROR_IO_PENDING) {
			/* WriteFile() failed. Return error. */
			return DS5W_E_IO_FAILED;
		}
	}

	/* Wait here until the write is done. This makes
	   hid_write() synchronous. */
	res = GetOverlappedResult(ptrContext->_internal.deviceHandle, &ptrContext->_internal.olWrite, &bytes_written, TRUE/*wait*/);
	if (!res) {
		/* The Write operation failed. */
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

	res = DeviceIoControl(ptrContext->_internal.deviceHandle,
		IOCTL_HID_GET_FEATURE,
		ptrContext->_internal.hidBuffer, length,
		ptrContext->_internal.hidBuffer, length,
		&bytes_returned, &ol);

	if (!res) {
		if (GetLastError() != ERROR_IO_PENDING) {
			/* DeviceIoControl() failed. Return error. */
			return DS5W_E_IO_FAILED;
		}
	}

	/* Wait here until the write is done. This makes
	   hid_get_feature_report() synchronous. */
	res = GetOverlappedResult(ptrContext->_internal.deviceHandle, &ol, &bytes_returned, TRUE/*wait*/);
	if (!res) {
		/* The operation failed. */
		return DS5W_E_IO_FAILED;
	}

	/* bytes_returned does not include the first byte which contains the
	   report ID. The data buffer actually contains one more byte than
	   bytes_returned. */
	bytes_returned++;

	return DS5W_OK;
}