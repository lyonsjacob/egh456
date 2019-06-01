/* C header files */
#include <stdio.h>
#include <math.h>
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
#include <ti/sysbios/knl/Task.h>//int main(void)
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

#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

/* TI-RTOS Header files */
// #include <ti/drivers/EMAC.h>
#include <ti/drivers/GPIO.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/drivers/I2C.h>
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
//#include "driverlib/pin_map.h"
//#include "driverlib/rom.h"
//#include "driverlib/rom_map.h"
//#include "driverlib/uart.h"

// GRLIB FILES
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "drivers/Kentec320x240x16_ssd2119_spi.h"
#include "drivers/touch.h"
#include "driverlib/udma.h"
#include "driverlib/fpu.h"

#include "driverlib/hibernate.h"
#include "inc/hw_hibernate.h"

// Motor files
#include <ti/drivers/PWM.h>

// Our header files:
#include "GUI.h"
#include "i2csensors.h"
//#include "ACC.h"
#include <MotorControl.h>

#define TASKSTACKSIZE  4096*2

uint32_t g_ui32SysClock;

Task_Struct gui_struct;
Char gui_stack[TASKSTACKSIZE];

I2C_Handle      i2c;
I2C_Params      gui_params;
I2C_Transaction i2cTransaction;

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

// GUI Task Function
Void guiRun() {
    tContext sContext;
    bool bUpdate;

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


    //=====CODE FOR INITIATING LIGH REG========

    initLux();

    //======CODE FOR CONFIGURING ACC=============

    initAcc();

    int count = 0;

    while (1) {

        if(count == 500){
            count = 0;
            readLux();
            readAcc();
        }

        bUpdate = DateTimeDisplayGet();
        if(bUpdate) run_timer();
        WidgetMessageQueueProcess();
        System_flush();
        count++;
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

void setupI2C2( void )
{
    //=======CREATE I2C FOR USAGE===============

    I2C_Params_init(&gui_params);
    gui_params.bitRate = I2C_400kHz;
    i2c = I2C_open(Board_I2C_LUX, &gui_params);
    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }
    else {
        System_printf("I2C Initialized!\n");
    }
}

int main(void)
{
    /* Call board init functions */

    Board_initGeneral();
    Board_initGPIO();
    Board_initI2C();

    Board_initPWM();
    MotorSetup();



    // Set the clocking to run directly from the crystal at 120MHz.
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN
                     | SYSCTL_USE_PLL |SYSCTL_CFG_VCO_480), 120000000);


    setupI2C2();
    // Setup tasks
    setup_gui_task();


    // Setup Hwis
    setup_adc_hwi();


    // Setup Swis


    /* Start BIOS */
    BIOS_start();

    return (0);
}
