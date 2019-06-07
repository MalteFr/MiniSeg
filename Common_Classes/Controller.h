/*
 * Controller.h
 *
 *    Author: Henri Hornburg
 *     Email: henri.hornburg@student.kit.edu
 * Co-author: Max Zuidberg
 *     Email: m.zuidberg@icloud.com
 *
 * The controller class contains the algorithm which determines the motor drive speeds based on an angle rate, an angle value and a steering position
 */

#ifndef CONTROLLER_H_
#define CONTROLLER_H_

/*
 * stdint.h:                Variable definitions for the C99 standard
 * math.h:   Floating point math functions like fabsf()
 * Config.h:     All configurable parameters of the segway, as for example its pinout. Note: all constants are prefixed by CFG_.
 * System.h: Header file for the System class (needed for error handling)
 */
#include <stdint.h>
#include <math.h>
#include "Config.h"
#include "System.h"



class Controller
{
public:
    Controller();
    ~Controller();
    void init(System *sys, float maxSpeed);
    void resetSpeeds();
    void updateValuesRad(float steeringValue, float angleRate, float accelHor, float accelVer);
    float getLeftSpeed();
    float getRightSpeed();
    float getMaxSpeed();
    void setMaxSpeed(float speed);

private:
    float integrate(float last, float current);
    float arcTanDeg(float a, float b);
    float compFilter(float a, float b, float filterFactor);

    System* ctlrSys;
    uint32_t ctlrFreq = 0;
    float ctlrAngleRad = 0.0;
    float ctlrAngleRate = 0.0;
    float ctlrAngleStableRad = 0.0;
    float ctlrTorque = 0.0;
    float ctlrOverspeedInt = 0.0;
    float ctlrLeftSpeed = 0.0, ctlrRightSpeed = 0.0;
    float ctlrDriveSpeed = 0.0;
    float ctlrMaxSpeed = 1.0;
};


#endif /* CONTROLLER_H_ */
