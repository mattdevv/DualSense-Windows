#include "DS5_Input.h"

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
	unsigned char buttonsAndDpad = hidInBuffer[0x07] & 0xF0;
	unsigned char buttonsA = hidInBuffer[0x08];
	unsigned char buttonsB = hidInBuffer[0x09];

	// Dpad
	switch (hidInBuffer[0x07] & 0x0F) {
		
	case 0x0: // Up
		buttonsAndDpad |= DS5W_ISTATE_BTN_DPAD_UP;
		break;
	case 0x2: // Right
		buttonsAndDpad |= DS5W_ISTATE_BTN_DPAD_RIGHT;
		break;
	case 0x4: // Down
		buttonsAndDpad |= DS5W_ISTATE_BTN_DPAD_DOWN;
		break;
	case 0x6: // Left
		buttonsAndDpad |= DS5W_ISTATE_BTN_DPAD_LEFT;
		break;
	case 0x1: // Right Up
		buttonsAndDpad |= DS5W_ISTATE_BTN_DPAD_RIGHT | DS5W_ISTATE_BTN_DPAD_UP;
		break;
	case 0x3: // Right Down
		buttonsAndDpad |= DS5W_ISTATE_BTN_DPAD_RIGHT | DS5W_ISTATE_BTN_DPAD_DOWN;
		break;
	case 0x5: // Left Down
		buttonsAndDpad |= DS5W_ISTATE_BTN_DPAD_LEFT | DS5W_ISTATE_BTN_DPAD_DOWN;
		break;
	case 0x7: // Left Up
		buttonsAndDpad |= DS5W_ISTATE_BTN_DPAD_LEFT | DS5W_ISTATE_BTN_DPAD_UP;
		break;
	}

	ptrInputState->buttonMap = (buttonsB << 16) | (buttonsA << 8) | buttonsAndDpad;

	// pointer to raw data in Hid report
	const short* raw_accelerometer = (short*)&hidInBuffer[0x15];
	const short* raw_gyroscope = (short*)&hidInBuffer[0x0F];

	// parse + calibrate accelerometer
	ptrInputState->accelerometer.x = ptrContext->_internal.calibrationData.accelerometer[0].calibrate(raw_accelerometer[0]);
	ptrInputState->accelerometer.y = ptrContext->_internal.calibrationData.accelerometer[1].calibrate(raw_accelerometer[1]);
	ptrInputState->accelerometer.z = ptrContext->_internal.calibrationData.accelerometer[2].calibrate(raw_accelerometer[2]);

	// parse + calibrate gyroscope
	ptrInputState->gyroscope.x = ptrContext->_internal.calibrationData.gyroscope[0].calibrate(raw_gyroscope[0]);
	ptrInputState->gyroscope.y = ptrContext->_internal.calibrationData.gyroscope[1].calibrate(raw_gyroscope[1]);
	ptrInputState->gyroscope.z = ptrContext->_internal.calibrationData.gyroscope[2].calibrate(raw_gyroscope[2]);

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

	unsigned int currentTime = *(unsigned int*)&hidInBuffer[0x1B];
	unsigned int previousTime = ptrContext->_internal.timestamp;

	// absolute difference between current and last timestamp
	unsigned int deltaTime; 
	if (previousTime > currentTime)
		deltaTime = (0xFFFFFFFF - previousTime) + currentTime;
	else
		deltaTime = currentTime - previousTime;

	ptrInputState->currentTime = currentTime;
	ptrInputState->deltaTime = deltaTime;

	ptrContext->_internal.timestamp = currentTime;

	// Battery
	ptrInputState->battery.charging = (hidInBuffer[0x35] & 0x08) != 0;
	ptrInputState->battery.fullyCharged = (hidInBuffer[0x34] & 0x20) != 0;
	ptrInputState->battery.level = ((hidInBuffer[0x34] & 0x0F) * 100) / 8;
}

void __DS5W::Input::parseCalibrationData(DS5W::DeviceCalibrationData* ptrCalibrationData, short* data)
{
	const short gyro_pitch_bias = data[0];
	const short gyro_yaw_bias = data[1];
	const short gyro_roll_bias = data[2];
	const short gyro_pitch_plus = data[3];
	const short gyro_pitch_minus = data[4];
	const short gyro_yaw_plus = data[5];
	const short gyro_yaw_minus = data[6];
	const short gyro_roll_plus = data[7];
	const short gyro_roll_minus = data[8];
	const short gyro_speed_plus = data[9];
	const short gyro_speed_minus = data[10];
	const short acc_x_plus = data[11];
	const short acc_x_minus = data[12];
	const short acc_y_plus = data[13];
	const short acc_y_minus = data[14];
	const short acc_z_plus = data[15];
	const short acc_z_minus = data[16];

	int speed_2x;
	int range_2g;

	/*
	 * Set gyroscope calibration and normalization parameters.
	 * Data values will be normalized to 1/DS_GYRO_RES_PER_DEG_S degree/s.
	 */
	speed_2x = (gyro_speed_plus + gyro_speed_minus);
	ptrCalibrationData->gyroscope[0].bias = gyro_pitch_bias;
	ptrCalibrationData->gyroscope[0].sens_numer = speed_2x * DS_GYRO_RES_PER_DEG_S;
	ptrCalibrationData->gyroscope[0].sens_denom = gyro_pitch_plus - gyro_pitch_minus;

	ptrCalibrationData->gyroscope[1].bias = gyro_yaw_bias;
	ptrCalibrationData->gyroscope[1].sens_numer = speed_2x * DS_GYRO_RES_PER_DEG_S;
	ptrCalibrationData->gyroscope[1].sens_denom = gyro_yaw_plus - gyro_yaw_minus;

	ptrCalibrationData->gyroscope[2].bias = gyro_roll_bias;
	ptrCalibrationData->gyroscope[2].sens_numer = speed_2x * DS_GYRO_RES_PER_DEG_S;
	ptrCalibrationData->gyroscope[2].sens_denom = gyro_roll_plus - gyro_roll_minus;

	/*
	 * Set accelerometer calibration and normalization parameters.
	 * Data values will be normalized to 1/DS_ACC_RES_PER_G g.
	 */
	range_2g = acc_x_plus - acc_x_minus;
	ptrCalibrationData->accelerometer[0].bias = acc_x_plus - range_2g / 2;
	ptrCalibrationData->accelerometer[0].sens_numer = 2 * DS_ACC_RES_PER_G;
	ptrCalibrationData->accelerometer[0].sens_denom = range_2g;

	range_2g = acc_y_plus - acc_y_minus;
	ptrCalibrationData->accelerometer[1].bias = acc_y_plus - range_2g / 2;
	ptrCalibrationData->accelerometer[1].sens_numer = 2 * DS_ACC_RES_PER_G;
	ptrCalibrationData->accelerometer[1].sens_denom = range_2g;

	range_2g = acc_z_plus - acc_z_minus;
	ptrCalibrationData->accelerometer[2].bias = acc_z_plus - range_2g / 2;
	ptrCalibrationData->accelerometer[2].sens_numer = 2 * DS_ACC_RES_PER_G;
	ptrCalibrationData->accelerometer[2].sens_denom = range_2g;
}
