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
	/// fill buffer with HID input report where id = buffer[0]
	/// </summary>
	DWORD getHIDInputReport(UCHAR reportID, HANDLE device, OVERLAPPED* ol, UCHAR* buffer, size_t length, int milliseconds);

	/// <summary>
	/// copy from buffer to device the HID output report where id = buffer[0]
	/// </summary>
	DWORD setHIDOutputReport(UCHAR reportID, HANDLE device, OVERLAPPED* ol, UCHAR* buffer, size_t length);

	/// <summary>
	/// fill buffer with HID feature report where id = buffer[0]
	/// </summary>
	DWORD getHIDFeatureReport(UCHAR reportID, HANDLE device, OVERLAPPED* ol, UCHAR* buffer, size_t length);
}
