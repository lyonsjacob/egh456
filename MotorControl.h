/*
 * MotorControl.h
 *
 *  Created on: 16 May 2019
 *      Author: root
 */



#ifndef MOTORCONTROL_H_
#define MOTORCONTROL_H_

/*Sets motors RPM*/
void setRPM(int32_t RPM);

/*Gets the current motor RPM*/
int32_t getRPM(void);

/*Gets the motors current current Acceleration in radian/second^2 */
int32_t getAcceleration(void);

/*Puts motor controller in emergency stop state*/
void emergencyStop(void);

/* sets up hall affect sensors, PWM and initializes motor driving hardware*/
void MotorSetup(void);

#endif /* MOTORCONTROL_H_ */
