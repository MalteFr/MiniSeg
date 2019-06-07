/*
 * Segway.cpp
 *
 *    Author: Henri Hornburg
 *     Email: henri.hornburg@student.kit.edu
 * Co-author: Max Zuidberg
 *     Email: m.zuidberg@icloud.com
 *
 * The segway class contains all the code needed drive a segway.
 * Please note that this segway is only imaginary and the code only provides
 * capabilities to teach the students some basic programming skills. All this
 * has to do with a segway is, that a gyroscope will read velocities and
 * translate them into motor torque on motors wired to a breadboard.
 */

#include <Segway.h>

Segway::Segway()
{
    /*
     * Default empty constructor
     */
}

Segway::~Segway()
{
    /*
     * Default empty destructor
     */
}

void Segway::init(System *sys)
{
    /*  Initializes all the objects used in the segway class.
     *  Needs to be called before any components can be used.
     *
     * sys: Pointer to the current System instance. Needed to get CPU clock
     *      frequency.
     */

    // Create private reference to the given System object.
    segwaySystem = sys;

    // Initialize all objects with the given parameters and the parameters from
    // the Config header file.
    segwayLeftMotor.init(segwaySystem, CFG_LM_PORT, CFG_LM_PIN1, CFG_LM_PIN2, CFG_PWM_INVERT, CFG_LM_FREQ);
    segwayRightMotor.init(segwaySystem, CFG_RM_PORT, CFG_RM_PIN1, CFG_RM_PIN2, CFG_PWM_INVERT, CFG_RM_FREQ);
    segwayEnableMotors.init(segwaySystem, CFG_EM_PORT, CFG_EM_PIN, CFG_EM_DIR);
    segwayFootSwitch.init(segwaySystem, CFG_FS_PORT, CFG_FS_PIN, CFG_FS_DIR, CFG_FS_PULLUP);
    segwayController.init(segwaySystem, CFG_CTLR_MAX_SPEED);
    segwayBatteryVoltage.init(segwaySystem, CFG_BATT_BASE, CFG_BATT_SSEQ, CFG_BATT_AIN);
    segwaySensor.init(segwaySystem, CFG_SENSOR_I2C_MODULE, CFG_SENSOR_ADRESSBIT);

    // Configure sensor orientation
    segwaySensor.setWheelAxis(CFG_SENSOR_WHEEL_AXIS);
    segwaySensor.setHorAxis(CFG_SENSOR_HOR_AXIS);
    segwaySensor.accelHorInvertSign(CFG_SENSOR_INVERT_HOR);
    segwaySensor.accelVerInvertSign(CFG_SENSOR_INVERT_VER);
    segwaySensor.angleRateInvertSign(CFG_SENSOR_INVERT_ANGLE_RATE);

    // This Enable Motors Pin is only needed for compatibility with the TivSeg
    // Hardware. It is not used at any other place in the code.
    segwayEnableMotors.write(CFG_EM_ACTIVE_STATE);

    // We use floats, therefore we want to profit from the FPU.
    segwaySystem->enableFPU();

    // Initializing done, segway is ready but not active yet.
    segwayStandby = true;
}

void Segway::update()
{
    /*
     * Read all sensor inputs, hand them to the controller and apply the
     * resulting motor duty cycles.
     */

    // Get state of the foot switch which is active high.
    bool footSwitchPressed = (segwayFootSwitch.read() == CFG_FS_ACTIVE_STATE);

    /*
     * Standby Mode controls whether the segway runs or not. The segway leaves
     * standby when the food switch is pressed and returns to standby if it's
     * released.
     * This is the polling variant, the other variant would be rising and
     * falling edge interrupts, which would be more complicated.
     */
    if (segwayStandby == true)
    {
        if (footSwitchPressed)
        {
            // Someone stepped on the segway, so we can leave standby and
            // start driving.
            segwayStandby = false;
        }
    }
    else
    {
        if (footSwitchPressed)
        {
            // User is still standing on the foot switch and we're not in
            // standby but in driving mode.

            // Variables for debugging purpose only

            float steeringValue;

            // Get current angle rate in rad from the gyro
            float angleRateRad = segwaySensor.getAngleRate() * 3.14159265358979 / 180;

            float accelHor = segwaySensor.getAccelHor();
            float accelVer = segwaySensor.getAccelVer();

            // Feed the new sensor data into the controller
            segwayController.updateValuesRad(steeringValue, angleRateRad, accelHor, accelVer);

            float leftMotorDuty = segwayController.getLeftSpeed();
            float rightMotorDuty = segwayController.getRightSpeed();

            // Apply the new duty cycles to the motors
            segwayLeftMotor.setDuty(leftMotorDuty);
            segwayRightMotor.setDuty(rightMotorDuty);
        }
        else
        {
            // User is not standing on the foot switch anymore (maybe not even
            // on the segway either)

            // Stop the motors and reset all speeds to 0 as the segway is not
            // moving in standby.
            segwayController.resetSpeeds();
            segwayLeftMotor.setDuty(0);
            segwayRightMotor.setDuty(0);

            segwayStandby = true;
        }
    }

    // Successfully passed the update method.
    segwayUpdateFlag = true;
}

void Segway::backgroundTasks()
{
    /*
     * While the Segway::update() method runs the time-critical controller
     * related code, this method can run in background for monitoring tasks.
     * Some tasks need to sync with the ISR/Segway::update method. That's what
     * the segwayUpdateFlag is for.
     */

    if (segwayUpdateFlag)
    {
		segwayUpdateFlag = false;
		
    }
	
}
