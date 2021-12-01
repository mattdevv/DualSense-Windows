/*
	Device.h is part of DualSenseWindows
	https://github.com/Ohjurot/DualSense-Windows

	Contributors of this file:
	11.2020 Ludwig F�chsl
	11.2021 mattdevv

	Licensed under the MIT License (To be found in repository root directory)
*/
#pragma once

#include <DualSenseWindows/DeviceSpecs.h>

namespace DS5W {
	typedef struct _CalibrationData {
		short bias;
		int sens_numer;
		int sens_denom;
	} CalibrationData;

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

			CalibrationData accel_calib_data[3];
			CalibrationData gyro_calib_data[3];

			/// <summary>
			/// Current state of connection
			/// </summary>
			bool connected;

			/// <summary>
			/// HID Input buffer (will be allocated by the context init function)
			/// </summary>
			unsigned char hidBuffer[547];
		}_internal;
	} DeviceContext;
}