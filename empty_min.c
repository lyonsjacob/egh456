/* XDCtools Header files */
#include <xdc/std.h>

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

// UART FILES
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
//#include "driverlib/rom.h"
//#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"

// GRLIB FILES
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "drivers/Kentec320x240x16_ssd2119_spi.h"
#include "drivers/touch.h"


/* Board Header file */
#include "Board.h"
#include <inc/hw_ints.h>

#define TASKSTACKSIZE  4024

uint32_t g_ui32SysClock;

Task_Struct task0Struct;
Char task0Stack[TASKSTACKSIZE];

Hwi_Handle myHwi;
Swi_Handle mySwi;

char buffer[50] = " ";
int buffer_index = 0;

/*
 *  ======== heartBeatFxn ========
 *  Toggle the Board_LED0. The Task_sleep is determined by arg0 which
 *  is configured for the heartBeat Task instance.
 */
Void heartBeatFxn(UArg arg0, UArg arg1)
{
    while (1) {
        Task_sleep((unsigned int)arg0);
        GPIO_toggle(Board_LED0);
    }
}

Void myHwiFunc() {
    uint32_t ui32Status;

    //
    // Get the interrrupt status.
    //
    ui32Status = UARTIntStatus(UART0_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    UARTIntClear(UART0_BASE, ui32Status);

    //
    // Loop while there are characters in the receive FIFO.
    //
    while(UARTCharsAvail(UART0_BASE))
    {
        //
        // Read the next character from the UART and write it back to the UART.
        //
        buffer[buffer_index] = (char)UARTCharGetNonBlocking(UART0_BASE);

        // If enter pressed
        //if (buffer[buffer_index] == '\r') {
        //    print_screen = 1;
        //}

        buffer_index++;
    }

    //Post Swi for follow up processing
    if (buffer[buffer_index-1] == ' ') {
        Swi_post(mySwi);
    }
}

void mySwiFunc() {
    int buffer_init = buffer_index - 1;
    int index;
    while (buffer_index > 0) {
        buffer_index--;
        index = buffer_init - buffer_index;
        UARTCharPutNonBlocking(UART0_BASE, buffer[index]);
    }
}

/*
 *  ======== main ========
 */
int main(void)
{
    Task_Params taskParams;

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

    /* Construct heartBeat Task  thread */
    Task_Params_init(&taskParams);
    taskParams.arg0 = 1000;
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.stack = &task0Stack;
    Task_construct(&task0Struct, (Task_FuncPtr)heartBeatFxn, &taskParams, NULL);

    // Setup hardware Hwi
    Hwi_Params hwiParams;
    Error_Block hwi_eb;
    Error_init(&hwi_eb);

    Hwi_Params_init(&hwiParams);
    int id = INT_UART0_TM4C123;
    myHwi = Hwi_create(id, (Hwi_FuncPtr)myHwiFunc, &hwiParams, &hwi_eb);

    if (myHwi == NULL) System_abort("Hwi create failed");

    // Setup Software Swi
    Swi_Params swiParams;
    Error_Block swi_eb;
    Error_init(&swi_eb);

    Swi_Params_init(&swiParams);
    mySwi = Swi_create((Swi_FuncPtr)mySwiFunc, &swiParams, &swi_eb);

    if (mySwi == NULL) {
        System_abort("Swi create failed");
    }

    //-------------------- Setup UART --------------
    // Set the clocking to run directly from the crystal at 120MHz.
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);

    // Enable the peripherals used by this example.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    //IntMasterEnable(); // Enable processor interrupts

    // Set GPIO A0 and A1 as UART pins.
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    UARTConfigSetExpClk(UART0_BASE, g_ui32SysClock, 115200,
                           (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                            UART_CONFIG_PAR_NONE));

    // Enable the UART interrupt.
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

    /* Turn on user LED  */
    GPIO_write(Board_LED0, Board_LED_ON);

    /* Start BIOS */
    BIOS_start();

    return (0);
}
