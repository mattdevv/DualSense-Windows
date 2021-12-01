/*
	DS5State.h is part of DualSenseWindows
	https://github.com/Ohjurot/DualSense-Windows

	Contributors of this file:
	11.2021 mattdevv

	Licensed under the MIT License (To be found in repository root directory)
*/
#pragma once

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

#define DS_ACC_RES_PER_G						8192
#define DS_ACC_RANGE							(4*DS_ACC_RES_PER_G)
#define DS_GYRO_RES_PER_DEG_S					1024
#define DS_GYRO_RANGE							(2048*DS_GYRO_RES_PER_DEG_S)
#define DS_TOUCHPAD_WIDTH						1920
#define DS_TOUCHPAD_HEIGHT						1080