/*
	DualSenseWindows API
	https://github.com/Ohjurot/DualSense-Windows

	MIT License

	Copyright (c) 2020 Ludwig Füchsl

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.

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
