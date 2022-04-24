/*
	DS5_Output.h is part of DualSenseWindows
	https://github.com/Ohjurot/DualSense-Windows

	Contributors of this file:
	11.2020 Ludwig Füchsl
	11.2021 Matthew Hall

	Licensed under the MIT License (To be found in repository root directory)
*/
#pragma once

#include <DualSenseWindows/DSW_Api.h>
#include <DualSenseWindows/Device.h>
#include <DualSenseWindows/DS5State.h>
#include <DualSenseWindows/DS_CRC32.h>

#include <Windows.h>

namespace __DS5W {
	namespace Output {
		/// <summary>
		/// Creates the hid output buffer
		/// </summary>
		/// <param name="hidOutBuffer">HID Output buffer</param>
		/// <param name="ptrOutputState">Pointer to state to read from</param>
		void createHidOutputBuffer(unsigned char* hidOutBuffer, DS5W::DS5OutputState* ptrOutputState);

		/// <summary>
		/// Creates the hid output buffer to disable all features (lights, rumble, etc.)
		/// </summary>
		/// <param name="hidOutBuffer">Pointer to start of output report (skipping report id)</param>
		void createHidOutputBufferDisabled(UCHAR* hidOutBuffer);

		/// <summary>
		/// Fills the context's output buffer with the HID report form of an output state
		/// </summary>
		/// <param name="ptrContext"></param>
		/// <param name="ptrOutputState"></param>
		/// <returns></returns>
		void createHIDOutputReport(DS5W::DeviceContext* ptrContext, DS5W::DS5OutputState* ptrOutputState, int* lengthOut);

		/// <summary>
		/// Fills the context's output buffer with an output state that disables all features (lights, rumble, etc.)
		/// </summary>
		/// <param name="ptrContext"></param>
		/// <param name="ptrOutputState"></param>
		/// <returns></returns>
		void createHIDOutputReportDisabled(DS5W::DeviceContext* ptrContext, int* lengthOut);

		/// <summary>
		/// Process trigger
		/// </summary>
		/// <param name="ptrEffect">Pointer to effect to be applied</param>
		/// <param name="buffer">Buffer for trigger parameters</param>
		void processTrigger(DS5W::TriggerEffect* ptrEffect, unsigned char* buffer);
	}
}