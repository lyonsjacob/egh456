/*
 * MotorControl.c
 *
 *  Created on: 16 May 2019
 *      Author: root
 */

#include <stdio.h>
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Swi.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/knl/Clock.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/drivers/PWM.h>

#include <MotorControl.h>

/* Board Header file */
#include "Board.h"
#include <inc/hw_ints.h>


struct motor_control
{
    PWM_Handle  pwm1;
    PWM_Params  params;
    uint8_t     emergencyStop;
    double      pwmPeriod, duty;
    int32_t     currentRPM, lastRPM, requiredRPM;
    int32_t     error, prevError;
    uint32_t    interruptCount;
    uint32_t    MaxAcceleration;
    int32_t     currentAccelerationRadss;
};

struct motor_control Motor_Control;
/* Motor speed control swi and clock prams*/
Swi_Struct motorSwiStruct;
Swi_Handle motorSwiHandle;
Clock_Struct clk0Struct;


/*Sets motors RPM*/
void setRPM(int32_t RPM)
{
    if(!Motor_Control.emergencyStop)
    {
        Motor_Control.requiredRPM  = RPM;
    }
}

/*Gets the current motor RPM*/
int32_t getRPM(void)
{
    return Motor_Control.currentRPM;
}


/*Puts motor controller in emergency stop state*/
void emergencyStop(void)
{
    Motor_Control.MaxAcceleration = 200;
    Motor_Control.requiredRPM     = 0;
    Motor_Control.emergencyStop   = 1;
}




void HALL_A_HWI(unsigned int index)
{
    Motor_Control.interruptCount++;
    //
    // Clear the asserted interrupts.
    //
    GPIO_clearInt(Board_HALL_A);
    GPIO_write(Board_STATE1, GPIO_read(Board_HALL_A));
}



void HALL_B_HWI(unsigned int index)
{
    Motor_Control.interruptCount++;
    //
    // Clear the asserted interrupts.
    //
    GPIO_clearInt(Board_HALL_B);
    GPIO_write(Board_STATE2, GPIO_read(Board_HALL_B));
}



void HALL_C_HWI(unsigned int index)
{
    Motor_Control.interruptCount++;
    //
    // Clear the asserted interrupts.
    //
    GPIO_clearInt(Board_HALL_C);
    GPIO_write(Board_STATE0, GPIO_read(Board_HALL_C));
}

void clk0Fxn(UArg arg0)
{
    Motor_Control.lastRPM       = Motor_Control.currentRPM;
    Motor_Control.prevError     = Motor_Control.error;
    double revolutions          = (double)Motor_Control.interruptCount/24.0;
    Motor_Control.currentRPM    = 600*revolutions;
    Motor_Control.error         = Motor_Control.requiredRPM - Motor_Control.currentRPM;
    Motor_Control.interruptCount= 0;
    Swi_post(motorSwiHandle);
}



void StartMotor(void)
{
    uint8_t hallA = GPIO_read(Board_HALL_A);
    uint8_t hallB = GPIO_read(Board_HALL_B);
    uint8_t hallC = GPIO_read(Board_HALL_C);


    PWM_setDuty(Motor_Control.pwm1, 50);

    if(!hallA && !hallB && hallC)
    {
        GPIO_write(Board_STATE1, Board_OFF);
        GPIO_write(Board_STATE2, Board_ON);
        GPIO_write(Board_STATE0, Board_ON);

    }else if(!hallA && hallB && hallC)
    {
        GPIO_write(Board_STATE1, Board_OFF);
        GPIO_write(Board_STATE2, Board_ON);
        GPIO_write(Board_STATE0, Board_OFF);

    }else if(!hallA && hallB && !hallC)
    {
        GPIO_write(Board_STATE1, Board_ON);
        GPIO_write(Board_STATE2, Board_ON);
        GPIO_write(Board_STATE0, Board_OFF);

    }else if(hallA && hallB && !hallC)
    {
        GPIO_write(Board_STATE1, Board_ON);
        GPIO_write(Board_STATE2, Board_OFF);
        GPIO_write(Board_STATE0, Board_OFF);

    }else if(hallA && !hallB && !hallC)
    {
        GPIO_write(Board_STATE1, Board_ON);
        GPIO_write(Board_STATE2, Board_OFF);
        GPIO_write(Board_STATE0, Board_ON);

    }else if(hallA && !hallB && hallC)
    {
        GPIO_write(Board_STATE1, Board_OFF);
        GPIO_write(Board_STATE2, Board_OFF);
        GPIO_write(Board_STATE0, Board_ON);
    }
    Motor_Control.duty = 4;
}


