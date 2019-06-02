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
//#include <semaphore.h>

I2C_Handle      i2c;
I2C_Params      gui_params;
I2C_Params      lux_params;
I2C_Params      acc_params;
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
// Our header files:
#include "GUI.h"
#include "temp.h"
#include <MotorControl.h>
#include "i2csensors.h"
// Motor files
#include <ti/drivers/PWM.h>

#include <ti/sysbios/gates/GateMutexPri.h>

// if using GateMutex uncomment such lines and comment out the semaphore ones
UInt gateKey;
GateMutexPri_Handle gampHandle;
GateMutexPri_Params gampParams;
/* Construct a GateMutexPri object to be use as a resource lock */

uint32_t g_ui32SysClock;

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
	// add hwi disable line by line until it crashes?
    tContext sContext;
    bool bUpdate;

    // graphics
    Kentec320x240x16_SSD2119Init(g_ui32SysClock);
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);

    // screen
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

//     gui functionality
    GUI_init();
    while (1) {
        bUpdate = DateTimeDisplayGet();
        if(bUpdate) run_timer();
        WidgetMessageQueueProcess();
        System_flush();
        Task_sleep(100);
    }
}

// LUX Task Function
Void luxRun() {

    //=====CODE FOR INITIATING LIGH REG========
    initLux();

    while (1) {
        readLux();

        System_flush();
        Task_sleep(100);
    }
}

// ACC Task Function
Void accRun() {

    //======CODE FOR CONFIGURING ACC=============
    initAcc();
    System_flush();
    while (1) {
        readAcc();

        System_flush();
        Task_sleep(100);
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
    gui_params.stackSize = 1024;
    gui_params.priority = 1;
    Task_create((Task_FuncPtr)guiRun, &gui_params, NULL);
}

void setup_lux_task(){
    Task_Params lux_params;
    Task_Params_init(&lux_params);
    lux_params.stackSize = 512;
    lux_params.priority = 2;
    Task_create((Task_FuncPtr)luxRun, &lux_params, NULL);
}

void setup_acc_task(){
    Task_Params acc_params;
    Task_Params_init(&acc_params);
    acc_params.stackSize = 512;
    acc_params.priority = 5;
    Task_create((Task_FuncPtr)accRun, &acc_params, NULL);
}

void setupI2C2( void )
{
    //=======CREATE I2C FOR USAGE===============

    I2C_Params_init(&lux_params);
    lux_params.bitRate = I2C_400kHz;
    i2c = I2C_open(Board_I2C_LUX, &lux_params);
    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }
    else {
        //System_printf("I2C Initialized!\n");
    }
}

void setupMutexPri()
{
    GateMutexPri_Params_init(&gampParams);
    gampHandle = GateMutexPri_create(&gampParams, NULL);
    if (gampHandle == NULL) {
        System_abort("Gate Mutex Pri create failed");
    }
}

int main(void)
{
    /* Call board init functions */

    Board_initGeneral();
    Board_initGPIO();
    Board_initUART();
    Board_initI2C();
    Board_initPWM();
    MotorSetup();



    // Set the clocking to run directly from the crystal at 120MHz.
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN
                   | SYSCTL_USE_PLL |SYSCTL_CFG_VCO_480), 120000000);

    setupMutexPri();

    setupI2C2();



    // Setup tasks
    setup_temp();
    setup_gui_task();
    setup_lux_task();
    setup_acc_task();



    // Setup Hwis
    setup_adc_hwi();

    // Setup Swis

    /* Start BIOS */
    BIOS_start();

    return (0);
}
