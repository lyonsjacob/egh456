/*
 * temp.c
 *
 *  Created on: 30 May 2019
 *      Author: jordikitto
 */

#define TEMPTASKSTACKSIZE 512
#define MAXREADINGSAVG 3

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
#include <ti/sysbios/hal/Hwi.h>

// EVENT FILES
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Mailbox.h>

// UART stuff
UART_Handle      uart0handle;
UART_Params      uart0params;
UART_Handle      uart7handle;
UART_Params      uart7params;

int i;
int isSetup = 0;

// Bytes
uint8_t calibration_byte = 0b01010101; // AKA 0x55 send data from right to left
uint8_t temp_register_byte = 0b10100000;
uint8_t last_response;
uint8_t response5[5];
uint8_t response3[3];
uint8_t response2[2];

// Clock
Void clkTempFxn();
Clock_Struct clkTempStruct;

// Temp readings
float temp1_avg;
float temp2_avg;

// Mailbox
#define MAILBOXSIZE 10
Mailbox_Struct mbxStruct;
Mailbox_Handle mbxHandle;

// Mailbox struct
typedef struct MsgObj {
    uint8_t temp1_1;
    uint8_t temp1_2;
    uint8_t temp2_1;
    uint8_t temp2_2;
} MsgObj, *Msg;

// Event
Event_Struct evtStruct;
Event_Handle evtHandle;

// ------------------ Functions ----------------------------------
Void read_temp_sensors();
Void calculate_average();

Void taskTempGetTask() {
    read_temp_sensors();
}

Void clkTempFxn() { // Clock that triggers as a Swi at intervals
    // Create task to get latest reading
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = TEMPTASKSTACKSIZE;
    taskParams.priority = 10;
    Task_create((Task_FuncPtr)taskTempGetTask, &taskParams, NULL);
}

