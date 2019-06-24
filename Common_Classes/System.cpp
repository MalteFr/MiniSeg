/*
 * System.cpp
 *
 *    Author: Max Zuidberg
 *     Email: m.zuidberg@icloud.com
 */

#include <System.h>


System::System()
{
    /*
     * Default empty constructor
     */
}

System::~System()
{
    /*
     * Default empty destructor
     */
}

void System::init(uint32_t clk)
{
    /*
     * Initialize the uC by setting and enabling the CPU clock, the PWM Unit
     * Clock Divisor, unlocking all available GPIO pins and allowing the
     * processor to respond to interrupts.
     * CPU clock frequency and PWM Unit Clock Divisor can be read with the
     * corresponding methods.
     * Note: Call this method before doing anything else in your program!
     *
     * clk: the desired clock frequency of the CPU in Hz. It can be 40MHz,
     *      50MHz or 80MHz.
     */

    // Configure clock (Tiva Workshop page 75)
    if (clk == 40000000)
    {
        SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ
                       | SYSCTL_OSC_MAIN);
    }
    else if (clk == 50000000)
    {
        SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ
                       | SYSCTL_OSC_MAIN);
    }
    else if (clk == 80000000)
    {
        SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ
                       | SYSCTL_OSC_MAIN);
    }
    else
    {
        error(SysWrongFrequency, &clk);
    }

    // Store the CPU clock
    systemClockFrequency = clk;

    /*
     * Set the clock divisor which applies to all PWM modules (TivaWare Guide
     * page 509 and datasheet page 1234). Fixed to 16, it allows PWM
     * frequencies of 38Hz-25kHz (with a CPU clock of 40MHz and a minimum
     * resolution of 1%).
     */
    SysCtlPWMClockSet(SYSCTL_PWMDIV_16);
    systemPWMClockDiv = 16;

    /*
     * Use the USB UART to send debugging data.
     */

     // Enable peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    // Wait until peripherals are enabled (TivaWare Guide page 502)
    SysCtlDelay(3);

    // Configure UART and use it with UARTStdio
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 115200, getClockFreq());

    /*
     *  Unlock the two Pins PF0 and PD7. Because those can be used for an NMI
     *  they cannot be used for any other purpose without unlocking them first.
     *  As no use of NMIs is intended, they are unlocked here and can be used
     *  afterwards by any other class.
     */

    // Peripherals must be enabled before accessing their registers
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    // Wait until peripheral is enabled (TivaWare Guide page 502)
    delayCycles(3);

    // Unlock both pins (Tiva Workshop page 70, datasheet page 656)
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR)  |= GPIO_PIN_0;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR)  |= GPIO_PIN_7;
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = 0;

    // Enable interrupts
    IntMasterEnable();
}

void System::error(ErrorCodes errorCode, void *faultOrigin0, void *faultOrigin1, void *faultOrigin2)
{
    /*
     * In case of an error other classes call this method and provide optional
     * debugging informations. It disables interrupts, stops all the
     * peripherals of the uC and enters an infinite loop.
     *
     * errorCode:   optional error parameter giving informations about the
     *              origin of the fault. Default is UnknownError
     * faultOrigin: optional pointers to the variable/object/function that
     *              caused the error.
     */

	// Disable Interrupts
    IntMasterDisable();

    // Stop all peripherals
    uint_fast8_t i;
    for (i = 0; i < systemPeripheralsCount; i++)
    {
        SysCtlPeripheralReset(systemPeripherals[i]);
        SysCtlPeripheralDisable(systemPeripherals[i]);
    }

    while (42);
}

uint32_t System::getClockFreq()
{
    /*
     * Returns the clock frequency of the CPU.
     */


    /*
     * Note: you could also use SysCtlClockGet(); but if you have a look at it,
     *       you'll see it's rather complicated. Therefore storing the
     *       frequency in a variable should be faster.
     */
    return systemClockFrequency;
}

