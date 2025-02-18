/*
	DualSenseWindows API
	https://github.com/mattdevv/DualSense-Windows

	Licensed under the MIT License (To be found in repository root directory)
*/
#pragma once

#include <DualSenseWindows/DSW_Api.h>
#include <DualSenseWindows/Device.h>
#include <DualSenseWindows/DS5State.h>
#include <DualSenseWindows/DeviceSpecs.h>

namespace DS5W {
	/// <summary>
	/// Tries to convert windows error codes into DS5W errors
	/// </summary>
	DS5W_ReturnValue convertSystemErrorCode(DWORD err);

	/// <summary>
	/// Set all DualSense features to off (rumble, lights, trigger-effects)
	/// </summary>
	/// <param name="ptrContext">Pointer to context</param>
	/// <returns>Error code of call</returns>
	DS5W_ReturnValue disableAllDeviceFeatures(DS5W::DeviceContext* ptrContext);

	/// <summary>
	/// Mark device as disconnected and unlink from Windows
	/// </summary>
	/// <param name="ptrContext">Device to be disconnected</param>
	void disconnectDevice(DS5W::DeviceContext* ptrContext);

	/// <summary>
	/// Populates the stored calibration data for a controller
	/// Only needs to be called once
	/// </summary>
	/// <param name="ptrContext">Device to get data for</param>
	/// <returns>Error code</returns>
	DS5W_ReturnValue getCalibrationData(DS5W::DeviceContext* ptrContext);

	/// <summary>
	/// Gets the current time according to a device and stores in the device's context
	/// </summary>
	/// <param name="ptrContext">Device to get time from</param>
	/// <returns>Error code</returns>
	DS5W_ReturnValue getInitialTimestamp(DS5W::DeviceContext* ptrContext);

	/// <summary>
	/// Get an input report and parse into an input state synchronously by calling the needed async functions internally
	/// </summary>
	/// <param name="ptrContext">Device to read from</param>
	/// <param name="length">Size of input report</param>
	/// <param name="waitTime">Maximum time to wait</param>
	/// <returns>Error code</returns>
	DS5W_ReturnValue getInputReport(DS5W::DeviceContext* ptrContext, USHORT reportLen, int waitTime);

	/// <summary>
	/// Parse an output state into an output report and send to the device synchronously by calling the needed async functions internally
	/// </summary>
	/// <param name="ptrContext">Device to write to</param>
	/// <param name="length">Size of output report</param>
	/// <param name="waitTime">Maximum time to wait</param>
	/// <returns>Error code</returns>
	DS5W_ReturnValue setOutputReport(DS5W::DeviceContext* ptrContext, USHORT reportLen, int waitTime);

	/// <summary>
	/// Begins a request to read an input report from a device
	/// Report will be copied to the context's read buffer which should not be used until this request is confirmed to have finished
	/// </summary>
	/// <param name="ptrContext">Device to read from</param>
	/// <param name="reportLen">Size of the input report</param>
	/// <returns>Error code</returns>
	DS5W_ReturnValue startInputRequest(DS5W::DeviceContext* ptrContext, USHORT reportLen);

	/// <summary>
	/// Begins a request to write an output report to a device
	/// Report will be copied from the context's write buffer which should not be used until this request is confirmed to have finished
	/// </summary>
	/// <param name="ptrContext">Device to write to</param>
	/// <param name="reportLen">Size of the output report</param>
	/// <returns>Error code</returns>
	DS5W_ReturnValue startOutputRequest(DS5W::DeviceContext* ptrContext, USHORT reportLen);

	/// <summary>
	/// Will await an IO request that was run overlapped
	/// Should only be used on requests that are run overlapped but seems to work regardless?
	/// </summary>
	/// <param name="ptrContext">Device doing request</param>
	/// <param name="ol">Synchronization struct used for request</param>
	/// <param name="waitTime">Maximum time to wait</param>
	/// <returns>Error code</returns>
	DS5W_ReturnValue awaitIORequest(DS5W::DeviceContext* ptrContext, LPOVERLAPPED ol, int waitTime);
}