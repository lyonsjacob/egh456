/* C header files */
#include <stdint.h>
#include <stdbool.h>

/* XDCtools Header files */
#include <xdc/std.h>

/* Board Header file */
#include "Board.h"
#include <inc/hw_ints.h>
#include "driverlib/sysctl.h"

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

/* TI-RTOS Header files */
// #include <ti/drivers/EMAC.h>
#include <ti/drivers/GPIO.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/knl/Swi.h>
// #include <ti/drivers/I2C.h>
// #include <ti/drivers/SDSPI.h>
// #include <ti/drivers/SPI.h>
#include <ti/drivers/UART.h>
// #include <ti/drivers/USBMSCHFatFs.h>
// #include <ti/drivers/Watchdog.h>
// #include <ti/drivers/WiFi.h>

#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
//#include "driverlib/rom.h"
//#include "driverlib/rom_map.h"
#include "driverlib/uart.h"

// GRLIB FILES
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "drivers/Kentec320x240x16_ssd2119_spi.h"
#include "drivers/touch.h"
#include "driverlib/udma.h"
#include "driverlib/fpu.h"

#include "driverlib/hibernate.h"
#include "inc/hw_hibernate.h"

// UART extra FILES
#include "inc/hw_ints.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"

// Our header files:
#include "GUI.h"

#define TASKSTACKSIZE  4024
#define UARTTASKSTACKSIZE 768

uint32_t g_ui32SysClock;

Task_Struct gui_struct;
Char gui_stack[TASKSTACKSIZE];
Task_Struct uart_struct;
Char uart_stack[UARTTASKSTACKSIZE];

// UART stuff
UART_Handle      uart0handle;
UART_Params      uart0params;
UART_Handle      uart7handle;
UART_Params      uart7params;

#ifdef ewarm
#pragma data_alignment=1024
tDMAControlTable psDMAControlTable[64];
#elif defined(ccs)
#pragma DATA_ALIGN(psDMAControlTable, 1024)
tDMAControlTable psDMAControlTable[64];
#else
tDMAControlTable psDMAControlTable[64] __attribute__ ((aligned(1024)));
#endif

// TOGGLE GPIO LIGHTS // off = 0 on = 1
void toggleLight(int light, int tog){

    if(light==0) GPIO_write(Board_LED0, tog);
    if(light==1) GPIO_write(Board_LED1, tog);

    // add lights here if needed
}

/*
 *  ======== Tasks ========
 */

// GUI Task Function

Void guiRun() {
    tContext sContext;
    bool bUpdate;

    FPUEnable();
    FPULazyStackingEnable();

    // graphics
    Kentec320x240x16_SSD2119Init(g_ui32SysClock);
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);

    // screen
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(10);
    uDMAControlBaseSet(&psDMAControlTable[0]);
    uDMAEnable();
    TouchScreenInit(g_ui32SysClock);
    TouchScreenCallbackSet(WidgetPointerMessage);

    //set up clock
    SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);
    HibernateEnableExpClk(g_ui32SysClock);
    HibernateRTCEnable();
    HibernateCounterMode(HIBERNATE_COUNTER_24HR);

    /*******************************/
    /* SET YOUR CALANDER TIME HERE */
    DateTimeDefaultSet(10, 30);
    /*******************************/
    /*******************************/

    // gui functionality
    GUI_init();

    while (1) {
        bUpdate = DateTimeDisplayGet();
        if(bUpdate) run_timer();
        WidgetMessageQueueProcess();
    }
}

Void uartRun() {
    // ----- Setup UART (for printing data)

    UART_Params_init(&uart0params); // Setup to defaults
    uart0params.baudRate  = 115200;
    uart0params.readEcho = UART_ECHO_OFF;
    uart0handle = UART_open(Board_UART0, &uart0params);
    if (!uart0handle) {
        System_printf("UART Putty did not open");
    }

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

    const unsigned char hello[] = "Address Initialize\n";
    int ret = UART_write(uart0handle, hello, sizeof(hello));
    System_printf("The UART wrote %d bytes\n", ret);

    // Setup connection (address initialize)
    // Send calibration byte
    uint8_t calibration_byte = 0b01010101; // AKA 0x55 send data from right to left
    UART_write(uart7handle, &calibration_byte, sizeof(calibration_byte));

    // Send address initialize (command)
    uint8_t addr_init_byte = 0b10010101;
    UART_write(uart7handle, &addr_init_byte, sizeof(addr_init_byte));

    // Send Address assign byte
    uint8_t addr_assign_byte = 0b00001101;
    UART_write(uart7handle, &addr_assign_byte, sizeof(addr_assign_byte));

    uint8_t echo_response;
    // Will get back echo of last 3 bytes
    int i;
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

    uint8_t last_response;
    UART_read(uart7handle, &last_response, sizeof(last_response));

    if (last_response == addr_response2) {
        System_printf("2nd response and last response are same!");
    } else {
        System_printf("BEWARE: 2nd response and last response are NOT the same!");
    }
    System_flush();
}

/*
 *  ======== main ========
 */

Hwi_Handle adcHwi;

void setup_adc_hwi(){
    Hwi_Params adc_hwi_params;
    Error_Block adc_hwi_eb;
    Error_init(&adc_hwi_eb);
    Hwi_Params_init(&adc_hwi_params);
    int id = INT_ADC0SS3_TM4C123;
    adcHwi = Hwi_create(id, (Hwi_FuncPtr)TouchScreenIntHandler, &adc_hwi_params, &adc_hwi_eb);
    if (adcHwi == NULL) System_abort("Hwi create failed");
}

void setup_gui_task(){
    Task_Params gui_params;
    Task_Params_init(&gui_params);
    gui_params.stackSize = TASKSTACKSIZE;
    gui_params.stack = &gui_stack;
    gui_params.priority = 1;
    Task_construct(&gui_struct, (Task_FuncPtr)guiRun, &gui_params, NULL);
}

void setup_uart_task() {
    Task_Params uart_params;
    Task_Params_init(&uart_params);
    uart_params.stackSize = UARTTASKSTACKSIZE;
    uart_params.stack = &uart_stack;
    uart_params.priority = 10;
    Task_construct(&uart_struct, (Task_FuncPtr)uartRun, &uart_params, NULL);
}

int main(void)
{
    /* Call board init functions */

    Board_initGeneral();
    // Board_initEMAC();
    Board_initGPIO();
    // Board_initI2C();
    // Board_initSDSPI();
    // Board_initSPI();
    Board_initUART();
    // Board_initUSB(Board_USBDEVICE);
    // Board_initUSBMSCHFatFs();
    // Board_initWatchdog();
    // Board_initWiFi();


    // Set the clocking to run directly from the crystal at 120MHz.
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN
                   | SYSCTL_USE_PLL |SYSCTL_CFG_VCO_480), 120000000);


    // Setup tasks
    //setup_gui_task();
    setup_uart_task();

    // Setup Hwis
    setup_adc_hwi();

    // Setup Swis

    /* Start BIOS */
    BIOS_start();

    return (0);
}
