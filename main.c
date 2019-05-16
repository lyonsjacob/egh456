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
// #include <ti/drivers/UART.h>
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

uint32_t g_ui32SysClock;

// Hwi Interrupts
Hwi_Handle myHwi;

Task_Struct gui_struct;
Char gui_stack[TASKSTACKSIZE];

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

/*
 *  ======== Hwi ========
 */

void motorTempHwi() {
    uint32_t ui32Status;

    //
    // Get the interrrupt status.
    //
    ui32Status = UARTIntStatus(UART7_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    UARTIntClear(UART7_BASE, ui32Status);

    //
    // Loop while there are characters in the receive FIFO.
    //
    char rec_char;
    while(UARTCharsAvail(UART7_BASE))
    {
        //
        // Read the next character from the UART and write it back to the UART.
        //

        rec_char = (char)UARTCharGetNonBlocking(UART7_BASE);

        UARTCharPutNonBlocking(UART0_BASE, rec_char);
    }
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
    Task_construct(&gui_struct, (Task_FuncPtr)guiRun, &gui_params, NULL);
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
    // Board_initUART();
    // Board_initUSB(Board_USBDEVICE);
    // Board_initUSBMSCHFatFs();
    // Board_initWatchdog();
    // Board_initWiFi();


    // Set the clocking to run directly from the crystal at 120MHz.
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN
                     | SYSCTL_USE_PLL |SYSCTL_CFG_VCO_480), 120000000);


    // Setup tasks
    setup_gui_task();


    // Setup Hwis
    setup_adc_hwi();


    // Setup Swis

    // ----- Setup UART (for printing data)
    // Setup hardware Hwi
    Hwi_Params hwiParams;
    Error_Block hwi_eb;
    Error_init(&hwi_eb);

    Hwi_Params_init(&hwiParams);
    int id = INT_UART7_TM4C123;
    myHwi = Hwi_create(id, (Hwi_FuncPtr)motorTempHwi, &hwiParams, &hwi_eb);

    if (myHwi == NULL) System_abort("Hwi create failed");

    // Enable the peripherals used by this example.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Set GPIO A0 and A1 as UART pins.
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure the UART for 115,200, 8-N-1 operation.
    UARTConfigSetExpClk(UART0_BASE, g_ui32SysClock, 115200,
                           (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                            UART_CONFIG_PAR_NONE));

    UARTCharPutNonBlocking(UART0_BASE, 'y');

    // ---- Setup UART (for reading motor temp)
    // Enable peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART7);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);

    // Set UART pins
    GPIOPinConfigure(GPIO_PC4_U7RX);
    GPIOPinConfigure(GPIO_PC5_U7TX);
    GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    // Configure the UART for 115,200, 8-N-1 operation.
    UARTConfigSetExpClk(UART7_BASE, g_ui32SysClock, 115200,
                           (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                            UART_CONFIG_PAR_NONE));

    // Enable the UART interrupt.
    IntEnable(INT_UART7);
    UARTIntEnable(UART7_BASE, UART_INT_RX | UART_INT_RT);

    /* Start BIOS */
    BIOS_start();

    return (0);
}
