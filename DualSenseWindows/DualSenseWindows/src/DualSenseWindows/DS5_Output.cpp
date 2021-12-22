#include "DS5_Output.h"

void __DS5W::Output::createHidOutputBuffer(unsigned char* hidOutBuffer, DS5W::DS5OutputState* ptrOutputState) {
	// Feature mask

	// 0x01 Set the main motors (also requires flag 0x02)
	// 0x02 Set the main motors (also requires flag 0x01)
	// 0x04 Set the right trigger motor
	// 0x08 Set the left trigger motor
	// 0x10 Enable modification of audio volume
	// 0x20 Enable internal speaker (even while headset is connected)
	// 0x40 Enable modification of microphone volume
	// 0x80 Enable internal mic (even while headset is connected)
	const unsigned char FEATURES1 = 0b00001111;

	// 0x01 Toggling microphone LED
	// 0x02 Toggling Audio/Mic Mute
	// 0x04 Toggling LED strips on the sides of the Touchpad
	// 0x08 Turn off all LED lights
	// 0x10 Toggle player LED lights below Touchpad
	// 0x20 ???
	// 0x40 Adjust overall motor/effect power
	// 0x80 ???
	const unsigned char FEATURES2 = 0b11110111;

	hidOutBuffer[0x00] = FEATURES1;
	hidOutBuffer[0x01] = FEATURES2;

	// Rumble motors
	hidOutBuffer[0x02] = ptrOutputState->rightRumble;
	hidOutBuffer[0x03] = ptrOutputState->leftRumble;

	// Mic led
	hidOutBuffer[0x08] = (unsigned char)ptrOutputState->microphoneLed;

	hidOutBuffer[0x24] = ptrOutputState->rumbleStrength;

	// Player led brightness
	hidOutBuffer[0x26] = 0x03;
	hidOutBuffer[0x29] = ptrOutputState->disableLeds ? 0x01 : 0x2;
	hidOutBuffer[0x2A] = ptrOutputState->playerLeds.brightness;

	// Player led mask
	hidOutBuffer[0x2B] = ptrOutputState->playerLeds.bitmask;
	if (ptrOutputState->playerLeds.playerLedFade) {
		hidOutBuffer[0x2B] &= ~(0x20);
	}
	else {
		hidOutBuffer[0x2B] |= 0x20;
	}

	// Lightbar
	hidOutBuffer[0x2C] = ptrOutputState->lightbar.r;
	hidOutBuffer[0x2D] = ptrOutputState->lightbar.g;
	hidOutBuffer[0x2E] = ptrOutputState->lightbar.b;

	// Adaptive Triggers
	memcpy(&hidOutBuffer[0x0A], &ptrOutputState->rightTriggerEffect, 11);
	memcpy(&hidOutBuffer[0x15], &ptrOutputState->leftTriggerEffect, 11);
	

	//processTrigger(&ptrOutputState->leftTriggerEffect, &hidOutBuffer[0x15]);
	//processTrigger(&ptrOutputState->rightTriggerEffect, &hidOutBuffer[0x0A]);
}

void __DS5W::Output::processTrigger(DS5W::TriggerEffect* ptrEffect, unsigned char* buffer) {
	// Switch on effect
	switch (ptrEffect->effectType) {
		// Continious
		case DS5W::TriggerEffectType::ContinuousResitance:
			// Mode
			buffer[0x00] = 0x01;
			// Parameters
			buffer[0x01] = ptrEffect->Continuous.startPosition;
			buffer[0x02] = ptrEffect->Continuous.force;

			break;

		// Section
		case DS5W::TriggerEffectType::SectionResitance:
			// Mode
			buffer[0x00] = 0x02;
			// Parameters
			buffer[0x01] = ptrEffect->Continuous.startPosition;
			buffer[0x02] = ptrEffect->Continuous.force;

			break;

		// EffectEx
		case DS5W::TriggerEffectType::EffectEx:
			// Mode
			buffer[0x00] = 0x02 | 0x20 | 0x04;
			// Parameters
			buffer[0x01] = 0xFF - ptrEffect->EffectEx.startPosition;
			// Keep flag
			if (ptrEffect->EffectEx.keepEffect) {
				buffer[0x02] = 0x02;
			}
			// Forces
			buffer[0x04] = ptrEffect->EffectEx.beginForce;
			buffer[0x05] = ptrEffect->EffectEx.middleForce;
			buffer[0x06] = ptrEffect->EffectEx.endForce;
			// Frequency
			buffer[0x09] = max(1, ptrEffect->EffectEx.frequency / 2);

			break;

		// Calibrate
		case DS5W::TriggerEffectType::Calibrate:
			// Mode 
			buffer[0x00] = 0xFC;

			break;

		// No resistance / default
		case DS5W::TriggerEffectType::NoResitance:
			__fallthrough;
		default:
			// All zero
			buffer[0x00] = 0x00;
			buffer[0x01] = 0x00;
			buffer[0x02] = 0x00;

			break;
	}		
}
