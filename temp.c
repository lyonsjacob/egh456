/*
 * temp.c
 *
 *  Created on: 30 May 2019
 *      Author: jordikitto
 */

#define TEMPTASKSTACKSIZE 1024

// Includes
#include <stdint.h>
#include <stdbool.h>
#include "Board.h"
#include <ti/drivers/UART.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/BIOS.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Swi.h>

// UART stuff
UART_Handle      uart0handle;
UART_Params      uart0params;
UART_Handle      uart7handle;
UART_Params      uart7params;

int i;

// Bytes
uint8_t calibration_byte = 0b01010101; // AKA 0x55 send data from right to left
uint8_t last_response;
uint8_t echo_response;

// Clock
Void clkTempFxn();
Clock_Struct clkTempStruct;

// Swi
Swi_Struct swiTempStruct;
Swi_Handle swiTempHandle;

// Task
Task_Struct taskTempStruct;
Char taskTempStack[TEMPTASKSTACKSIZE];

// Temp readings
float temp1;
float temp1_avg;
float temp2;
float temp2_avg;
float temp1_readings[3];
float temp2_readings[3];
int readings_index = 0;

// Functions
Void read_temp_sensors();


Void clkTempFxn() {
    Swi_post(swiTempHandle);
}

Void swiTempGetFxn() {
    runTaskTempAvgFxn();
}

Void runTaskTempAvgFxn() {
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = TEMPTASKSTACKSIZE;
    taskParams.stack = &taskTempStack;
    Task_construct(&taskTempStruct, (Task_FuncPtr)taskTempAvgFxn, &taskParams, NULL);
}

Void taskTempAvgFxn() {
    read_temp_sensors();
}

void setup_temp() {
    /*
    // ----- Setup UART (for printing data)
    UART_Params_init(&uart0params); // Setup to defaults
    uart0params.baudRate  = 115200;
    uart0params.readEcho = UART_ECHO_OFF;
    uart0handle = UART_open(Board_UART0, &uart0params);
    if (!uart0handle) {
        System_printf("UART Putty did not open");
    }
    const unsigned char hello[] = "Address Initialize\n";
    int ret = UART_write(uart0handle, hello, sizeof(hello));
    System_printf("The UART wrote %d bytes\n", ret);
    */

    // ----- Setup TEMP UART
    UART_Params_init(&uart7params);
    uart7params.baudRate  = 115200;
    //uart7params.readEcho = UART_ECHO_OFF;
    uart7params.readDataMode = UART_DATA_BINARY;
    uart7params.writeDataMode = UART_DATA_BINARY;
    uart7params.parityType = UART_PAR_NONE;
    uart7handle = UART_open(Board_UART7, &uart7params);
    if (!uart7handle) {
        System_printf("The UART Temp did not open");
    }

    // Setup connection (address initialize)
    // Send calibration byte
    UART_write(uart7handle, &calibration_byte, sizeof(calibration_byte));

    // Send address initialize (command)
    uint8_t addr_init_byte = 0b10010101;
    UART_write(uart7handle, &addr_init_byte, sizeof(addr_init_byte));

    // Send Address assign byte
    uint8_t addr_assign_byte = 0b00001101;
    UART_write(uart7handle, &addr_assign_byte, sizeof(addr_assign_byte));

    // Will get back echo of last 3 bytes
    for (i=0; i < 3; i++) {
        UART_read(uart7handle, &echo_response, sizeof(echo_response));
    }

    // Read address responses of temp sensor 1
    uint8_t addr_response1;
    UART_read(uart7handle, &addr_response1, sizeof(addr_response1));

    Task_sleep(7);

    // Read address responses of temp sensor 2
    uint8_t addr_response2;
    UART_read(uart7handle, &addr_response2, sizeof(addr_response2));

    Task_sleep(1250);

    // Send Last Device poll
    UART_write(uart7handle, &calibration_byte, sizeof(calibration_byte));
    uint8_t last_response_byte = 0b01010111;
    UART_write(uart7handle, &last_response_byte, sizeof(last_response_byte));

    // Will get back echo of last 2 bytes
    for (i=0; i < 2; i++) {
        UART_read(uart7handle, &echo_response, sizeof(echo_response));
    }

    Task_sleep(7);

    UART_read(uart7handle, &last_response, sizeof(last_response));

    if (last_response == addr_response2) {
        System_printf("2nd response and last response are same!\n");
    } else {
        System_printf("BEWARE: 2nd response and last response are NOT the same!\n");
    }
    System_flush();

    // ------------- SETUP CONFIGURATION REGISTERS
    uint8_t config_settings = 0;
    uint8_t config_addr = 0xA1;
    uint8_t global_write = 0b00010001;
    UART_write(uart7handle, &calibration_byte, sizeof(calibration_byte));
    UART_write(uart7handle, &global_write, sizeof(global_write));
    UART_write(uart7handle, &config_addr, sizeof(config_settings));
    UART_write(uart7handle, &config_settings, sizeof(config_settings));
    UART_write(uart7handle, &config_settings, sizeof(config_settings));

    // Will get back echo of last 4 bytes
    for (i=0; i < 5; i++) {
        UART_read(uart7handle, &echo_response, sizeof(echo_response));
    }

    // Setup clock function to read every 0.5 seconds
    Clock_Params clkParams;
    Clock_Params_init(&clkParams);
    clkParams.period = 500;
    Clock_construct(&clkTempStruct, (Clock_FuncPtr)clkTempFxn, 500, &clkParams);
    Clock_start(Clock_handle(&clkTempStruct));

    // Setup swi to get values
    Swi_Params swiParams;
    Swi_Params_init(&swiParams);
    // Set priority? Trigger?
    Swi_construct(&swiTempStruct, (Swi_FuncPtr)swiTempGetFxn, &swiParams, NULL);
    swiTempHandle = Swi_handle(&swiTempStruct);
}

