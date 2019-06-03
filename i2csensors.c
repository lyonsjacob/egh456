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

#include "GUI.h"

float convertedLux;
float convertedAcc[3];

I2C_Handle      i2c;
I2C_Transaction i2cTransaction;

uint16_t        lux;
uint8_t         luxTxBuffer[3];
uint8_t         luxRxBuffer[6];

int16_t         acc;
uint8_t         accTxBuffer[3];
uint8_t         accRxBuffer[6];

// Register addresses
#define LUX_REG_RESULT                      0x00
#define LUX_REG_CONFIGURATION               0x01
#define ACC_REG_RESULT                      0x12
#define ACC_REG_CONFIGURATION               0x00


int getLux(int res)
{
    return (int)(res * convertedLux);
}

int getAcc(int res)
{
    int absAcc;
    absAcc = (int)(res * sqrt(pow(convertedAcc[0],2) + pow(convertedAcc[1],2) + pow(convertedAcc[2],2)));
    return absAcc;
}

void initLux()
{
    convertedLux = 0;
    luxTxBuffer[0] = LUX_REG_CONFIGURATION;
    luxTxBuffer[1] = 0xC4;
    luxTxBuffer[2] = 0x10;
    i2cTransaction.slaveAddress = Board_OPT3001_ADDR;
    i2cTransaction.writeBuf = luxTxBuffer;
    i2cTransaction.writeCount = 3;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    if (!I2C_transfer(i2c, &i2cTransaction)) {
        System_abort("Error Initiating Lux Sensor\n");
    }
    /*else {
        System_printf("Lux Sensor Configured!\n");
    }*/
}

void initAcc()
{
    convertedAcc[0] = 0;
    convertedAcc[1] = 0;
    convertedAcc[2] = 0;
    accTxBuffer[0] = 0x7E; //Power mode set register
    i2cTransaction.slaveAddress = Board_BMI160_ADDR;
    i2cTransaction.writeBuf = accTxBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = accRxBuffer;
    i2cTransaction.readCount = 1;

    if (!I2C_transfer(i2c, &i2cTransaction)) {
        System_abort("Error Reading Accelorometer Power Mode\n");
    }
    /*else {
        System_printf("Accelerometer Power Mode Read!\n");
    }*/

    accRxBuffer[0] = 0b00010001; //Normal power mode
    accTxBuffer[1] = accRxBuffer[0];

    i2cTransaction.writeBuf = accTxBuffer;
    i2cTransaction.writeCount = 2;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    if (!I2C_transfer(i2c, &i2cTransaction)) {
        System_abort("Error Changing Accelorometer Power Mode\n");
    }
    /*else {
        System_printf("Accelerometer Power Mode Changed!\n");
    }*/

    accTxBuffer[0] = 0x41; //Power mode set register
    i2cTransaction.slaveAddress = Board_BMI160_ADDR;
    i2cTransaction.writeBuf = accTxBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = accRxBuffer;
    i2cTransaction.readCount = 1;

    if (!I2C_transfer(i2c, &i2cTransaction)) {
        System_abort("Error Reading Accelorometer Range\n");
    }
    /*else {
        System_printf("Accelerometer Range Read!\n");
    }*/

    accRxBuffer[0] = 0b00000101; //Normal power mode
    accTxBuffer[1] = accRxBuffer[0];

    i2cTransaction.writeBuf = accTxBuffer;
    i2cTransaction.writeCount = 2;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    if (!I2C_transfer(i2c, &i2cTransaction)) {
        System_abort("Error Changing Accelorometer Range\n");
    }
    /*else {
        System_printf("Accelerometer Range Changed!\n");
    }*/
}

void readLux()
{
    luxTxBuffer[0] = LUX_REG_RESULT;
    i2cTransaction.slaveAddress = Board_OPT3001_ADDR;
    i2cTransaction.writeBuf = luxTxBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = luxRxBuffer;
    i2cTransaction.readCount = 2;

//    char luxStr[40];
    uint16_t result, exponent;

    if (I2C_transfer(i2c, &i2cTransaction)) {

        lux = luxRxBuffer[0] << 8 | luxRxBuffer[1];
        result = lux & 0x0FFF;
        exponent = (lux & 0xF000) >> 12;
        convertedLux = result * (0.01 * exp2(exponent));
        if(convertedLux < 1){
            changeDisplayToNight();
        }
        if(convertedLux > 1){
            changeDisplayToDay();
        }

    }
    /*else {
        System_printf("I2C Bus fault\n");
    }*/
}

void readAcc()
{
    accTxBuffer[0] = 0x12; //First acc reading register
    i2cTransaction.slaveAddress = Board_BMI160_ADDR;
    i2cTransaction.writeBuf = accTxBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = accRxBuffer;
    i2cTransaction.readCount = 6;

//    char accStr[40];

    if (I2C_transfer(i2c, &i2cTransaction)) {
        int i;
        for(i = 0; i < 6; i+= 2){
            acc = (int16_t)((accRxBuffer[i + 1] << 8) | accRxBuffer[i]);
            convertedAcc[i/2] = ((acc * 0.061)/1000) * 9.8;
        }
    }
    /*else {
        System_printf("I2C Bus fault\n");
    }*/
}

