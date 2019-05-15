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
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/drivers/PWM.h>


/* Board Header file */
#include "Board.h"
#include <inc/hw_ints.h>


struct motor_control
{
    PWM_Handle pwm1;
    int hall_A;
    int hall_B;
    int hall_C;
    PWM_Params params;
    uint16_t   pwmPeriod;
    uint16_t   duty;
};

struct motor_control Motor_Control;


Void HALL_A_HWI(unsigned int index)
{
    //
    // Clear the asserted interrupts.
    //
    GPIO_clearInt(Board_HALL_A);
    if(GPIO_read(Board_HALL_A)){
        GPIO_write(Board_STATE0, Board_ON);
        GPIO_write(Board_LED0, Board_ON);
    }else{
        GPIO_write(Board_STATE0, Board_OFF);
        GPIO_write(Board_LED0, Board_ON);
    }
}

Void HALL_B_HWI(unsigned int index)
{
    //
    // Clear the asserted interrupts.
    //
    GPIO_clearInt(Board_HALL_B);
    if(GPIO_read(Board_HALL_B)){
        GPIO_write(Board_STATE1, Board_ON);
        GPIO_write(Board_LED1, Board_ON);
    }else{
        GPIO_write(Board_STATE1, Board_OFF);
        GPIO_write(Board_LED1, Board_ON);
    }

}

Void HALL_C_HWI(unsigned int index)
{
    //
    // Clear the asserted interrupts.
    //
    GPIO_clearInt(Board_HALL_C);
    if(GPIO_read(Board_HALL_C)){
        GPIO_write(Board_STATE2, Board_ON);
        GPIO_write(Board_LED2, Board_ON);
    }else{
        GPIO_write(Board_STATE2, Board_OFF);
        GPIO_write(Board_LED2, Board_ON);
    }

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

    /* Turn on user LED */
    GPIO_write(Board_LED0, Board_ON);
    GPIO_write(Board_LED1, Board_ON);
    GPIO_write(Board_LED2, Board_ON);

    /* SysMin will only print to the console when you call flush or exit */
    System_flush();

    /* install Button callback HALL A, B and C callback */
    GPIO_setCallback(Board_HALL_A, HALL_A_HWI);
    GPIO_setCallback(Board_HALL_B, HALL_B_HWI);
    GPIO_setCallback(Board_HALL_C, HALL_C_HWI);

    /* Enable interrupts */
    GPIO_enableInt(Board_HALL_A);
    GPIO_enableInt(Board_HALL_B);
    GPIO_enableInt(Board_HALL_C);

    Motor_Control.pwmPeriod = 3000;      // Period and duty in microseconds
    Motor_Control.duty = 200;          // set motor speed

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
    GPIO_write(Board_BRAKE, Board_ON);
    /*set direction*/
    GPIO_write(Board_DIRECTION, Board_ON);

    /* Start BIOS */
    BIOS_start();

    return (0);
}
