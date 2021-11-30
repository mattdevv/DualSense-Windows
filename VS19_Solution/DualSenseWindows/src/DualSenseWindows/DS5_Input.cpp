#include "DS5_Input.h"

inline int mult_frac(int x, int numer, int denom)
{
	int quot = (x) / (denom);			
	int rem = (x) % (denom);			
	return (quot * (numer)) + ((rem * (numer)) / (denom));	
}

void __DS5W::Input::evaluateHidInputBuffer(unsigned char* hidInBuffer, DS5W::DS5InputState* ptrInputState, DS5W::DeviceContext* ptrContext) {
	// Convert sticks to signed range
	ptrInputState->leftStick.x = (char)(((short)(hidInBuffer[0x00] - 128)));
	ptrInputState->leftStick.y = (char)(((short)(hidInBuffer[0x01] - 127)) * -1);
	ptrInputState->rightStick.x = (char)(((short)(hidInBuffer[0x02] - 128)));
	ptrInputState->rightStick.y = (char)(((short)(hidInBuffer[0x03] - 127)) * -1);

	// Convert trigger to unsigned range
	ptrInputState->leftTrigger = hidInBuffer[0x04];
	ptrInputState->rightTrigger = hidInBuffer[0x05];

	// Buttons
	ptrInputState->buttonsAndDpad = hidInBuffer[0x07] & 0xF0;
	ptrInputState->buttonsA = hidInBuffer[0x08];
	ptrInputState->buttonsB = hidInBuffer[0x09];

	// Dpad
	switch (hidInBuffer[0x07] & 0x0F) {
		// Up
	case 0x0:
		ptrInputState->buttonsAndDpad |= DS5W_ISTATE_DPAD_UP;
		break;
		// Down
	case 0x4:
		ptrInputState->buttonsAndDpad |= DS5W_ISTATE_DPAD_DOWN;
		break;
		// Left
	case 0x6:
		ptrInputState->buttonsAndDpad |= DS5W_ISTATE_DPAD_LEFT;
		break;
		// Right
	case 0x2:
		ptrInputState->buttonsAndDpad |= DS5W_ISTATE_DPAD_RIGHT;
		break;
		// Left Down
	case 0x5:
		ptrInputState->buttonsAndDpad |= DS5W_ISTATE_DPAD_LEFT | DS5W_ISTATE_DPAD_DOWN;
		break;
		// Left Up
	case 0x7:
		ptrInputState->buttonsAndDpad |= DS5W_ISTATE_DPAD_LEFT | DS5W_ISTATE_DPAD_UP;
		break;
		// Right Up
	case 0x1:
		ptrInputState->buttonsAndDpad |= DS5W_ISTATE_DPAD_RIGHT | DS5W_ISTATE_DPAD_UP;
		break;
		// Right Down
	case 0x3:
		ptrInputState->buttonsAndDpad |= DS5W_ISTATE_DPAD_RIGHT | DS5W_ISTATE_DPAD_DOWN;
		break;
	}
	
	short raw_accelerometer[3];
	short raw_gyroscope[3];
	
	memcpy(&raw_accelerometer, &hidInBuffer[0x0F], 2 * 3);
	memcpy(&raw_gyroscope, &hidInBuffer[0x15], 2 * 3);

	ptrInputState->accelerometer.x = mult_frac(ptrContext->_internal.accel_calib_data[0].sens_numer,
		raw_accelerometer[0] - ptrContext->_internal.accel_calib_data[0].bias,
		ptrContext->_internal.accel_calib_data[0].sens_denom);

	ptrInputState->accelerometer.y = mult_frac(ptrContext->_internal.accel_calib_data[1].sens_numer,
		raw_accelerometer[1] - ptrContext->_internal.accel_calib_data[1].bias,
		ptrContext->_internal.accel_calib_data[1].sens_denom);

	ptrInputState->accelerometer.z = mult_frac(ptrContext->_internal.accel_calib_data[2].sens_numer,
		raw_accelerometer[2] - ptrContext->_internal.accel_calib_data[2].bias,
		ptrContext->_internal.accel_calib_data[2].sens_denom);

	ptrInputState->gyroscope.x = mult_frac(ptrContext->_internal.gyro_calib_data[0].sens_numer,
		raw_gyroscope[0] - ptrContext->_internal.gyro_calib_data[0].bias,
		ptrContext->_internal.gyro_calib_data[0].sens_denom);

	ptrInputState->gyroscope.y = mult_frac(ptrContext->_internal.gyro_calib_data[1].sens_numer,
		raw_gyroscope[1] - ptrContext->_internal.gyro_calib_data[1].bias,
		ptrContext->_internal.gyro_calib_data[1].sens_denom);

	ptrInputState->gyroscope.z = mult_frac(ptrContext->_internal.gyro_calib_data[2].sens_numer,
		raw_gyroscope[2] - ptrContext->_internal.gyro_calib_data[2].bias,
		ptrContext->_internal.gyro_calib_data[2].sens_denom);

	// Evaluate touch state 1
	UINT32 touchpad1Raw = *(UINT32*)(&hidInBuffer[0x20]);
	ptrInputState->touchPoint1.y = (touchpad1Raw & 0xFFF00000) >> 20;
	ptrInputState->touchPoint1.x = (touchpad1Raw & 0x000FFF00) >> 8;
	ptrInputState->touchPoint1.down = (touchpad1Raw & (1 << 7)) == 0;
	ptrInputState->touchPoint1.id = (touchpad1Raw & 127);

	// Evaluate touch state 2
	UINT32 touchpad2Raw = *(UINT32*)(&hidInBuffer[0x24]);
	ptrInputState->touchPoint2.y = (touchpad2Raw & 0xFFF00000) >> 20;
	ptrInputState->touchPoint2.x = (touchpad2Raw & 0x000FFF00) >> 8;
	ptrInputState->touchPoint2.down = (touchpad2Raw & (1 << 7)) == 0;
	ptrInputState->touchPoint2.id = (touchpad2Raw & 127);

	// Evaluate headphone input
	ptrInputState->headPhoneConnected = hidInBuffer[0x35] & 0x01;

	// Trigger force feedback
	ptrInputState->leftTriggerFeedback = hidInBuffer[0x2A];
	ptrInputState->rightTriggerFeedback = hidInBuffer[0x29];

	// Battery
	ptrInputState->battery.chargin = (hidInBuffer[0x35] & 0x08);
	ptrInputState->battery.fullyCharged = (hidInBuffer[0x36] & 0x20);
	ptrInputState->battery.level = (hidInBuffer[0x36] & 0x0F);
}

