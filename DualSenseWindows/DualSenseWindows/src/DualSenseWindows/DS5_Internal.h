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

#include <DualSenseWindows/DSW_Api.h>
#include <DualSenseWindows/Device.h>
#include <DualSenseWindows/DS5State.h>
#include <DualSenseWindows/DeviceSpecs.h>

namespace DS5W {

	/// <summary>
	/// Disconnect from windows and mark device as disconnected
	/// </summary>
	/// <param name="ptrContext">Device to be disconnected</param>
	void disconnectDevice(DS5W::DeviceContext* ptrContext);

	/// <summary>
	/// Set all DualSense features to off (rumble, lights, trigger-effects)
	/// </summary>
	/// <param name="ptrContext">Pointer to context</param>
	void disableAllDeviceFeatures(DS5W::DeviceContext* ptrContext);

	DS5W_ReturnValue getCalibrationData(DS5W::DeviceContext* ptrContext);
	DS5W_ReturnValue getInitialTimestamp(DS5W::DeviceContext* ptrContext);

	/// <summary>
	/// Get input report
	/// </summary>
	/// <param name="ptrContext">Pointer to context</param>
	/// <param name="length">Size of input report</param>
	/// <param name="milliseconds">Maximum time to wait (0 = infinite)</param>
	/// <returns>Error code of call</returns>
	DS5W_ReturnValue getInputReport(DS5W::DeviceContext* ptrContext, size_t length, int milliseconds);
	DS5W_ReturnValue setOutputReport(DS5W::DeviceContext* ptrContext, size_t length);
	DS5W_ReturnValue getFeatureReport(DS5W::DeviceContext* ptrContext, size_t length);
}