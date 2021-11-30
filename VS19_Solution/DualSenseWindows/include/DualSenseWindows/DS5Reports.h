/*
	DS5State.h is part of DualSenseWindows
	https://github.com/Ohjurot/DualSense-Windows

	Contributors of this file:
	11.2020 Ludwig F�chsl
	11.2021 mattdevv

	Licensed under the MIT License (To be found in repository root directory)
*/
#pragma once

#define DS_FEATURE_REPORT_CALIBRATION			0x05
#define DS_FEATURE_REPORT_CALIBRATION_SIZE		41
#define DS_FEATURE_REPORT_PAIRING_INFO			0x09
#define DS_FEATURE_REPORT_PAIRING_INFO_SIZE		20
#define DS_FEATURE_REPORT_FIRMWARE_INFO			0x20
#define DS_FEATURE_REPORT_FIRMWARE_INFO_SIZE	64

#define DS_ACC_RES_PER_G	8192
#define DS_ACC_RANGE		(4*DS_ACC_RES_PER_G)
#define DS_GYRO_RES_PER_DEG_S	1024
#define DS_GYRO_RANGE		(2048*DS_GYRO_RES_PER_DEG_S)
#define DS_TOUCHPAD_WIDTH	1920
#define DS_TOUCHPAD_HEIGHT	1080

namespace DS5W {
	
	typedef struct _CalibrationReport {
		short gyro_pitch_bias;
		short gyro_yaw_bias;
		short gyro_roll_bias;
		short gyro_pitch_plus;
		short gyro_pitch_minus;
		short gyro_yaw_plus;
		short gyro_yaw_minus;
		short gyro_roll_plus;
		short gyro_roll_minus;
		short gyro_speed_plus;
		short gyro_speed_minus;
		short acc_x_plus;
		short acc_x_minus;
		short acc_y_plus;
		short acc_y_minus;
		short acc_z_plus;
		short acc_z_minus;
	} CalibrationReport;

	typedef struct _CalibrationData {
		short bias;
		int sens_numer;
		int sens_denom;
	} CalibrationData;

}