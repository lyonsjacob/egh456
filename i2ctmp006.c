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
 *    ======== i2ctmp006.c ========
 */
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>

/* Example/Board Header files */
#include "Board.h"

//#include "GUI.h"

#define TASKSTACKSIZE       2048

Task_Struct task0Struct;
Char task0Stack[TASKSTACKSIZE];
float convertedLux = 0;
float convertedAcc = 0;

I2C_Handle      i2c;
I2C_Params      i2cParams;
I2C_Transaction i2cTransaction;

// ----------------------- Constants -----------------------


// Register addresses
#define LUX_REG_RESULT                      0x00
#define LUX_REG_CONFIGURATION               0x01
#define ACC_REG_RESULT                      0x12
#define ACC_REG_CONFIGURATION               0x00


float getLUX(void)
{
    return convertedLux;
}

/*
 *  ======== luxFxn ========
 *  Task for this function is created statically. See the project's .cfg file.
 */

Void luxFxn(UArg arg0, UArg arg1)
{
    //=======CREATE VARIABLES==================
    uint16_t        lux;
    int16_t         acc;
    uint8_t         txBuffer[3];
    uint8_t         rxBuffer[6];

    //=====CODE FOR INITIATING LIGH REG========

    txBuffer[0] = LUX_REG_CONFIGURATION;
    txBuffer[1] = 0xC4;
    txBuffer[2] = 0x10;
    i2cTransaction.slaveAddress = Board_OPT3001_ADDR;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 3;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    if (!I2C_transfer(i2c, &i2cTransaction)) {
        System_abort("Error Initiating Lux Sensor\n");
    }
    else {
        System_printf("Lux Sensor Configured!\n");
    }

    //======CODE FOR CONFIGURING ACC=============

    txBuffer[0] = 0x7E; //Power mode set register
    i2cTransaction.slaveAddress = Board_BMI160_ADDR;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 1;

    if (!I2C_transfer(i2c, &i2cTransaction)) {
        System_abort("Error Configuring Accelorometer\n");
    }
    else {
        System_printf("Accelerometer Configured!\n");
    }

    rxBuffer[0] = 0b00010001; //Normal power mode
    txBuffer[1] = rxBuffer[0];

    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 2;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    if (!I2C_transfer(i2c, &i2cTransaction)) {
        System_abort("Error Changing Accelorometer\n");
    }
    else {
        System_printf("Accelerometer Changed!\n");
    }

    //======CODE TO READ LUX AND ACC SENSORS============



    char luxStr[40];
    char accStr[40];
    uint16_t result, exponent;

    while(1) {

        txBuffer[0] = LUX_REG_RESULT;
        i2cTransaction.slaveAddress = Board_OPT3001_ADDR;
        i2cTransaction.writeBuf = txBuffer;
        i2cTransaction.writeCount = 1;
        i2cTransaction.readBuf = rxBuffer;
        i2cTransaction.readCount = 2;

        if (I2C_transfer(i2c, &i2cTransaction)) {

            lux = rxBuffer[0] << 8 | rxBuffer[1];
            result = lux & 0x0FFF;
            exponent = (lux & 0xF000) >> 12;
            convertedLux = result * (0.01 * exp2(exponent));
            sprintf(luxStr, "Lux: %5.2f\n", convertedLux);
            System_printf("%s\n", luxStr);
//            if(convertedLux < 5){
//                changeDisplayToNight();
//            }
//            if(covertedLux >5){
//                changeDisplayToDay();
//            }

        }
        else {
            System_printf("I2C Bus fault\n");
        }

        txBuffer[0] = 0x12; //First acc reading register
        i2cTransaction.slaveAddress = Board_BMI160_ADDR;
        i2cTransaction.writeBuf = txBuffer;
        i2cTransaction.writeCount = 1;
        i2cTransaction.readBuf = rxBuffer;
        i2cTransaction.readCount = 6;

        if (I2C_transfer(i2c, &i2cTransaction)) {
            int i;
            for(i = 0; i < 6; i += 2){
                acc = (int16_t)((rxBuffer[i + 1] << 8) | rxBuffer[i]);
                convertedAcc = (acc * 0.061)/1000;
                sprintf(accStr, "Itteration: %d, Acc: %5.2f\n", i, convertedAcc);
                System_printf("%s\n", accStr);
            }
        }
        else {
            System_printf("I2C Bus fault\n");
        }
        System_printf("\n");
        System_flush();
        Task_sleep(50);
    }

    //==========================================
}

///*
// *  ======== accFxn ========
// *  Task for this function is created statically. See the project's .cfg file.
// */
Void accFxn(UArg arg0, UArg arg1)
{
    //=======CREATE VARIABLES==================
    int16_t         acc;
    uint8_t         txBuffer[3];
    uint8_t         rxBuffer[6];

    //======CODE FOR CONFIGURING ACC=============

    txBuffer[0] = 0x7E; //Power mode set register
    i2cTransaction.slaveAddress = Board_BMI160_ADDR;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 1;

    if (!I2C_transfer(i2c, &i2cTransaction)) {
        System_abort("Error Configuring Accelorometer\n");
    }
    else {
        System_printf("Accelerometer Configured!\n");
    }

    rxBuffer[0] = 0b00010001; //Normal power mode
    txBuffer[1] = rxBuffer[0];

    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 2;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    if (!I2C_transfer(i2c, &i2cTransaction)) {
        System_abort("Error Changing Accelorometer\n");
    }
    else {
        System_printf("Accelerometer Changed!\n");
    }

    //=======CODE TO CHECK PMU_STATUS============

    txBuffer[0] = 0x03; //PMU_STATUS register
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 1;

    if (!I2C_transfer(i2c, &i2cTransaction)) {
        System_abort("Error Reading PMU_STATUS\n");
    }
    else {
        System_printf("PMU_STATUS: %x\n", rxBuffer[0]);
    }

    //=========CODE TO READ ACC==================

    txBuffer[0] = 0x12; //First acc reading register
    i2cTransaction.slaveAddress = Board_BMI160_ADDR;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 6;

    char accStr[40];

    while(1) {
        if (I2C_transfer(i2c, &i2cTransaction)) {
            int i;
            for(i = 0; i < 6; i += 2){
                acc = (int16_t)((rxBuffer[i + 1] << 8) | rxBuffer[i]);
                convertedAcc = (acc * 0.244)/1000;
                sprintf(accStr, "Itteration: %d, Acc: %5.2f\n", i, convertedAcc);
                System_printf("%s\n", accStr);
            }
        }
        else {
            System_printf("I2C Bus fault\n");
        }
        System_printf("\n");
        System_flush();
        Task_sleep(50);
    }

    //===========================================
}

void setupI2C2( void )
{
    //=======CREATE I2C FOR USAGE===============

    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    i2c = I2C_open(Board_I2C_LUX, &i2cParams);
    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }
    else {
        System_printf("I2C Initialized!\n");
    }
}

/*
 *  ======== main ========
 */
//int main(void)
//{
//    Task_Params taskParams;
//
//    /* Call board init functions */
//    Board_initGeneral();
//    Board_initGPIO();
//    Board_initI2C();
//
//    setupI2C2();
//
//    /* Construct opt3001 Task thread */
//    Task_Params_init(&taskParams);
//    taskParams.stackSize = TASKSTACKSIZE;
//    taskParams.stack = &task0Stack;
//    Task_construct(&task0Struct, (Task_FuncPtr)luxFxn, &taskParams, NULL);
//
//    /* Start BIOS */
//    BIOS_start();
//
//    return (0);
//}
