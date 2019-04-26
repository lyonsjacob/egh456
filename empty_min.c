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

Task_Struct taskgrStruct;
Char taskgrStack[TASKSTACKSIZE];

Hwi_Handle myHwi;
Swi_Handle mySwi;

#define buffer_size 50
char buffer[buffer_size] = " ";
int buffer_index = 0;

int print_screen = 0;

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

// GRLIB FXN
Void guiRun() {
    tContext sContext;
    tRectangle sRect;

    //
    // The FPU should be enabled because some compilers will use floating-
    // point registers, even for non-floating-point code.  If the FPU is not
    // enabled this will cause a fault.  This also ensures that floating-
    // point operations could be added to this application and would work
    // correctly and use the hardware floating-point unit.  Finally, lazy
    // stacking is enabled for interrupt handlers.  This allows floating-
    // point instructions to be used within interrupt handlers, but at the
    // expense of extra stack usage.
    //
    //FPUEnable();
    //FPULazyStackingEnable(); <-- broke it

    // Initialize the display driver.
    Kentec320x240x16_SSD2119Init(g_ui32SysClock);

    // Initialize the graphics context.
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.i16YMax = 23;

    char buffer_print[buffer_size];
    int i;
    while (1) {
        if (print_screen == 1) {
            // Clear print buffer
            for (i = 0; i < buffer_size; i++) {
                buffer_print[i] = '\0';
            }
            // Load print buffer
            for (i = 0; i < buffer_index; i++) {
                buffer_print[i] = buffer[i];
            }
            buffer_index = 0; // reset index

            GrContextForegroundSet(&sContext, ClrDarkBlue);
            GrRectFill(&sContext, &sRect);

            //
            // Put a white box around the banner.
            //
            GrContextForegroundSet(&sContext, ClrWhite);
            GrRectDraw(&sContext, &sRect);

            //
            // Put the application name in the middle of the banner.
            //
            GrContextFontSet(&sContext, &g_sFontCm20);
            GrStringDrawCentered(&sContext, buffer_print, -1,
                                 GrContextDpyWidthGet(&sContext) / 2, 8, 0);

            buffer_index = 0;
            print_screen = 0;
        }
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
        if (buffer[buffer_index] == '\r') {
            // Print to screen allow
            print_screen = 1;
        }

        // Increment buffer to allow next char to enter
        buffer_index++;
    }

    //Post Swi for follow up processing
    if (buffer[buffer_index-1] == ' ') {
        Swi_post(mySwi);
    }
}

void mySwiFunc() {
    // We want to start print from the start
    int buffer_init = buffer_index - 1;
    int index;
    while (buffer_index > 0) {
        buffer_index--;
        index = buffer_init - buffer_index; // start at 0 and count up
        UARTCharPutNonBlocking(UART0_BASE, buffer[index]);
    }
}

/*
 *  ======== main ========
 */
int main(void)
{
    Task_Params taskParams;
    Task_Params taskParams_grlib;

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

    /* Construct the grlib Task thread */
    Task_Params_init(&taskParams_grlib);
    taskParams_grlib.stackSize = TASKSTACKSIZE;
    taskParams_grlib.stack = &taskgrStack;
    Task_construct(&taskgrStruct, (Task_FuncPtr)guiRun, &taskParams_grlib, NULL);



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
