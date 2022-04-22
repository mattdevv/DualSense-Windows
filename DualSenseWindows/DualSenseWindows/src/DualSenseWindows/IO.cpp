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

DS5W_API DS5W_ReturnValue DS5W::enumDevices(void* ptrBuffer, unsigned int inArrLength, unsigned int* requiredLength, bool pointerToArray) {

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
					const unsigned int SEED = 0xABABABAB;
					unsigned int hashedPath;
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

DS5W_API DS5W_ReturnValue DS5W::enumUnknownDevices(void* ptrBuffer, unsigned int inArrLength, unsigned int* knownDeviceIDs, unsigned int numKnownDevices, unsigned int* requiredLength, bool pointerToArray) {
	// check for invalid pointer
	if (knownDeviceIDs == nullptr)
	{
		return DS5W_E_INVALID_ARGS;
	}
	
	// Check for empty known devices
	if (numKnownDevices == 0) {
		// can do quicker enumDevices call
		return enumDevices(ptrBuffer, inArrLength, requiredLength, pointerToArray);
	}

	// Check for invalid non expected buffer
	if (inArrLength == 0 || ptrBuffer == nullptr) 
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
				if (vendorId == SONY_CORP_VENDOR_ID && productId == DUALSENSE_CONTROLLER_PROD_ID) 
				{
					// Create unique identifier from device path
					const unsigned int SEED = 0xABABABAB;
					unsigned int hashedPath;
					MurmurHash3_x86_32((void*)devicePath->DevicePath, requiredSize, SEED, &hashedPath);

					// only check devices which are not already known
					if (!inArray(hashedPath, knownDeviceIDs, numKnownDevices))
					{
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
									else if (deviceCaps.InputReportByteLength == DS_INPUT_REPORT_BT_SIZE) {
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

	// Check length
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
	
	// get calibration data so gyroscope/acceleration data can be decoded properly
	_DS5W_ReturnValue err = getCalibrationData(ptrContext);
	if (!DS5W_SUCCESS(err))
		goto initFailed;

	// get timestamp so deltatime is valid
	err = getInitialTimestamp(ptrContext);
	if (!DS5W_SUCCESS(err))
		goto initFailed;

	// Return OK
	return DS5W_OK;

initFailed:
	// Close handle and set error state
	disconnectDevice(ptrContext);

	// Return error
	return err;
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
	
	// Unset bool
	ptrContext->_internal.connected = false;

	// Unset string
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

	// Turn off all features to prevent infinite rumble
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
	if (ptrContext->_internal.connected == false) {
		return DS5W_E_DEVICE_REMOVED;
	}

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
	DS5W_RV err = getInputReport(ptrContext, inputReportID, inputReportLength, IO_TIMEOUT_MILLISECONDS);

	// error check
	if (!DS5W_SUCCESS(err)) {
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
	
	UCHAR outputReportID;
	USHORT outputReportLength;

	// Get output report info and build buffer
	if (ptrContext->_internal.connectionType == DS5W::DeviceConnection::BT) {
		// report info
		outputReportID = DS_OUTPUT_REPORT_BT;
		outputReportLength = DS_OUTPUT_REPORT_BT_SIZE;

		// build buffer
		ZeroMemory(ptrContext->_internal.hidOutBuffer, outputReportLength);
		ptrContext->_internal.hidOutBuffer[0x01] = 0x02;	// magic value?
		__DS5W::Output::createHidOutputBuffer(&ptrContext->_internal.hidOutBuffer[2], ptrOutputState);

		// BT buffer also needs a hash
		const UINT32 crcChecksum = __DS5W::CRC32::compute(ptrContext->_internal.hidOutBuffer, DS_OUTPUT_REPORT_BT_SIZE - 4);
		ptrContext->_internal.hidOutBuffer[0x4A] = (unsigned char)((crcChecksum & 0x000000FF) >> 0UL);
		ptrContext->_internal.hidOutBuffer[0x4B] = (unsigned char)((crcChecksum & 0x0000FF00) >> 8UL);
		ptrContext->_internal.hidOutBuffer[0x4C] = (unsigned char)((crcChecksum & 0x00FF0000) >> 16UL);
		ptrContext->_internal.hidOutBuffer[0x4D] = (unsigned char)((crcChecksum & 0xFF000000) >> 24UL);
	}
	else {
		// report info
		outputReportID = DS_OUTPUT_REPORT_USB;
		outputReportLength = DS_OUTPUT_REPORT_USB_SIZE;

		// build buffer
		ZeroMemory(ptrContext->_internal.hidOutBuffer, outputReportLength);
		__DS5W::Output::createHidOutputBuffer(&ptrContext->_internal.hidOutBuffer[1], ptrOutputState);
	}

	// Write to controller
	DS5W_RV err = setOutputReport(ptrContext, outputReportID, outputReportLength);

	// error check
	if (!DS5W_SUCCESS(err)) {
		if (err == DS5W_E_DEVICE_REMOVED) {
			disconnectDevice(ptrContext);
		}
		return err;
	}

	// OK 
	return DS5W_OK;
}