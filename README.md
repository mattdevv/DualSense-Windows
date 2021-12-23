![](https://raw.githubusercontent.com/mattdevv/DualSense-Windows/main/Doc/GitHub_readme/header.png)



Windows API for the PS5 DualSense controller written in C++ for C++ and C#. 

## Features

- Reading all button input from the controller
- Reading the analog sticks and analog triggers
- Reading the two finger touchpad positions
- Reading the accelerometer and gyroscope with device calibration
- Reading the battery level
- Controlling the left and right haptic feedback with variable strength
- Controlling the adaptive triggers and reading back the users force while active
- Controlling the RGB color of the lightbar
- Setting the player indication LEDs and the microphone LED

## Using the API

This is the minimal example on how to use the library:

```c++
#include <Windows.h>
#include <ds5w.h>
#include <iostream>

int main(int argc, char** argv){
   	// Array of controller infos
	DS5W::DeviceEnumInfo infos[16];
	
	// Number of controllers found
	unsigned int controllersCount = 0;
	
	// Call enumerate function and switch on return value
	switch(DS5W::enumDevices(infos, 16, &controllersCount)){
		case DS5W_OK:
        // The buffer was not big enough. Ignore for now
		case DS5W_E_INSUFFICIENT_BUFFER:
			break;
			
		// Any other error will terminate the application
		default:
			// Insert your error handling
			return -1;
	}
    
    // Check number of controllers
    if(!controllersCount){
		return -1;
	}

	// Context for controller
	DS5W::DeviceContext con;
	
	// Init controller and close application is failed
	if(DS5W_FAILED(DS5W::initDeviceContext(&infos[0], &con))){
		return -1;
	}
    
   	// Main loop
	while(true){
		// Input state
		DS5W::DS5InputState inState;
	
		// Retrieve data
		if (DS5W_SUCCESS(DS5W::getDeviceInputState(&con, &inState))){
			// Check for the Logo button
			if(inState.buttonsB & DS5W_ISTATE_BTN_B_PLAYSTATION_LOGO){
				// Break from while loop
				break;
			}
		
            // Create struct and zero it
			DS5W::DS5OutputState outState;
			ZeroMemory(&outState, sizeof(DS5W::DS5OutputState));

			// Set output data
			outState.leftRumble = inState.leftTrigger;
			outState.rightRumble = inState.rightTrigger;

			// Send output to the controller
			DS5W::setDeviceOutputState(&con, &outState);
		}
	}
	
	// Shutdown context
	DS5W::freeDeviceContext(&con);
    
    // Return zero
   	return 0;
}
``` 

## Special thanks to

I have partially used the following sources to implement the functionality:

- The original repo by Ohjurot
- The GitHub community on this project and https://github.com/Ryochan7/DS4Windows/issues/1545
- https://www.reddit.com/r/gamedev/comments/jumvi5/dualsense_haptics_leds_and_more_hid_output_report/
- https://gist.github.com/dogtopus/894da226d73afb3bdd195df41b3a26aa
- https://gist.github.com/Ryochan7/ef8fabae34c0d8b30e2ab057f3e6e039
- https://gist.github.com/Ryochan7/91a9759deb5dff3096fc5afd50ba19e2
- https://github.com/torvalds/linux/blob/master/drivers/hid/hid-sony.c
- https://elixir.bootlin.com/linux/latest/source/drivers/hid/hid-playstation.c



[Important Informations about Trademarks](https://github.com/mattdevv/DualSense-Windows/blob/main/TRADEMARKS.md)

