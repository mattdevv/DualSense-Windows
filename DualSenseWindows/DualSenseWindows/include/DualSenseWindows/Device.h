/*
	Device.h is part of DualSenseWindows
	https://github.com/Ohjurot/DualSense-Windows

	Contributors of this file:
	11.2020 Ludwig F�chsl
	11.2021 Matthew Hall

	Licensed under the MIT License (To be found in repository root directory)
*/
#pragma once

#include <DualSenseWindows/DeviceSpecs.h>

// more accurate integer multiplication by a fraction
constexpr int mult_frac(int x, int numer, int denom)
{
	int quot = x / denom;
	int rem = x % denom;
	return quot * numer + (rem * numer) / denom;
}

namespace DS5W {
	/// <summary>
	/// Storage for calibration values used to parse raw motion data
	/// </summary>
	typedef struct _AxisCalibrationData {
		short bias;
		int sens_numer;
		int sens_denom;

		int calibrate(int rawValue)
		{
			return mult_frac(sens_numer, rawValue - bias, sens_denom);
		}
	} AxisCalibrationData;

	typedef struct _DeviceCalibrationData {
		/// <summary>
			/// Values to calibrate controller's accelerometer and gyroscope
			/// </summary>
		AxisCalibrationData accel_calib_data[3];

		/// <summary>
		/// Values to calibrate controller's gyroscope
		/// </summary>
		AxisCalibrationData gyro_calib_data[3];
	} DeviceCalibrationData;

	/// <summary>
	/// Enum for device connection type
	/// </summary>
	typedef enum class _DeviceConnection : unsigned char {
		/// <summary>
		/// Controler is connected via USB
		/// </summary>
		USB = 0,

		/// <summary>
		/// Controler is connected via bluetooth
		/// </summary>
		BT = 1,
	} DeviceConnection;

	/// <summary>
	/// Struckt for storing device enum info while device discovery
	/// </summary>
	typedef struct _DeviceEnumInfo {
		/// <summary>
		/// Encapsulate data in struct to (at least try) prevent user from modifing the context
		/// </summary>
		struct {
			/// <summary>
			/// Path to the discovered device
			/// </summary>
			wchar_t path[260];

			/// <summary>
			/// Connection type of the discoverd device
			/// </summary>
			DeviceConnection connection;
		} _internal;
	} DeviceEnumInfo;

	/// <summary>
	/// Device context
	/// </summary>
	typedef struct _DeviceContext {
		/// <summary>
		/// Encapsulate data in struct to (at least try) prevent user from modifing the context
		/// </summary>
		struct {
			/// <summary>
			/// Path to the device
			/// </summary>
			wchar_t devicePath[260];

			/// <summary>
			/// Handle to the open device
			/// </summary>
			void* deviceHandle;

			/// <summary>
			/// Connection of the device
			/// </summary>
			DeviceConnection connection;

			/// <summary>
			/// Collection of values required to parse controller's motion data
			/// </summary>
			DeviceCalibrationData calibrationData;

			/// <summary>
			/// Current state of connection
			/// </summary>
			bool connected;

			/// <summary>
			/// HID Input buffer (will be allocated by the context init function)
			/// </summary>
			unsigned char hidBuffer[78];
		}_internal;
	} DeviceContext;
}