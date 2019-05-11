/* XDCtools Header files */
#include <xdc/std.h>
#include <math.h>

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
#include "grlib/slider.h"
#include "driverlib/udma.h"
#include "utils/ustdlib.h"
#include "driverlib/fpu.h"
#include "images.h"
#include "driverlib/sysctl.h"
#include "grlib/pushbutton.h"
#include "grlib/container.h"



/* Board Header file */
#include "Board.h"
#include <inc/hw_ints.h>

#define TASKSTACKSIZE  4024

uint32_t g_ui32SysClock;


//GLOBAL VARIABLES
uint16_t speed,current,acc,temp;
uint8_t sec,min,hour; //timer vars



//*****************************************************************************
//*****************************************************************************
// GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI GUI
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//
// The DMA control structure table.
//
//*****************************************************************************
#ifdef ewarm
#pragma data_alignment=1024
tDMAControlTable psDMAControlTable[64];
#elif defined(ccs)
#pragma DATA_ALIGN(psDMAControlTable, 1024)
tDMAControlTable psDMAControlTable[64];
#else
tDMAControlTable psDMAControlTable[64] __attribute__ ((aligned(1024)));
#endif





extern tCanvasWidget tabs[];
void paintMotorControl(tWidget *psWidget, tContext *psContext);
void paintAnalytics(tWidget *psWidget, tContext *psContext);
void OnSliderChange(tWidget *psWidget, int32_t i32Value);
void startMotor(), stopMotor(), onNext(), onBack();

uint32_t disp_tab;

Canvas(analytics, tabs+1, 0, 0, &g_sKentec320x240x16_SSD2119, 0,
       24, 320, 166, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, paintAnalytics);

//*****************************************************************************
//
// Static Banners (top, Bottom, time, day or night)
//
//*****************************************************************************

//BACK AND FOWARD BUTTONS
RectangularButton(nextButton, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 270, 190,
                  50, 50, PB_STYLE_IMG | PB_STYLE_TEXT, ClrBlack, ClrBlack, 0,
                  ClrSilver, &g_sFontCm20, "+", g_pui8Blue50x50,
                  g_pui8Blue50x50Press, 0, 0, onNext);

RectangularButton(backButton, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 0, 190,
                  50, 50, PB_STYLE_FILL, ClrBlack, ClrBlack, 0, ClrSilver,
                  &g_sFontCm20, "-", g_pui8Blue50x50, g_pui8Blue50x50Press, 0, 0,
                  onBack);

//START AND STOP BUTTONS
RectangularButton(startButton, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 110, 190,
                  100, 50, PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT, ClrLimeGreen,
                  ClrLimeGreen, ClrWhite, ClrWhite, &g_sFontCm20, "Start", 0,
                  0, 0, 0, startMotor);
RectangularButton(stopButton, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 110, 190,
                  100, 50, PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT, ClrDarkSalmon,
                  ClrDarkSalmon, ClrWhite, ClrWhite, &g_sFontCm20, "Stop", 0,
                  0, 0, 0, stopMotor);
//TITLE OF PANEL
char * namesOfPanels[] = {"  Motor Control  ", "  Power Usage  ","  Light Level  ", "  Motor Temperature  ","  Acceleration  ","  Motor Speed  "};
Canvas(titleNames, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 70, 0, 200, 30,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, 0, 0, ClrWhite,
       &g_sFontCm20, 0, 0, 0);
int NUM_PANELS = 6;

//DAY OR NIGHT FUNCTIONALITY
char * dayOrNightNames[] = { "Day", "Night" };
Canvas(dayOrNight, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 270, 0, 50, 30,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, 0, 0, ClrSilver,
       &g_sFontCm14, 0, 0, 0);

//TIMER
Canvas(dispTime, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 5, 0, 50, 30,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, 0, 0, ClrSilver,
       &g_sFontCm14, 0, 0, 0);

//*****************************************************************************
//
// Motor Control Widgets --> sliders = the canvas
//
//*****************************************************************************

Canvas(motorControl, tabs, 0, 0, &g_sKentec320x240x16_SSD2119, 0,
       24, 320, 166, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, paintMotorControl);

