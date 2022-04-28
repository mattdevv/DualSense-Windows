/*
	DualSenseWindows API
	https://github.com/mattdevv/DualSense-Windows

	Licensed under the MIT License (To be found in repository root directory)
*/
#pragma once

#include <DualSenseWindows/Device.h>
#include <DualSenseWindows/DS5State.h>
#include <DualSenseWindows/DeviceSpecs.h>

namespace DS5W {
	/// <summary>
	/// Checks that the device has correct IDs
	/// </summary>
	/// <param name="device">Handle of device</param>
	/// <param name="vendorID">Desired vendor</param>
	/// <param name="productID">Desired product</param>
	/// <returns>Whether device meets criteria</returns>
	bool CheckDeviceAttributes(HANDLE device, USHORT vendorID, USHORT productID);

	/// <summary>
	/// Returns the device's input report size
	/// </summary>
	USHORT GetDeviceInputReportSize(HANDLE device);

	/// <summary>
	/// Block thread indefinitely until IO request is signalled
	/// Will never return if device is removed, so maybe dont use
	/// </summary>
	/// <param name="device">Device performing request</param>
	/// <param name="ol">Synchronisation struct</param>
	/// <returns>Error code of request</returns>
	DWORD AwaitOverlapped(HANDLE device, LPOVERLAPPED ol);

	/// <summary>
	/// Block thread until time elapses or IO request is signalled 
	/// </summary>
	/// <param name="device">Device performing request</param>
	/// <param name="ol">Synchronisation struct</param>
	/// <param name="milliseconds">Maximum time to wait</param>
	/// <returns>Error code of request</returns>
	DWORD AwaitOverlappedTimeout(HANDLE device, LPOVERLAPPED ol, int milliseconds);
}
