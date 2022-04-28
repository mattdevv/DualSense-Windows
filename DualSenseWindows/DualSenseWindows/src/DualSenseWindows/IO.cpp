/*
	IO.cpp is part of DualSenseWindows
	https://github.com/mattdevv/DualSense-Windows

	Contributors of this file:
	11.2020 Ludwig Füchsl
	11.2021 Matthew Hall

	Licensed under the MIT License (To be found in repository root directory)
*/

#include <DualSenseWindows/IO.h>
#include <DualSenseWindows/DS_CRC32.h>
#include <DualSenseWindows/DS5_Input.h>
#include <DualSenseWindows/DS5_HID.h>
#include <DualSenseWindows/DS5_Internal.h>
#include <DualSenseWindows/DS5_Output.h>

#include <MurmurHash3/MurmurHash3.h>

#define NOMINMAX

#include <Windows.h>
#include <malloc.h>

#include <initguid.h>
#include <Hidclass.h>
#include <SetupAPI.h>
#include <hidsdi.h>

template <class T>
inline bool inArray(T& item, T* arr, uint32_t size)
{
	for (uint32_t i = 0; i < size; i++)
		if (arr[i] == item)
			return true;

	return false;
}

DS5W_API DS5W_ReturnValue DS5W::enumDevices(void* ptrBuffer, UINT inArrLength, UINT* requiredLength, bool pointerToArray) {

	// Check for invalid non expected buffer
	if (inArrLength == 0 || ptrBuffer == nullptr) {
		return DS5W_E_INVALID_ARGS;
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

			HANDLE deviceHandle = CreateFileW(devicePath->DevicePath, NULL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);

			// Check if device is reachable
			if (deviceHandle && (deviceHandle != INVALID_HANDLE_VALUE)) {

				// Skip devices with wrong IDs
				if (CheckDeviceAttributes(deviceHandle, SONY_CORP_VENDOR_ID, DUALSENSE_CONTROLLER_PROD_ID))
				{
					// skip if the output array is full
					if (inputArrIndex < inArrLength) {

						// Get valid pointer to target
						DS5W::DeviceEnumInfo* ptrInfo = nullptr;
						if (pointerToArray) {
							ptrInfo = &(((DS5W::DeviceEnumInfo*)ptrBuffer)[inputArrIndex]);
						}
						else {
							ptrInfo = (((DS5W::DeviceEnumInfo**)ptrBuffer)[inputArrIndex]);
						}

						USHORT inputReportLength = GetDeviceInputReportSize(deviceHandle);

						// Check if controller matches USB specifications
						if (inputReportLength == DS_INPUT_REPORT_USB_SIZE) {
							// Bluetooth device found and valid -> copy variables and increment index
							ptrInfo->_internal.connection = DS5W::DeviceConnection::USB;
							wcscpy_s(ptrInfo->_internal.path, 260, (const wchar_t*)devicePath->DevicePath);

							// Create unique identifier from device path
							unsigned int hashedPath;
							MurmurHash3_x86_32((void*)devicePath->DevicePath, requiredSize, ID_HASH_SEED, &hashedPath);
							ptrInfo->_internal.uniqueID = hashedPath;

							inputArrIndex++;
						}
						// Check if controler matches BT specifications
						else if (inputReportLength == DS_INPUT_REPORT_BT_SIZE) {
							// USB device found and valid -> copy variables and increment index
							ptrInfo->_internal.connection = DS5W::DeviceConnection::BT;
							wcscpy_s(ptrInfo->_internal.path, 260, (const wchar_t*)devicePath->DevicePath);

							// Create unique identifier from device path
							unsigned int hashedPath;
							MurmurHash3_x86_32((void*)devicePath->DevicePath, requiredSize, ID_HASH_SEED, &hashedPath);
							ptrInfo->_internal.uniqueID = hashedPath;

							inputArrIndex++;
						}
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
	
	const unsigned int numFoundDevices = inputArrIndex;

	// Set required size if exists
	if (requiredLength) {
		*requiredLength = numFoundDevices;
	}

	// Check if array was suficient
	if (numFoundDevices > inArrLength) {
		return DS5W_E_INSUFFICIENT_BUFFER;
	}

	return DS5W_OK;
}

DS5W_API DS5W_ReturnValue DS5W::enumUnknownDevices(void* ptrBuffer, UINT inArrLength, UINT* knownDeviceIDs, UINT numKnownDevices, UINT* requiredLength, bool pointerToArray) {
	// Check for empty known devices
	if (numKnownDevices == 0) {
		// can do quicker enumDevices call
		return enumDevices(ptrBuffer, inArrLength, requiredLength, pointerToArray);
	}

	// Check for invalid non expected buffer
	if (inArrLength == 0 || ptrBuffer == nullptr || knownDeviceIDs == nullptr)
	{
		return DS5W_E_INVALID_ARGS;
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

			HANDLE deviceHandle = CreateFileW(devicePath->DevicePath, NULL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);

			// Check if device is reachable
			if (deviceHandle && (deviceHandle != INVALID_HANDLE_VALUE)) {
				
				// Skip devices with wrong IDs
				if (CheckDeviceAttributes(deviceHandle, SONY_CORP_VENDOR_ID, DUALSENSE_CONTROLLER_PROD_ID))
				{
					// Create unique identifier from device path
					unsigned int hashedPath;
					MurmurHash3_x86_32((void*)devicePath->DevicePath, requiredSize, ID_HASH_SEED, &hashedPath);

					// skip devices which are already known
					if (!inArray(hashedPath, knownDeviceIDs, numKnownDevices))
					{
						// skip if output buffer is full
						if (inputArrIndex < inArrLength) {

							// Get pointer to target
							DS5W::DeviceEnumInfo* ptrInfo = nullptr;
							if (pointerToArray) {
								ptrInfo = &(((DS5W::DeviceEnumInfo*)ptrBuffer)[inputArrIndex]);
							}
							else {
								ptrInfo = (((DS5W::DeviceEnumInfo**)ptrBuffer)[inputArrIndex]);
							}

							USHORT inputReportLength = GetDeviceInputReportSize(deviceHandle);

							// Check if controller matches USB specifications
							if (inputReportLength == DS_INPUT_REPORT_USB_SIZE) {
								// Bluetooth device found and valid -> copy variables and increment index
								ptrInfo->_internal.connection = DS5W::DeviceConnection::USB;
								wcscpy_s(ptrInfo->_internal.path, 260, (const wchar_t*)devicePath->DevicePath);
								ptrInfo->_internal.uniqueID = hashedPath;

								inputArrIndex++;
							}
							// Check if controler matches BT specifications
							else if (inputReportLength == DS_INPUT_REPORT_BT_SIZE) {
								// USB device found and valid -> copy variables and increment index
								ptrInfo->_internal.connection = DS5W::DeviceConnection::BT;
								wcscpy_s(ptrInfo->_internal.path, 260, (const wchar_t*)devicePath->DevicePath);
								ptrInfo->_internal.uniqueID = hashedPath;

								inputArrIndex++;
							}
						}
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

	const unsigned int numFoundDevices = inputArrIndex;

	// Set required size if exists
	if (requiredLength) {
		*requiredLength = numFoundDevices;
	}

	// Check if array was suficient
	if (numFoundDevices > inArrLength) {
		return DS5W_E_INSUFFICIENT_BUFFER;
	}

	return DS5W_OK;
}

DS5W_API DS5W_ReturnValue DS5W::initDeviceContext(DS5W::DeviceEnumInfo* ptrEnumInfo, DS5W::DeviceContext* ptrContext) {
	// Check if pointers are valid
	if (!ptrEnumInfo || !ptrContext) {
		return DS5W_E_INVALID_ARGS;
	}

	// Check device path is set
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

	// Check success
	if (deviceHandle == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
			return DS5W_E_DEVICE_REMOVED;

		return DS5W_E_EXTERNAL_WINAPI;
	}

	// Copy device info to context
	ptrContext->_internal.connected = true;
	ptrContext->_internal.deviceHandle = deviceHandle;
	ptrContext->_internal.connectionType = ptrEnumInfo->_internal.connection;
	ptrContext->_internal.uniqueID = ptrEnumInfo->_internal.uniqueID;
	wcscpy_s(ptrContext->_internal.devicePath, 260, ptrEnumInfo->_internal.path);

	// create overlapped structs for IO
	memset(&(ptrContext->_internal.olRead), 0, sizeof(OVERLAPPED));
	memset(&(ptrContext->_internal.olWrite), 0, sizeof(OVERLAPPED));
	ptrContext->_internal.olRead.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	ptrContext->_internal.olWrite.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	// get calibration data so gyroscope/acceleration data can be decoded properly
	_DS5W_ReturnValue err = getCalibrationData(ptrContext);
	if (DS5W_FAILED(err)) {
		// Close handle and set error state
		disconnectDevice(ptrContext);
		return err;
	}

	// get timestamp so deltatime is valid
	err = getInitialTimestamp(ptrContext);
	if (DS5W_FAILED(err)) {
		// Close handle and set error state
		disconnectDevice(ptrContext);
		return err;
	}

	// Return OK
	return DS5W_OK;
}

DS5W_API void DS5W::freeDeviceContext(DS5W::DeviceContext* ptrContext) {
	// Check pointer
	if (!ptrContext) {
		return;
	}

	// Turn off controller first if still connected
	if (ptrContext->_internal.connected) {
		shutdownDevice(ptrContext);
	}

	// Free Windows events for I/O
	CloseHandle(ptrContext->_internal.olRead.hEvent);
	CloseHandle(ptrContext->_internal.olWrite.hEvent);

	// Clear path so it cannot be reused
	ptrContext->_internal.devicePath[0] = 0x0;
}

DS5W_API void DS5W::shutdownDevice(DS5W::DeviceContext* ptrContext)
{
	// Check pointer
	if (!ptrContext) {
		return;
	}

	// Skip if already disconnected
	if (ptrContext->_internal.connected == false)
		return;

	// Prevent further API IO calls by marking disconnected
	// internal IO calls are still allowed
	ptrContext->_internal.connected = false;

	// Internal IO call turns off all features to prevent infinite rumble
	disableAllDeviceFeatures(ptrContext);

	// mark as disconnected
	disconnectDevice(ptrContext);
}

DS5W_API DS5W_ReturnValue DS5W::reconnectDevice(DS5W::DeviceContext* ptrContext) {	
	// Check pointer
	if (!ptrContext) {
		return DS5W_E_INVALID_ARGS;
	}

	// Skip if already connected
	if (ptrContext->_internal.connected == true)
		return DS5W_OK;
	
	// Check length
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
		disconnectDevice(ptrContext);
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
	if (ptrContext->_internal.connected == false) {
		return DS5W_E_DEVICE_REMOVED;
	}

	DS5W_ReturnValue err;

	// Get device input
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		ptrContext->_internal.hidInBuffer[0] = DS_INPUT_REPORT_BT;
		err = getInputReport(ptrContext, DS_INPUT_REPORT_BT_SIZE, IO_TIMEOUT_MILLISECONDS);
	}
	else {
		ptrContext->_internal.hidInBuffer[0] = DS_INPUT_REPORT_USB;
		err = getInputReport(ptrContext, DS_INPUT_REPORT_USB_SIZE, IO_TIMEOUT_MILLISECONDS);
	}

	// error check
	if (DS5W_FAILED(err)) {
		if (err == DS5W_E_DEVICE_REMOVED){
			disconnectDevice(ptrContext);
		}
		return err;
	}

	// Evaluete input buffer
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		// Call bluetooth evaluator if connection is qual to BT
		__DS5W::Input::evaluateHidInputBuffer(&ptrContext->_internal.hidInBuffer[2], ptrInputState, ptrContext);
	} else {
		// Else it is USB so call its evaluator
		__DS5W::Input::evaluateHidInputBuffer(&ptrContext->_internal.hidInBuffer[1], ptrInputState, ptrContext);
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
	if (ptrContext->_internal.connected == false) {
		return DS5W_E_DEVICE_REMOVED;
	}
	
	// Fill internal buffer with correct HID report for connection type
	int outputReportLength = __DS5W::Output::createHIDOutputReport(ptrContext, ptrOutputState);

	// Send report to controller
	DS5W_RV err = setOutputReport(ptrContext, outputReportLength, IO_TIMEOUT_MILLISECONDS);

	// error check
	if (DS5W_FAILED(err)) {
		if (err == DS5W_E_DEVICE_REMOVED) {
			disconnectDevice(ptrContext);
		}
		return err;
	}

	// OK 
	return DS5W_OK;
}

DS5W_API DS5W_ReturnValue DS5W::startInputRequest(DS5W::DeviceContext* ptrContext)
{
	// Check pointer
	if (!ptrContext) {
		return DS5W_E_INVALID_ARGS;
	}

	// Check for connection
	if (ptrContext->_internal.connected == false) {
		return DS5W_E_DEVICE_REMOVED;
	}

	DS5W_ReturnValue err;

	// Start request for device input
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		ptrContext->_internal.hidInBuffer[0] = DS_INPUT_REPORT_BT;
		err = startInputRequest(ptrContext, DS_INPUT_REPORT_BT_SIZE);
	}
	else {
		ptrContext->_internal.hidInBuffer[0] = DS_INPUT_REPORT_USB;
		err = startInputRequest(ptrContext, DS_INPUT_REPORT_USB_SIZE);
	}

	// error check
	if (!DS5W_SUCCESS(err)) {
		if (err == DS5W_E_DEVICE_REMOVED) {
			disconnectDevice(ptrContext);
		}
		return err;
	}

	// Return ok
	return DS5W_OK;
}

DS5W_API DS5W_ReturnValue DS5W::awaitInputRequest(DS5W::DeviceContext* ptrContext)
{
	// Check pointer
	if (!ptrContext) {
		return DS5W_E_INVALID_ARGS;
	}

	// Check for connection
	if (ptrContext->_internal.connected == false) {
		return DS5W_E_DEVICE_REMOVED;
	}

	// block thread here until request is fulfilled or timeout
	DS5W_ReturnValue err = awaitIORequest(ptrContext, &ptrContext->_internal.olRead, IO_TIMEOUT_MILLISECONDS);

	// error check
	if (!DS5W_SUCCESS(err)) {
		if (err == DS5W_E_DEVICE_REMOVED) {
			disconnectDevice(ptrContext);
		}
		return err;
	}

	// Return ok
	return DS5W_OK;
}

DS5W_API void DS5W::getHeldInputState(DS5W::DeviceContext* ptrContext, DS5W::DS5InputState* ptrInputState)
{
	// Check pointer
	if (!ptrContext || !ptrInputState) {
		return;
	}

	// Evaluete input buffer
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		// bluetooth HID report is offset by 2
		__DS5W::Input::evaluateHidInputBuffer(&ptrContext->_internal.hidInBuffer[2], ptrInputState, ptrContext);
	}
	else {
		// usb HID report is offset by 1
		__DS5W::Input::evaluateHidInputBuffer(&ptrContext->_internal.hidInBuffer[1], ptrInputState, ptrContext);
	}
}