void setup_temp_internal() {
    // ----- Setup TEMP UART
    UART_Params_init(&uart7params);
    uart7params.baudRate  = 115200;
    //uart7params.readEcho = UART_ECHO_OFF;
    uart7params.readDataMode = UART_DATA_BINARY;
    uart7params.writeDataMode = UART_DATA_BINARY;
    uart7params.parityType = UART_PAR_NONE;
    uart7handle = UART_open(Board_UART7, &uart7params);
    if (!uart7handle) {
        //System_printf("The UART Temp did not open");
    }

    Task_sleep(10);

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
    UART_read(uart7handle, &response3, sizeof(response3));

    Task_sleep(10);

    // Read address responses of temp sensor 1
    uint8_t addr_response1;
    UART_read(uart7handle, &addr_response1, sizeof(addr_response1));

    Task_sleep(10);

    // Read address responses of temp sensor 2
    uint8_t addr_response2;
    UART_read(uart7handle, &addr_response2, sizeof(addr_response2));

    Task_sleep(1500);

    // Send Last Device poll
    UART_write(uart7handle, &calibration_byte, sizeof(calibration_byte));
    uint8_t last_response_byte = 0b01010111;
    UART_write(uart7handle, &last_response_byte, sizeof(last_response_byte));

    // Will get back echo of last 2 bytes
    UART_read(uart7handle, &response2, sizeof(response2));

    Task_sleep(10);

    UART_read(uart7handle, &last_response, sizeof(last_response));

    if (last_response == addr_response2) {
        //System_printf("2nd response and last response are same!\n");
    } else {
        //System_printf("BEWARE: 2nd response and last response are NOT the same!\n");
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

    Task_sleep(10);

    // Will get back echo of last 5 bytes
    UART_read(uart7handle, &response5, sizeof(response5));

    // Event construct (used to tell mailbox send event)
    Event_construct(&evtStruct, NULL);
    evtHandle = Event_handle(&evtStruct);

    // Mailbox construct (used to store readings)
    Mailbox_Params mbxParams;
    Mailbox_Params_init(&mbxParams);
    mbxParams.readerEvent = evtHandle;
    mbxParams.readerEventId = Event_Id_02; // Enables implicit event posting
    Mailbox_construct(&mbxStruct,sizeof(MsgObj), MAILBOXSIZE, &mbxParams, NULL);
    mbxHandle = Mailbox_handle(&mbxStruct);

    // Create task to average readings
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = TEMPTASKSTACKSIZE;
    taskParams.priority = 2;
    Task_create((Task_FuncPtr)calculate_average, &taskParams, NULL);

    // Setup clock function to read every 0.5 seconds
    Clock_Params clkParams;
    Clock_Params_init(&clkParams);
    clkParams.period = 500;
    Clock_construct(&clkTempStruct, (Clock_FuncPtr)clkTempFxn, 500, &clkParams);
    Clock_start(Clock_handle(&clkTempStruct));

    //System_printf("Temperature sensors setup\n");
}

void setup_temp() {
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = TEMPTASKSTACKSIZE * 2;
    taskParams.priority = 6;
    Task_create((Task_FuncPtr)setup_temp_internal, &taskParams, NULL);
}

void read_temp_sensors() {
    //UInt taskKey = Task_disable();
    //System_printf("Reading sensors\n");
    // ------------- LETS READ A TEMP

    // Send global read
    // 1. Send calibration byte
    UART_write(uart7handle, &calibration_byte, sizeof(calibration_byte));
    // 2. Send addr byte
    UART_write(uart7handle, &last_response, sizeof(last_response));
    // 3. Send register pointer = temperature register
    UART_write(uart7handle, &temp_register_byte, sizeof(temp_register_byte));

    // Will get back echo of last 3 bytes
    UART_read(uart7handle, &response3, 3);

    // Read temperatures
    uint8_t temp_sensor_1[2];
    uint8_t temp_sensor_2[2];

    Task_sleep(10);

    // Returns furthest sensor first
    UART_read(uart7handle, &temp_sensor_2, sizeof(temp_sensor_2));
    Task_sleep(10);
    UART_read(uart7handle, &temp_sensor_1, sizeof(temp_sensor_1));

    MsgObj msg;
    msg.temp1_1 = temp_sensor_1[0];
    msg.temp1_2 = temp_sensor_1[1];
    msg.temp2_1 = temp_sensor_2[0];
    msg.temp2_2 = temp_sensor_2[1];
    Mailbox_post(mbxHandle, &msg, BIOS_NO_WAIT);
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

Void calculate_average() {
    // Read temperatures
    float temp1 = 0;
    float temp2 = 0;
    UInt readings_count = 0;

    // Mailbox and event
    MsgObj readings;
    UInt posted;
    while (1) {

        // Wait for events for char, return, space
        posted = Event_pend(evtHandle,
                          Event_Id_NONE, // AND
                          Event_Id_02, // OR
                          BIOS_WAIT_FOREVER);

        if (posted & Event_Id_02) { // Temp reading send to mailbox
            if (Mailbox_pend(mbxHandle, &readings, BIOS_NO_WAIT)) {
                temp1 += TMP107_DecodeTemperatureResult(readings.temp1_2, readings.temp1_1);
                temp2 += TMP107_DecodeTemperatureResult(readings.temp2_2, readings.temp2_1);
                readings_count++;
            }
        }

        // Average once we have enough
        if (readings_count >= MAXREADINGSAVG) {
            temp1_avg = temp1 / MAXREADINGSAVG;
            temp2_avg = temp2 / MAXREADINGSAVG;
            //System_printf("Sensor 2: %fºC \t Sensor 1: %fºC\n", temp2_avg, temp1_avg);
            //System_flush();

            // Reset
            readings_count = 0;
            temp1 = 0;
            temp2 = 0;
        }
    }
}

int get_temp1(int resolution) {
    return (int)(temp1_avg * resolution);
}

int get_temp2(int resolution) {
    return (int)(temp2_avg * resolution);
}

