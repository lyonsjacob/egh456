/*
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== empty.c ========
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


/* Board Header file */
#include "Board.h"
#include <inc/hw_ints.h>


struct motor_control
{
    PWM_Handle  pwm1;
    PWM_Params  params;
    uint8_t     emergencyStop;
    uint16_t    pwmPeriod, duty;
    uint32_t    currentRPM, lastRPM, requiredRPM;
    uint32_t    interruptCount;
    uint32_t    MaxAcceleration;


};

struct motor_control Motor_Control;
/* Motor speed control swi and clock prams*/
Swi_Struct motorSwiStruct;
Swi_Handle motorSwiHandle;
Clock_Struct clk0Struct;


/*Sets motors RPM*/
void setRPM(uint32_t RPM)
{
    if(!Motor_Control.emergencyStop){
        Motor_Control.requiredRPM  = RPM;
    }

}

/*Gets the current motor RPM*/
uint32_t getRPM(void)
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
    if(GPIO_read(Board_HALL_A)){
        GPIO_write(Board_STATE0, Board_ON);
        GPIO_write(Board_LED0, Board_ON);
    }else{
        GPIO_write(Board_STATE0, Board_OFF);
        GPIO_write(Board_LED0, Board_OFF);
    }
}



void HALL_B_HWI(unsigned int index)
{
    Motor_Control.interruptCount++;
    //
    // Clear the asserted interrupts.
    //
    GPIO_clearInt(Board_HALL_B);
    if(GPIO_read(Board_HALL_B)){
        GPIO_write(Board_STATE2, Board_ON);
        GPIO_write(Board_LED1, Board_ON);
    }else{
        GPIO_write(Board_STATE2, Board_OFF);
        GPIO_write(Board_LED1, Board_OFF);
    }

}



void HALL_C_HWI(unsigned int index)
{
    Motor_Control.interruptCount++;
    //
    // Clear the asserted interrupts.
    //
    GPIO_clearInt(Board_HALL_C);
    if(GPIO_read(Board_HALL_C)){
        GPIO_write(Board_STATE0, Board_ON);
        GPIO_write(Board_LED2, Board_ON);
    }else{
        GPIO_write(Board_STATE0, Board_OFF);
        GPIO_write(Board_LED2, Board_OFF);
    }

}

void clk0Fxn(UArg arg0)
{
    Motor_Control.lastRPM=Motor_Control.currentRPM;
    double revolutions = (double)Motor_Control.interruptCount/24.0;
    Motor_Control.currentRPM = 600*revolutions;
    Motor_Control.interruptCount=0;
    Swi_post(motorSwiHandle);
}



void MotorControlSwi(UArg arg0, UArg arg1)
{
    uint32_t error = Motor_Control.requiredRPM -Motor_Control.currentRPM;

    uint32_t AccelerationRadss = ((Motor_Control.lastRPM -Motor_Control.currentRPM)*0.10472)*10;

    /*set duty cycle*/
    Motor_Control.duty = Motor_Control.duty+error*0.1;

    /*check if motor duty cycle is at 100%*/
    if(Motor_Control.duty > Motor_Control.pwmPeriod ){
        Motor_Control.duty = Motor_Control.pwmPeriod;
    }

    PWM_setDuty(Motor_Control.pwm1, Motor_Control.duty);

    /*reset emergency stop if motor is stationary*/
    if((Motor_Control.emergencyStop)&&(!Motor_Control.requiredRPM)){
        Motor_Control.emergencyStop = 0;
    }

///////////DeBug/////////////////////////////////////////////////
    System_printf("RPM = %lu\n", (ULong)Motor_Control.currentRPM);
    System_flush();
    System_printf("ACC = %lu\n", (ULong)AccelerationRadss);
    System_flush();
}







void SetupMotorClock(void)
{
    /* Construct BIOS Objects */
       Clock_Params clkParams;

       Clock_Params_init(&clkParams);
       clkParams.period = 100;
       clkParams.startFlag = TRUE;

       /* Construct a periodic Clock Instance with period = 100 system time units */
       Clock_construct(&clk0Struct, (Clock_FuncPtr)clk0Fxn,
                       100, &clkParams);
}

void initializeMotorStructValues(void){
    Motor_Control.pwmPeriod       = 3000;      // Period and duty in microseconds
    Motor_Control.duty            = 0;          // set motor speed
    Motor_Control.interruptCount  = 0;
    Motor_Control.currentRPM      = 0;
    Motor_Control.lastRPM         = 0;
    Motor_Control.MaxAcceleration = 100;
    Motor_Control.emergencyStop   = 0;
}


/* sets up hall affect sensors, PWM and initializes motor driving hardware*/
void MotorSetup(void)
{
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
      if (Motor_Control.pwm1 == NULL) {
          System_abort("Board_PWM0 did not open");
      }

      /* turn motor driver hardware on*/
      GPIO_write(Board_DVR_Enable, Board_ON);
      /*set Motor speed too 0*/
      PWM_setDuty(Motor_Control.pwm1, Motor_Control.duty);
      /*turn brake off*/
      GPIO_write(Board_BRAKE, Board_OFF);
      /*set direction*/
      GPIO_write(Board_DIRECTION, Board_ON);

      Swi_restore(key);
}






/*
 *  ======== main ========
 */
int main(void)
{
    /* Call board init functions */
    Board_initGeneral();
    Board_initGPIO();
    Board_initPWM();

    MotorSetup();

    /* Construct BIOS objects */
    Swi_Params motorSwiParams;
    Swi_Params_init(&motorSwiParams);

    /*construct motor driver SWI*/
    Swi_construct(&motorSwiStruct, (Swi_FuncPtr)MotorControlSwi, &motorSwiParams, NULL);
    motorSwiHandle = Swi_handle(&motorSwiStruct);

    /* Start BIOS */
    BIOS_start();

    return (0);
}


