/*
 * main.cpp
 *
 *    Author: Henri Hornburg
 *     Email: henri.hornburg@student.kit.edu
 * Co-author: Max Zuidberg
 *     Email: m.zuidberg@icloud.com
 *
 * NOTE: You should not change this file unless you have a very good reason!
 */


/*
 * Config.h: Header file containing all configurable parameters of the segway,
 *           as for example its pinout.
 *           Note: all constants are prefixed by CFG_.
 * System.h: Header file for the System class (needed for example for clock
 *           settings)
 * Segway.h: Header file for the Segway class which contains all the code for
 *           controlling a segway.
 */
#include "Config.h"
#include "System.h"
#include "Segway.h"
#include "Timer.h"


// These objects are used inside the global ISR mainTimerISR, thus must be
// global, too.
Segway segway;
Timer mainTimer;


void mainTimerISR()
{
    /*
     * This ISR is periodically called by the updateTsimer. It causes the
     * segway class to update all inputs and set the corresponding new outputs.
     *
     * Note: it is not possible to set a class method as ISR. Therefore this
     *       "helper-function" is needed.
     */

    mainTimer.clearInterruptFlag();

    // Update segway
    segway.update();
}

int main(void)
{
    System system;

    // Initialize objects according to the values in Config.h
    system.init(CFG_SYS_FREQ);
    mainTimer.init(&system, CFG_MAIN_TIMER_BASE, mainTimerISR, CFG_CTLR_UPDATE_FREQ);

    // Initialize and start segway
    segway.init(&system);
    mainTimer.start();

	while (42)
	{
	    // Some monitoring tasks can run silently in background while the
	    // critical parts run inside the ISR
	    segway.backgroundTasks();
	}
}