void MotorControlSwi(UArg arg0, UArg arg1)
{
    Motor_Control.currentAccelerationRadss = ((Motor_Control.lastRPM -Motor_Control.currentRPM)*0.10472)*10;

    /*Kick start motor*/
    if(!Motor_Control.currentRPM && Motor_Control.requiredRPM)
    {
        StartMotor();
        return;
    }


    double integral = (double)Motor_Control.prevError*0.1 + (((double)Motor_Control.prevError-(double)Motor_Control.prevError)*0.1)/2;
    double derivative = ((double)Motor_Control.prevError-(double)Motor_Control.prevError)/0.1;

    /*set duty cycle in emergency stop scenario*/
    if (Motor_Control.emergencyStop)
    {
        Motor_Control.duty = (int)(Motor_Control.duty+Motor_Control.error*0.0053);
        /*Integral control*/
        Motor_Control.duty = (int)(Motor_Control.duty-0.05*integral);
        /*derivative control*/
        Motor_Control.duty = (double)(Motor_Control.duty-0.09*derivative);

    /*set duty cycle in not emergency stop scenario*/
    }else
    {
        Motor_Control.duty = (double)(Motor_Control.duty+Motor_Control.error*+0.0056);
        /*Integral control*/
        Motor_Control.duty = (double)(Motor_Control.duty-0.045*integral);
        /*derivative control*/
        Motor_Control.duty = (double)(Motor_Control.duty-0.09*derivative);


    }

    /*check if motor duty cycle is negative*/
    if(Motor_Control.duty < 0)
    {
        Motor_Control.duty = 0;
    }

    /*check if motor duty cycle is greater than 100%*/
    if(Motor_Control.duty > Motor_Control.pwmPeriod )
    {
        Motor_Control.duty = Motor_Control.pwmPeriod;
    }

    /*reset duty cycle if motor is stationary*/
    if(Motor_Control.duty < 4 && !Motor_Control.requiredRPM)
    {
        Motor_Control.duty = 0;
    }

    /*reset emergency stop if motor is stationary*/
    if((Motor_Control.emergencyStop)&&(!Motor_Control.currentRPM))
    {
        Motor_Control.emergencyStop = 0;
        Motor_Control.MaxAcceleration = 200;
    }

    if(Motor_Control.currentAccelerationRadss < Motor_Control.MaxAcceleration || Motor_Control.currentAccelerationRadss > -Motor_Control.MaxAcceleration)
    {
        GPIO_write(Board_LED2, Board_OFF);
    }else
    {
        GPIO_write(Board_LED2, Board_ON);
    }

    PWM_setDuty(Motor_Control.pwm1, Motor_Control.duty);
}


void SetupMotorClock(void)
{
    /* Construct BIOS Objects */
       Clock_Params clkParams;

       Clock_Params_init(&clkParams);
       clkParams.period = 100;
       clkParams.startFlag = TRUE;

       /* Construct a periodic Clock Instance with period = 10 system time units */
       Clock_construct(&clk0Struct, (Clock_FuncPtr)clk0Fxn,
                       100, &clkParams);
}




void initializeMotorStructValues(void)
{
    Motor_Control.pwmPeriod       = 100;      // Period and duty in microseconds
    Motor_Control.duty            = 0;          // set motor speed
    Motor_Control.interruptCount  = 0;
    Motor_Control.currentRPM      = 0;
    Motor_Control.lastRPM         = 0;
    Motor_Control.requiredRPM     = 0;
    Motor_Control.MaxAcceleration = 200;
    Motor_Control.emergencyStop   = 0;
    Motor_Control.prevError       = 0;
    Motor_Control.error           = 0;
}


void SetupMotorControlswi(void)
{
    /* Construct BIOS objects */
    Swi_Params motorSwiParams;
    Swi_Params_init(&motorSwiParams);

    /*construct motor driver SWI*/
    Swi_construct(&motorSwiStruct, (Swi_FuncPtr)MotorControlSwi, &motorSwiParams, NULL);
    motorSwiHandle = Swi_handle(&motorSwiStruct);
}


/* sets up hall affect sensors, PWM and initializes motor driving hardware*/
void MotorSetup(void)
{
      SetupMotorControlswi();
      SetupMotorClock();
      initializeMotorStructValues();

      UInt key = Swi_disable();
      /* install callback HALL A, B and C callback */
      GPIO_setCallback(Board_HALL_A, HALL_A_HWI);
      GPIO_setCallback(Board_HALL_B, HALL_B_HWI);
      GPIO_setCallback(Board_HALL_C, HALL_C_HWI);

      /* Enable interrupts */
      GPIO_enableInt(Board_HALL_A);
      GPIO_enableInt(Board_HALL_B);
      GPIO_enableInt(Board_HALL_C);

      /* Setup PWM for motors*/
      PWM_Params_init(&Motor_Control.params);
      Motor_Control.params.period = Motor_Control.pwmPeriod;
      Motor_Control.pwm1 = PWM_open(Board_PWM0, &Motor_Control.params);
      if (Motor_Control.pwm1 == NULL)
      {
          System_abort("Board_PWM0 did not open");
      }

      /* turn motor driver hardware on*/
      GPIO_write(Board_DVR_Enable, Board_ON);
      /*set Motor speed too 0*/
      PWM_setDuty(Motor_Control.pwm1, Motor_Control.duty);
      /*turn brake off*/
      GPIO_write(Board_BRAKE, Board_ON);
      /*set direction*/
      GPIO_write(Board_DIRECTION, Board_ON);

      Swi_restore(key);
}









