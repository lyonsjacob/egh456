/*
 * MotorControl.h
 *
 *  Created on: 16 May 2019
 *      Author: root
 */



#ifndef MOTORCONTROL_H_
#define MOTORCONTROL_H_

/*Sets motors RPM*/
void setRPM(uint32_t RPM);

/*Gets the current motor RPM*/
uint32_t getRPM(void);

/*Puts motor controller in emergency stop state*/
void emergencyStop(void);


void HALL_A_HWI(unsigned int index);

void HALL_B_HWI(unsigned int index);

void HALL_C_HWI(unsigned int index);

void clk0Fxn(UArg arg0);

void MotorControlSwi(UArg arg0, UArg arg1);

void SetupMotorClock(void);

void initializeMotorStructValues(void);

/* sets up hall affect sensors, PWM and initializes motor driving hardware*/
void MotorSetup(void);

#endif /* MOTORCONTROL_H_ */
