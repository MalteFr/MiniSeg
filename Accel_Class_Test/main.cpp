/*
 * main.cpp
 *
 *    Author: Max Zuidberg
 *     Email: m.zuidberg@icloud.com
 *
 *
 * Test program for the MPU6050. It simply reads the sensor values. These can
 * be visualized in the CCS debug view.
 */



/*
 * stdbool.h:           Boolean definitions for the C99 standard
 * stdint.h:            Variable definitions for the C99 standard
 * System.h:            Header file for the System class
 *                      (needed for example for clock settings)
 * GPIO.h:              Header file for the GPIO class
 * MPU6050.h:           Header file for the MPU6050 class
 */
#include <stdint.h>

#include <stdbool.h>
#include "System.h"
#include "GPIO.h"
#include "MPU6050.h"


int main(void)

{
    /*
     * See https://www.youtube.com/watch?v=-iGnh0_9YxE to find out how to
     * check these values (line 78-83).
     * Remember to check that the graphs are configured to float! Rearrange
     * the graphs as you like. You need to do this only once per debug session.
     * Use the button SW1 to start and stop sampling. During a stop the sensor
     * can be replaced by another one.
     */

    // Define all variables and objects
    System system;
    GPIO sensorEnable, sensorSupply;
    MPU6050 sensor;

    // Start system and run at 40 MHz.
    system.init(40000000);

    float angle, angleRate, hor, ver;

    // Use the on-board switch SW1 connected to PF4 to enable or disable the
    // sensor. We need a pull-up, as the switch is connected to GND.
    sensorEnable.init(&system, GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_DIR_MODE_IN, true);

    // Use PF2 to supply the sensor.
	sensorSupply.init(&system, GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_DIR_MODE_OUT);

	// Configure as supply pin (max. current) and make sure it's of.
    sensorSupply.write(0);
	sensorSupply.setCurrent(12);

	while (42)
	{
        // wait for user to (re-)enable sampling
        while (sensorEnable.read());

        // Power up sensor
        sensorSupply.write(1);

        // Delay for debounce and sensor power up
        system.delayUS(50000);

        // Wait for button release
        while (!sensorEnable.read());

        // Initialize possibly new sensor.
        sensor.init(&system, I2C1_BASE, 0, 'y', 'z');

	    // If the button SW1 (PF4) is pressed, stop sampling.
	    while (sensorEnable.read())
	    {
	        angleRate = sensor.getAngleRate();
	        angle += angleRate/10;
	        hor = sensor.getAccelHor();
	        ver = sensor.getAccelVer();
	        system.sendDebugFloats(angleRate, 10*hor, 10*ver);
	        system.delayUS(10000);
	    }

	    // Power down sensor
	    sensorSupply.write(0);

	    // Delay for debounce
        system.delayUS(50000);

        // Wait for button release
        while (!sensorEnable.read());
	}
}