tSliderWidget sliders[] = {
   SliderStruct(tabs, sliders+1, 0,
               &g_sKentec320x240x16_SSD2119, 170, 60, 130, 30, 0, 100, 0,
               (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
               ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
               &g_sFontCm14, "0 mps2", 0, 0, OnSliderChange),
   SliderStruct(tabs, sliders+2, 0,
               &g_sKentec320x240x16_SSD2119, 20, 140, 130, 30, 800, 1200, 0,
               (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
               ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
               &g_sFontCm14, "800 mA", 0, 0, OnSliderChange),
   SliderStruct(tabs, sliders+3, 0,
               &g_sKentec320x240x16_SSD2119, 170, 140, 130, 30, 30, 60, 0,
               (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
               ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
               &g_sFontCm14, "30 celsius", 0, 0, OnSliderChange),
    SliderStruct(tabs, &motorControl,0,
                &g_sKentec320x240x16_SSD2119, 20, 60, 130, 30, 0, 3000, 0,
                (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                 SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                &g_sFontCm14, "0 rpm", 0, 0, OnSliderChange),
};

tCanvasWidget tabs[] = {
    CanvasStruct(0, 0, sliders, &g_sKentec320x240x16_SSD2119, 0,
                 24, 320, 166, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, &analytics, &g_sKentec320x240x16_SSD2119, 0, 24,
                 320, 166, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
};


void paintAnalytics(tWidget *psWidget, tContext *psContext){
    GrContextFontSet(psContext, &g_sFontCm16);
    GrContextForegroundSet(psContext, ClrSilver);

}

void paintMotorControl(tWidget *psWidget, tContext *psContext){

    GrContextFontSet(psContext, &g_sFontCm16);
    GrContextForegroundSet(psContext, ClrSilver);


    GrStringDraw(psContext, "Set Speed", -1, 54, 40, 0);
    GrStringDraw(psContext, "Max Accelerometer", -1, 176, 40, 0);
    GrStringDraw(psContext, "Max Current", -1, 50, 120, 0);
    GrStringDraw(psContext, "Max Temperature", -1, 180, 120, 0);
}

void OnSliderChange(tWidget *psWidget, int32_t i32Value){

    static char pcSliderText[10], pcSliderText1[10];
    static char pcSliderText2[10], pcSliderText3[10];

    //Accelerometer changed
    if(psWidget == (tWidget *)&sliders[0]){
        acc = i32Value; // 1 mps^2? [0 -> 100]
        usprintf(pcSliderText, "%d mps2", acc);
        SliderTextSet(&sliders[0], pcSliderText);
        WidgetPaint((tWidget *)&sliders[0]);
    }

    //Current changed
    if(psWidget == (tWidget *)&sliders[1]){
        current = i32Value; // STEPS 10 mA [800 -> 1200]
        usprintf(pcSliderText1, "%d mA", current);
        SliderTextSet(&sliders[1], pcSliderText1);
        WidgetPaint((tWidget *)&sliders[1]);
    }

    //Temp changed
    if(psWidget == (tWidget *)&sliders[2]){
        temp = i32Value;//(uint16_t)((i32Value+90)/3); // [30 -> 63]
        usprintf(pcSliderText2, "%d celsius", temp);
        SliderTextSet(&sliders[2], pcSliderText2);
        WidgetPaint((tWidget *)&sliders[2]);
    }

    //Speed changed
    if(psWidget == (tWidget *)&sliders[3]){
        speed = i32Value; // STEPS OF 30 RPM [0 -> 3000]
        usprintf(pcSliderText3, "%d rpm", speed);
        SliderTextSet(&sliders[3], pcSliderText3);
        WidgetPaint((tWidget *)&sliders[3]);
    }
}

void startMotor(){
    GPIO_write(Board_LED0, Board_LED_ON);

    WidgetRemove((tWidget *)&startButton);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&stopButton);
    WidgetPaint((tWidget *)&stopButton);
}

void stopMotor(){
    GPIO_write(Board_LED0, Board_LED_OFF);

    WidgetRemove((tWidget *)&stopButton);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&startButton);
    WidgetPaint((tWidget *)&startButton);
}

void onNext(){

    if(disp_tab==1) return;

    WidgetRemove((tWidget *)(tabs + disp_tab));

    disp_tab++;

    WidgetAdd(WIDGET_ROOT, (tWidget *)(tabs + disp_tab));
    WidgetPaint((tWidget *)(tabs + disp_tab));

    CanvasTextSet(&titleNames, namesOfPanels[disp_tab]);
    WidgetPaint((tWidget *)&titleNames);


    if(tabs == 1){
        //DISPLAYING OF THE BUTTONS
        PushButtonImageOff(&nextButton);
        PushButtonTextOff(&nextButton);
        PushButtonFillOn(&nextButton);
        WidgetPaint((tWidget *)&nextButton);
    }

    if(tabs == (NUM_PANELS - 1)){
        PushButtonImageOn(&backButton);
        PushButtonTextOn(&backButton);
        PushButtonFillOff(&backButton);
        WidgetPaint((tWidget *)&backButton);
    }
}

void onBack(){

    if(disp_tab==0) return;

    WidgetRemove((tWidget *)(tabs + disp_tab));

    disp_tab--;

    WidgetAdd(WIDGET_ROOT, (tWidget *)(tabs + disp_tab));
    WidgetPaint((tWidget *)(tabs + disp_tab));

    CanvasTextSet(&titleNames, namesOfPanels[disp_tab]);
    WidgetPaint((tWidget *)&titleNames);

    if(tabs == 0){
        //DISPLAYING OF THE BUTTONS
        PushButtonImageOff(&backButton);
        PushButtonTextOff(&backButton);
        PushButtonFillOn(&backButton);
        WidgetPaint((tWidget *)&backButton);
    }

    if(tabs == (NUM_PANELS - 2)){
        PushButtonImageOn(&nextButton);
        PushButtonTextOn(&nextButton);
        PushButtonFillOff(&nextButton);
        WidgetPaint((tWidget *)&nextButton);
    }
}

void changeDisplayToNight(){
    CanvasTextSet(&dayOrNight, dayOrNightNames[1]);
    GPIO_write(Board_LED1, Board_LED_ON);
}

void changeDisplayToDay(){
    CanvasTextSet(&dayOrNight, dayOrNightNames[0]);
    GPIO_write(Board_LED1, Board_LED_OFF);
}

void changeDisplayDate(){
    static char timer[10];
    usprintf(timer, "%02d:%02d:%02d",hour,min,sec);
    CanvasTextSet(&dispTime, timer);
}
/* - - - - - END GUI FUNCTIONALITY - - - - - - */


/* - - - - - - GUI TASK FUNCTIONALITY - - - - - */


Task_Struct gui_struct;
Char gui_stack[TASKSTACKSIZE];

// GRLIB FXN
Void guiRun() {
    tContext sContext;
    //tRectangle sRect;

    FPUEnable();
    FPULazyStackingEnable();

    // Initialize the display driver.
    Kentec320x240x16_SSD2119Init(g_ui32SysClock);

    // Initialize the graphics context.
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Configure and enable uDMA
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(10);
    uDMAControlBaseSet(&psDMAControlTable[0]);
    uDMAEnable();

    //
    // Initialize the touch screen driver and have it route its messages to the
    // widget tree.
    //
    TouchScreenInit(g_ui32SysClock);
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Add the first panel to the widget tree.
    //
    disp_tab=0, sec=0, hour=0, min=0;
    WidgetAdd(WIDGET_ROOT, (tWidget *)tabs);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&startButton);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&titleNames);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&nextButton);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&backButton);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&dayOrNight);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&dispTime);

    // get this working...
    changeDisplayDate();

    // run get lux here and change this line
    changeDisplayToNight();//changeDisplayToDay();

    CanvasTextSet(&titleNames, namesOfPanels[0]);
    WidgetPaint(WIDGET_ROOT);

    while (1) {
        WidgetMessageQueueProcess();
    }
}

//*****************************************************************************
//*****************************************************************************
// END GUI END GUI END GUI END GUI END GUI END GUI END GUI END GUI END GUI
//*****************************************************************************
//*****************************************************************************





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


    /* Start BIOS */
    BIOS_start();

    return (0);
}
