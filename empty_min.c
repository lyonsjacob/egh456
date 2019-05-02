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
#include "grlib/slider.h"
#include "driverlib/udma.h"
#include "utils/ustdlib.h"
#include "driverlib/fpu.h"
#include "images.h"




/* Board Header file */
#include "Board.h"
#include <inc/hw_ints.h>

#define TASKSTACKSIZE  4024

uint32_t g_ui32SysClock;


/* - - - - - - GUI FUNCTIONALITY - - - - - - -*/

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



Task_Struct gui_struct;
Char gui_stack[TASKSTACKSIZE];

extern tCanvasWidget tabs[];
//void paintMotorControl(tWidget *psWidget, tContext *psContext);
void paintAnalytics(tWidget *psWidget, tContext *psContext);
void OnSliderChange(tWidget *psWidget, int32_t i32Value);

/*
Canvas(motorControl, tabs, 0, 0, &g_sKentec320x240x16_SSD2119, 0,
       24, 320, 166, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, paintMotorControl);*/
Canvas(analytics, tabs, 0, 0, &g_sKentec320x240x16_SSD2119, 0,
       24, 320, 166, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, paintAnalytics);

Canvas(g_sSliderValueCanvas, tabs, 0, 0,
       &g_sKentec320x240x16_SSD2119, 210, 30, 60, 40,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, 0, ClrSilver,
       &g_sFontCm24, "50%",
       0, 0);