void System::enableFPU()
{
    /*
     * Enable the Floating Point Unit. Any calculations involving "float" will
     * be executed on the FPU which is faster than the CPU.
     */

    // Tiva Workshop page 189 and 193-194
    FPULazyStackingEnable();
    FPUEnable();
}

uint32_t System::getPWMClockDiv()
{
    /*
     * Return the value of the PWM Unit Clock Divisor. It is a power of 2
     * ranging from 1 to 64.
     */

    return systemPWMClockDiv;
}

void System::delayCycles(uint32_t cycles)
{
    /*
     * Halt the CPU for at least the given amount of cycles.
     * Note: Because this function uses SysCtlDelay (see TivaWare guide page
     *       489) only multiples of 3 clock cycles are possible. Any other
     *       value will be rounded up to the next multiple.
     *
     * cycles: Minimum number of clock cycles to halt the CPU
     */

    // To round up in an integer division of p by q, you have to increase
    // p by q-1
    cycles = (cycles + 2) / 3;
    SysCtlDelay(cycles);
}

void System::delayUS(uint32_t us)
{
    /*
     * Halt the CPU for at least the given amount of microseconds.
     * Note: Because this function uses SysCtlDelay (see TivaWare guide page
     *       489) only multiples of 3 clock cycles are possible. Any other
     *       value will be rounded up to the next multiple.
     *
     * us: Minimum delay in microseconds
     */

    // Convert to clock cycles
    us *= getClockFreq() / 1000000;

    delayCycles(us);
}

void System::setDebugging(bool debug)
{
    /*
     * Enable or disable the transmission of debugging data via UART.
     * By default debugging is enabled.
     */
    systemDebuggingEnabled = debug;
}

void System::setDebugFloats(int32_t val1, int32_t val2, int32_t val3, int32_t val4, int32_t val5, int32_t val6)
{
    /*
     * Set up to 6 values to monitor via USB UART (Serial Monitor or Serial
     * Plotter). Each value can be set without modifying the other values.
     * Unused values will transmit as 0.
     * Set the values you do not want to update to SKIP_DBG_VAL. In that case
     * the last set value is preserved.
     * Example:
     *  You have to function that use debugging. Function foo watches 1 value,
     *  function bar watches 2 values. Of course you do not want to overwrite
     *  the values from foo inside bar and vice-versa.
     *  Therefore you write inside foo:
     *      fooSys.setDebugFloats(newValue1);
     *  And inside bar:
     *      barSys.setDebugFloats(SKIP_DBG_VAL, newValue2, newValue3);
     * Because the default value of the 6 possible debug values is already
     * SKIP_DBG_VAL you don't need to provide any additional values. Therefore
     * you do not need the parameters 2 to 6 inside foo, respectively the
     * parameters 4 to 6 inside bar.
     *
     * val: Up to 6 signed integers that you want to monitor. Default value is
     *      SKIP_DBG_VAL.
     */

    if (val1 != SKIP_DBG_VAL)
    {
    	systemDebugVal1 = val1;
    }
    if (val2 != SKIP_DBG_VAL)
    {
		systemDebugVal2 = val2;
	}
    if (val3 != SKIP_DBG_VAL)
    {
		systemDebugVal3 = val3;
	}
    if (val4 != SKIP_DBG_VAL)
    {
		systemDebugVal4 = val4;
	}
    if (val5 != SKIP_DBG_VAL)
    {
		systemDebugVal5 = val5;
	}
    if (val6 != SKIP_DBG_VAL)
    {
		systemDebugVal6 = val6;
	}
}

void System::sendDebugFloats()
{
	/*
	 * If debugging is enabled, send data to PC via UART. Unused values default
	 * to 0.
	 * This method should be called periodically (f.ex. 100Hz Timer interrupt)
	 */

	if (systemDebuggingEnabled)
	{
		UARTprintf("%3d\t%3d\t%3d\t%3d\t%3d\t%3d\n", systemDebugVal1, systemDebugVal2, systemDebugVal3, systemDebugVal4, systemDebugVal5, systemDebugVal6);
	}
}
