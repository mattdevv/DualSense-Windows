/*
	DS5Specs.h is part of DualSenseWindows
	https://github.com/Ohjurot/DualSense-Windows

	Contributors of this file:
	11.2021 Matthew Hall

	Licensed under the MIT License (To be found in repository root directory)
*/
#pragma once

#define IO_TIMEOUT_MILLISECONDS 100

#define SONY_CORP_VENDOR_ID						0x054C
#define DUALSENSE_CONTROLLER_PROD_ID			0x0CE6

#define DS_INPUT_REPORT_USB						0x01
#define DS_INPUT_REPORT_USB_SIZE				64
#define DS_INPUT_REPORT_BT						0x31
#define DS_INPUT_REPORT_BT_SIZE					78

#define DS_OUTPUT_REPORT_USB					0x02
#define DS_OUTPUT_REPORT_USB_SIZE				63
#define DS_OUTPUT_REPORT_BT						0x31
#define DS_OUTPUT_REPORT_BT_SIZE				78

#define DS_FEATURE_REPORT_CALIBRATION			0x05
#define DS_FEATURE_REPORT_CALIBRATION_SIZE		41
#define DS_FEATURE_REPORT_PAIRING_INFO			0x09
#define DS_FEATURE_REPORT_PAIRING_INFO_SIZE		20
#define DS_FEATURE_REPORT_FIRMWARE_INFO			0x20
#define DS_FEATURE_REPORT_FIRMWARE_INFO_SIZE	64

#define DS_MAX_INPUT_REPORT_SIZE				78 /* DS_INPUT_REPORT_BT_SIZE = 78 */
#define DS_MAX_OUTPUT_REPORT_SIZE				78 /* DS_OUTPUT_REPORT_BT_SIZE = 78 */

#define DS_ACC_RES_PER_G						8192
#define DS_ACC_RANGE							(4*DS_ACC_RES_PER_G)
#define DS_GYRO_RES_PER_DEG_S					1024
#define DS_GYRO_RANGE							(2048*DS_GYRO_RES_PER_DEG_S)
#define DS_TOUCHPAD_WIDTH						1920
#define DS_TOUCHPAD_HEIGHT						1080

/*	// body of input report
	// starts at byte 1 (USB), byte 2 (BT)

	0x00 uint8_t left_stick_x
	0x01 uint8_t left_stick_y
	0x02 uint8_t right_stick_x
	0x03 uint8_t right_stick_y
	0x04 uint8_t left_trigger
	0x05 uint8_t right_trigger

	0x06 uint8_t seq_number; // unknown use

	0x07 uint8_t buttons[4];
	0x0B uint8_t reserved[4];

	0x0F uint16_t gyro[3]; // needs calibration
	0x15 uint16_t accel[3]; // needs calibration
	0x1B uint32_t sensor_timestamp; // in units of 1/3 microseconds
	0x1F uint8_t reserved2;

	0x20 struct touch_point points[2]; // 4 bytes each

	0x28 uint8_t reserved3[12];
	0x34 uint8_t status;
	0x35 uint8_t reserved4[10];

*/