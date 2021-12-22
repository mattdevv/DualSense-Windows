/*
	DS5_Input.h is part of DualSenseWindows
	https://github.com/Ohjurot/DualSense-Windows

	Contributors of this file:
	11.2020 Ludwig F�chsl
	11.2021 Matthew Hall

	Licensed under the MIT License (To be found in repository root directory)
*/
#pragma once

#include <DualSenseWindows/DSW_Api.h>
#include <DualSenseWindows/Device.h>
#include <DualSenseWindows/DS5State.h>

#include <Windows.h>

namespace __DS5W {
	namespace Input {
		/// <summary>
		/// Interprete the hid returned buffer 
		/// </summary>
		/// <param name="hidInBuffer">Input buffer</param>
		/// <param name="ptrInputState">Input state to be set</param>
		/// <returns></returns>
		void evaluateHidInputBuffer(unsigned char* hidInBuffer, DS5W::DS5InputState* ptrInputState, DS5W::DeviceCalibrationData* ptrCalibrationData);

		/// <summary>
		/// Extract necessary values from calibration report
		/// </summary>
		void parseCalibrationData(DS5W::DeviceCalibrationData* ptrCalibrationData, short* data);
	}
}