void __DS5W::Input::parseCalibrationData(DS5W::DeviceContext* device, short* data)
{
	short gyro_pitch_bias = data[0];
	short gyro_yaw_bias = data[1];
	short gyro_roll_bias = data[2];
	short gyro_pitch_plus = data[3];
	short gyro_pitch_minus = data[4];
	short gyro_yaw_plus = data[5];
	short gyro_yaw_minus = data[6];
	short gyro_roll_plus = data[7];
	short gyro_roll_minus = data[8];
	short gyro_speed_plus = data[9];
	short gyro_speed_minus = data[10];
	short acc_x_plus = data[11];
	short acc_x_minus = data[12];
	short acc_y_plus = data[13];
	short acc_y_minus = data[14];
	short acc_z_plus = data[15];
	short acc_z_minus = data[16];

	int speed_2x;
	int range_2g;

	/*
	 * Set gyroscope calibration and normalization parameters.
	 * Data values will be normalized to 1/DS_GYRO_RES_PER_DEG_S degree/s.
	 */
	speed_2x = (gyro_speed_plus + gyro_speed_minus);
	device->_internal.gyro_calib_data[0].bias = gyro_pitch_bias;
	device->_internal.gyro_calib_data[0].sens_numer = speed_2x * DS_GYRO_RES_PER_DEG_S;
	device->_internal.gyro_calib_data[0].sens_denom = gyro_pitch_plus - gyro_pitch_minus;

	device->_internal.gyro_calib_data[1].bias = gyro_yaw_bias;
	device->_internal.gyro_calib_data[1].sens_numer = speed_2x * DS_GYRO_RES_PER_DEG_S;
	device->_internal.gyro_calib_data[1].sens_denom = gyro_yaw_plus - gyro_yaw_minus;

	device->_internal.gyro_calib_data[2].bias = gyro_roll_bias;
	device->_internal.gyro_calib_data[2].sens_numer = speed_2x * DS_GYRO_RES_PER_DEG_S;
	device->_internal.gyro_calib_data[2].sens_denom = gyro_roll_plus - gyro_roll_minus;

	/*
	 * Set accelerometer calibration and normalization parameters.
	 * Data values will be normalized to 1/DS_ACC_RES_PER_G g.
	 */
	range_2g = acc_x_plus - acc_x_minus;
	device->_internal.accel_calib_data[0].bias = acc_x_plus - range_2g / 2;
	device->_internal.accel_calib_data[0].sens_numer = 2 * DS_ACC_RES_PER_G;
	device->_internal.accel_calib_data[0].sens_denom = range_2g;

	range_2g = acc_y_plus - acc_y_minus;
	device->_internal.accel_calib_data[1].bias = acc_y_plus - range_2g / 2;
	device->_internal.accel_calib_data[1].sens_numer = 2 * DS_ACC_RES_PER_G;
	device->_internal.accel_calib_data[1].sens_denom = range_2g;

	range_2g = acc_z_plus - acc_z_minus;
	device->_internal.accel_calib_data[2].bias = acc_z_plus - range_2g / 2;
	device->_internal.accel_calib_data[2].sens_numer = 2 * DS_ACC_RES_PER_G;
	device->_internal.accel_calib_data[2].sens_denom = range_2g;
}