float TMP107_DecodeTemperatureResult(int HByte, int LByte){
    // convert raw byte response to floating point temperature
    int Bytes;
    float temperature;
    Bytes = HByte << 8 | LByte;
    Bytes &= 0xFFFC; //Mask NVM bits not used in Temperature Result
    temperature = (float) Bytes / 256;
    return temperature;
}

void read_temp_sensors() {
    // ------------- LETS READ A TEMP
    uint8_t temp_register_byte = 0b10100000;

    // Send global read
    // 1. Send calibration byte
    UART_write(uart7handle, &calibration_byte, sizeof(calibration_byte));
    // 2. Send addr byte
    UART_write(uart7handle, &last_response, sizeof(last_response));
    // 3. Send register pointer = temperature register
    UART_write(uart7handle, &temp_register_byte, sizeof(temp_register_byte));

    Task_sleep(7);

    // Will get back echo of last 3 bytes
    for (i=0; i < 3; i++) {
        UART_read(uart7handle, &echo_response, 1);
    }

    Task_sleep(7);

    // Read temperatures
    uint8_t temp_sensor_1[2];
    uint8_t temp_sensor_2[2];

    // Returns furthest sensor first
    for (i=0; i < 2; i++) {
        UART_read(uart7handle, &temp_sensor_2[i], sizeof(temp_sensor_2[i]));
    }
    Task_sleep(7);
    for (i=0; i < 2; i++) {
        UART_read(uart7handle, &temp_sensor_1[i], sizeof(temp_sensor_1[i]));
    }

    temp1 = TMP107_DecodeTemperatureResult(temp_sensor_1[1], temp_sensor_1[0]);
    temp2 = TMP107_DecodeTemperatureResult(temp_sensor_2[1], temp_sensor_2[0]);
    System_printf("Sensor 2: %fºC \t Sensor 1: %fºC\n", temp2, temp1);
    System_flush();
}