tSliderWidget sliders[] = {
    SliderStruct(tabs, sliders + 1, 0,
                &g_sKentec320x240x16_SSD2119, 5, 115, 220, 30, 0, 100, 25,
                (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                 SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                &g_sFontCm20, "25%", 0, 0, OnSliderChange),
    SliderStruct(tabs, sliders + 2, 0,
                &g_sKentec320x240x16_SSD2119, 5, 155, 220, 25, 0, 100, 25,
                (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                 SL_STYLE_TEXT),
                ClrWhite, ClrBlueViolet, ClrSilver, ClrBlack, 0,
                &g_sFontCm18, "Foreground Text Only", 0, 0, OnSliderChange),
    SliderStruct(tabs, sliders + 3, 0,
                &g_sKentec320x240x16_SSD2119, 240, 70, 26, 110, 0, 100, 50,
                (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_VERTICAL |
                 SL_STYLE_OUTLINE | SL_STYLE_LOCKED), ClrDarkGreen,
                 ClrDarkRed, ClrSilver, 0, 0, 0, 0, 0, 0, 0),
    SliderStruct(tabs, sliders + 4, 0,
                &g_sKentec320x240x16_SSD2119, 280, 30, 30, 150, 0, 100, 75,
                (SL_STYLE_IMG | SL_STYLE_BACKG_IMG | SL_STYLE_VERTICAL |
                SL_STYLE_OUTLINE), 0, ClrBlack, ClrSilver, 0, 0, 0,
                0, g_pui8GettingHotter28x148, g_pui8GettingHotter28x148Mono,
                OnSliderChange),
    SliderStruct(tabs, sliders + 5, 0,
                &g_sKentec320x240x16_SSD2119, 5, 30, 195, 37, 0, 100, 50,
                SL_STYLE_IMG | SL_STYLE_BACKG_IMG, 0, 0, 0, 0, 0, 0,
                0, g_pui8GreenSlider195x37, g_pui8RedSlider195x37,
                OnSliderChange),
    SliderStruct(tabs, &g_sSliderValueCanvas, 0,
                &g_sKentec320x240x16_SSD2119, 5, 80, 220, 25, 0, 100, 50,
                (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_TEXT |
                 SL_STYLE_BACKG_TEXT | SL_STYLE_TEXT_OPAQUE |
                 SL_STYLE_BACKG_TEXT_OPAQUE),
                ClrBlue, ClrYellow, ClrSilver, ClrYellow, ClrBlue,
                &g_sFontCm18, "Text in both areas", 0, 0,
                OnSliderChange),

    //add extra canvases here as slider struct.
};
#define SLIDER_TEXT_VAL_INDEX   0
#define SLIDER_LOCKED_INDEX     2
#define SLIDER_CANVAS_VAL_INDEX 4

#define NUM_SLIDERS (sizeof(g_psSliders) / sizeof(g_psSliders[0]))

tCanvasWidget tabs[] = {
    CanvasStruct(0, 0, sliders, &g_sKentec320x240x16_SSD2119, 0,
                 24, 320, 166, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, &analytics, &g_sKentec320x240x16_SSD2119, 0, 24,
                 320, 166, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
};


void paintAnalytics(tWidget *psWidget, tContext *psContext){
    //function here
}

void OnSliderChange(tWidget *psWidget, int32_t i32Value){

    static char pcCanvasText[5];
    static char pcSliderText[5];

    //
    // Is this the widget whose value we mirror in the canvas widget and the
    // locked slider?
    //
    if(psWidget == (tWidget *)&sliders[SLIDER_CANVAS_VAL_INDEX])
    {
        //
        // Yes - update the canvas to show the slider value.
        //
        usprintf(pcCanvasText, "%3d%%", i32Value);
        CanvasTextSet(&g_sSliderValueCanvas, pcCanvasText);
        WidgetPaint((tWidget *)&g_sSliderValueCanvas);

        //
        // Also update the value of the locked slider to reflect this one.
        //
        SliderValueSet(&sliders[SLIDER_LOCKED_INDEX], i32Value);
        WidgetPaint((tWidget *)&sliders[SLIDER_LOCKED_INDEX]);
    }

    if(psWidget == (tWidget *)&sliders[SLIDER_TEXT_VAL_INDEX])
    {
        //
        // Yes - update the canvas to show the slider value.
        //
        usprintf(pcSliderText, "%3d%%", i32Value);
        SliderTextSet(&sliders[SLIDER_TEXT_VAL_INDEX], pcSliderText);
        WidgetPaint((tWidget *)&sliders[SLIDER_TEXT_VAL_INDEX]);
    }
}
/* - - - - - END GUI FUNCTIONALITY - - - - - - */


/* - - - - - - GUI TASK FUNCTIONALITY - - - - - */

void RTOSTouchScreenInit(uint32_t ui32SysClock){
    //check touch.c screen init
}


uint32_t disp_tab;
// GRLIB FXN
Void guiRun() {
    tContext sContext;
    tRectangle sRect;

    FPUEnable();
    FPULazyStackingEnable();

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
    GrContextForegroundSet(&sContext, ClrDarkBlue);
    GrRectFill(&sContext, &sRect);

    GrContextForegroundSet(&sContext, ClrWhite);
    GrRectDraw(&sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&sContext, &g_sFontCm20);
    GrStringDrawCentered(&sContext, "Motor Control", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 8, 0);

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
    //TouchScreenInit(g_ui32SysClock);
    //TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Add the first panel to the widget tree.
    //
    disp_tab = 0;
    WidgetAdd(WIDGET_ROOT, (tWidget *)tabs);

    //
    // Issue the initial paint request to the widgets.
    //
    WidgetPaint(WIDGET_ROOT);

    while (1) {
        WidgetMessageQueueProcess();
    }
}

/* - - - - - - END GUI TASK FUNCTIONALITY - - - - - */

/*
 *  ======== main ========
 */
int main(void)
{
    Task_Params gui_params;

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


    /* Gui task */
    Task_Params_init(&gui_params);
    gui_params.stackSize = TASKSTACKSIZE;
    gui_params.stack = &gui_stack;
    Task_construct(&gui_struct, (Task_FuncPtr)guiRun, &gui_params, NULL);

    // Set the clocking to run directly from the crystal at 120MHz.
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN
                     | SYSCTL_USE_PLL |SYSCTL_CFG_VCO_480), 120000000);

    /* Turn on user LED  */
    GPIO_write(Board_LED0, Board_LED_ON);

    /* Start BIOS */
    BIOS_start();

    return (0);
}