// for debugging with known values
//void __DS5W::Input::evaluateHidInputBuffer(unsigned char* hidInBuffer, DS5W::DS5InputState* ptrInputState) {
//	// Convert sticks to signed range
//	ptrInputState->leftStick.x = 1;
//	ptrInputState->leftStick.y = 2;
//	ptrInputState->rightStick.x = 3;
//	ptrInputState->rightStick.y = 4;
//
//	// Convert trigger to unsigned range
//	ptrInputState->leftTrigger = 5;
//	ptrInputState->rightTrigger = 6;
//
//	// Buttons
//	ptrInputState->buttonsAndDpad = 7;
//	ptrInputState->buttonsA = 8;
//	ptrInputState->buttonsB = 9;
//
//	ptrInputState->accelerometer.x = 10;
//	ptrInputState->accelerometer.y = 11;
//	ptrInputState->accelerometer.z = 12;
//	ptrInputState->gyroscope.x = 13;
//	ptrInputState->gyroscope.y = 14;
//	ptrInputState->gyroscope.z = 15;
//
//	ptrInputState->touchPoint1.y = 17;
//	ptrInputState->touchPoint1.x = 16;
//	ptrInputState->touchPoint1.down = false;
//	ptrInputState->touchPoint1.id = 18;
//
//	ptrInputState->touchPoint2.y = 20;
//	ptrInputState->touchPoint2.x = 19;
//	ptrInputState->touchPoint2.down = true;
//	ptrInputState->touchPoint2.id = 21;
//
//	// Battery
//	ptrInputState->battery.chargin = true;
//	ptrInputState->battery.fullyCharged = false;
//	ptrInputState->battery.level = 22;
//
//	// Evaluate headphone input
//	ptrInputState->headPhoneConnected = false;
//
//	// Trigger force feedback
//	ptrInputState->leftTriggerFeedback = 23;
//	ptrInputState->rightTriggerFeedback = 24 ;
//}
