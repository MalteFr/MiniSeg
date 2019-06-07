/*
 * Controller.cpp
 *
 *    Author: Henri Hornburg
 *     Email: henri.hornburg@student.kit.edu
 * Co-author: Max Zuidberg
 *     Email: m.zuidberg@icloud.com
 *
 * The controller class contains the algorithm which determines the motor
 * drive speeds based on an angle rate, an angle value and a steering position.
 */

#include "Controller.h"

Controller::Controller()
{
    /*
     * Default empty constructor
     */
}

Controller::~Controller()
{
    /*
     * Default empty destructor
     */
}

void Controller::init(System *sys, float maxSpeed)
{
    /*
     * Initialize the controller by configuring the working and behavior variables.
     *
     * sys: Pointer to the current System instance.
     */

    // Create local reference to the given System object.
    ctlrSys = sys;

    // We use floats, therefore we want to profit from the FPU.
    // Check if it's already enabled and if not, enable it.
    ctlrSys->enableFPU();

    ctlrMaxSpeed = maxSpeed;

    // Initialize speed values
    resetSpeeds();
}

void Controller::resetSpeeds()
{
    /*
     * Reset all speed values to 0
     */

    ctlrDriveSpeed = 0;
    ctlrLeftSpeed = 0;
    ctlrRightSpeed = 0;
}

void Controller::updateValuesRad(float steeringValue, float angleRateRad, float accelHor, float accelVer)
{
    /*
     * Controller based on the P(I)D controller by http://www.tlb.org/#scooter
     * Careful! he uses radians.
     */

    // Get angle from accelerometer and gyrometer. Factor by experiments.
    // Based on http://www.ups.bplaced.de/Dokumentation/Runner%207.38.pdf
    float angleAccelRad = atan2f(-accelHor, -accelVer);
    ctlrAngleRad = compFilter(integrate(ctlrAngleRad, angleRateRad),
                              angleAccelRad, CFG_CTLR_FILTER_FACT);

    // A low pass filter to prevent higher frequency oscillations (forward -
    // backward). Factor by experiments.
    ctlrAngleRate = compFilter(angleRateRad, ctlrAngleRate, CFG_CTLR_LOW_PASS_FACT);

    /*
     * Calculate torque needed for balance.
     * Note: the reduction of the factor 0.4 to 0.2 is needed to prevent
     * higher frequency oscillations (forward - backward). Factor by
     * experiments.
     */
    ctlrTorque = 5.0 * (ctlrAngleRad - ctlrAngleStableRad) + 0.2 * ctlrAngleRate;

    // Speed limiter
    float overspeed = ctlrDriveSpeed - ctlrMaxSpeed;
    if (overspeed > 0)
    {
        // too fast
        overspeed = fminf(0.2, overspeed + 0.05);
        ctlrOverspeedInt = fminf(0.4, integrate(ctlrOverspeedInt, overspeed));
    }
    else
    {
        overspeed = 0;

        // stop speed limiter
        if (ctlrOverspeedInt > 0)
        {
            ctlrOverspeedInt -= 0.04 / CFG_CTLR_UPDATE_FREQ;
        }
    }

    // New stable position
    ctlrAngleStableRad = 0.4 * overspeed + 0.7 * ctlrOverspeedInt;

    // Reduce steering when driving faster
    float steeringAdjusted = 0.07 / (0.3 + fabsf(ctlrDriveSpeed)) * steeringValue;

    // Update current drive speed
    ctlrDriveSpeed = integrate(ctlrDriveSpeed, 1.2 * ctlrTorque);

    // Apply steering
    ctlrLeftSpeed = ctlrTorque + ctlrDriveSpeed + steeringAdjusted;
    ctlrRightSpeed = ctlrTorque + ctlrDriveSpeed - steeringAdjusted;

    ctlrSys->sendDebugFloats(ctlrAngleRad*1800/3.14159, steeringValue*100,
                             ctlrLeftSpeed*100, ctlrRightSpeed*100);
}

float Controller::getLeftSpeed()
{
    /*
     * Returns the duty cycle for the left motor as float from -1. to 1.
     */

    // The controller itself has no real limitation. Therefore it is done here.
    if (ctlrLeftSpeed > 1.0)
    {
        return 1.0;
    }
    else if (ctlrLeftSpeed < -1.0)
    {
        return -1.0;
    }

    return ctlrLeftSpeed;
}
float Controller::getRightSpeed()
{
    /*
     * Returns the duty cycle for the right motor as float from -1. to 1.
     */

    // The controller itself has no real limitation. Therefore it is done here.
    if (ctlrRightSpeed > 1.0)
    {
        return 1.0;
    }
    else if (ctlrRightSpeed < -1.0)
    {
        return -1.0;
    }

    return ctlrRightSpeed;
}

float Controller::getMaxSpeed()
{
	return ctlrMaxSpeed;
}

void Controller::setMaxSpeed(float speed)
{
	ctlrMaxSpeed = speed;
}

float Controller::integrate(float last, float current)
{
    /*
     * Integrates numerically.
     * Example: position = integrate(position, velocity)
     *
     * last:       Result of the last iteration
     * current:    value to integrate
     */

    return (last + current / CFG_CTLR_UPDATE_FREQ);
}

float Controller::arcTanDeg(float a, float b)
{
    /*
     * Returns arctan(a/b) in degree.
     */
    return (atan2f(a, b) * 180 / 3.14159265358979);
}

float Controller::compFilter(float a, float b, float filterFactor)
{
    /*
     * Applies a complementary filter.
     */

    return (filterFactor * a + (1 - filterFactor) * b);
}
