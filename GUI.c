#include <stdint.h>
#include <stdbool.h>

#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "drivers/Kentec320x240x16_ssd2119_spi.h"
#include "drivers/touch.h"
#include "grlib/slider.h"
#include "utils/ustdlib.h"
#include "images.h"
#include "driverlib/sysctl.h"
#include "grlib/pushbutton.h"
#include "grlib/container.h"

#include "driverlib/hibernate.h"
#include "inc/hw_hibernate.h"

#include "main.h"




uint16_t speed,current,acc,temp;    // user set vars
uint32_t sec,min,hour;              // timer vars
uint32_t disp_tab;                  // which tab

extern tCanvasWidget tabs[];
void paintMotorControl(tWidget *psWidget, tContext *psContext);
void paintGraph(tWidget *psWidget, tContext *psContext);
void OnSliderChange(tWidget *psWidget, int32_t i32Value);
void startMotor(), stopMotor(), onNext(), onBack();
void changeDisplayDate();



/* - - - - - - CALANDER TIME - - - - - - */
bool DateTimeGet(struct tm *sTime){

    HibernateCalendarGet(sTime);

    if(((sTime->tm_sec < 0) || (sTime->tm_sec > 59)) ||
       ((sTime->tm_min < 0) || (sTime->tm_min > 59)) ||
       ((sTime->tm_hour < 0) || (sTime->tm_hour > 23)))
    {
        return false;
    }

    return true;
}

// SET THE TIME HERE
void DateTimeDefaultSet(int setHour, int setMin){
    min = setMin;
    hour = setHour;

    struct tm sTime;

    HibernateCalendarGet(&sTime);

    sTime.tm_hour = hour;
    sTime.tm_min = min;

    HibernateCalendarSet(&sTime);
}

bool DateTimeDisplayGet(){
    static uint32_t ui32SecondsPrev = 0xFF;
    struct tm sTime;

    if(DateTimeGet(&sTime) == false) return false;

    if(ui32SecondsPrev == sTime.tm_sec) return false;

    ui32SecondsPrev = sTime.tm_sec;

    hour = sTime.tm_hour;
    min = sTime.tm_min;
    sec = sTime.tm_sec;

    return true;
}

void run_timer(){
    changeDisplayDate();
}


/* - - - - - GUI FUNCTIONALITY - - - - - */

Canvas(powerUsage, tabs+1, 0, 0, &g_sKentec320x240x16_SSD2119, 0,
       30, 320, 160, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, paintGraph);
Canvas(lightLevel, tabs+2, 0, 0, &g_sKentec320x240x16_SSD2119, 0,
       30, 320, 160, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, paintGraph);
Canvas(motorTemp, tabs+3, 0, 0, &g_sKentec320x240x16_SSD2119, 0,
       30, 320, 160, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, paintGraph);
Canvas(accelData, tabs+4, 0, 0, &g_sKentec320x240x16_SSD2119, 0,
       30, 320, 160, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, paintGraph);
Canvas(motorSpeed, tabs+5, 0, 0, &g_sKentec320x240x16_SSD2119, 0,
       30, 320, 160, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, paintGraph);

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
char * namesOfPanels[] = {"    Motor Control    ","    Power Usage     ","    Light Level    ",
                          "  Motor Temperature  ","   Accelerometer    ","    Motor Speed    "};
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
       30, 320, 160, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, paintMotorControl);

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
                     30, 320, 160, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
        CanvasStruct(0, 0, &powerUsage, &g_sKentec320x240x16_SSD2119, 0, 30,
                     320, 160, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
        CanvasStruct(0, 0, &lightLevel, &g_sKentec320x240x16_SSD2119, 0, 30,
                     320, 160, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
        CanvasStruct(0, 0, &motorTemp, &g_sKentec320x240x16_SSD2119, 0, 30,
                     320, 160, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
        CanvasStruct(0, 0, &accelData, &g_sKentec320x240x16_SSD2119, 0, 30,
                     320, 160, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
        CanvasStruct(0, 0, &motorSpeed, &g_sKentec320x240x16_SSD2119, 0, 30,
                     320, 160, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
};


void paintGraph(tWidget *psWidget, tContext *psContext){
    GrContextFontSet(psContext, &g_sFontCm16);
    GrContextForegroundSet(psContext, ClrSilver);

    /*********** GRPHING IMPLEMENTATION HERE ************/

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
    toggleLight(0,1);

    WidgetRemove((tWidget *)&startButton);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&stopButton);
    WidgetPaint((tWidget *)&stopButton);
}

void stopMotor(){
    toggleLight(0,0);

    WidgetRemove((tWidget *)&stopButton);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&startButton);
    WidgetPaint((tWidget *)&startButton);
}

void onNext(){

    if(disp_tab == (NUM_PANELS - 1)) return;

    WidgetRemove((tWidget *)(tabs + disp_tab));

    disp_tab++;

    WidgetAdd(WIDGET_ROOT, (tWidget *)(tabs + disp_tab));
    WidgetPaint((tWidget *)(tabs + disp_tab));

    CanvasTextSet(&titleNames, namesOfPanels[disp_tab]);
    WidgetPaint((tWidget *)&titleNames);


    if(disp_tab == 1){
        PushButtonImageOn(&backButton);
        PushButtonTextOn(&backButton);
        PushButtonFillOff(&backButton);
        WidgetPaint((tWidget *)&backButton);
    }

    if(disp_tab == (NUM_PANELS - 1)){
        PushButtonImageOff(&nextButton);
        PushButtonTextOff(&nextButton);
        PushButtonFillOn(&nextButton);
        WidgetPaint((tWidget *)&nextButton);
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

    if(disp_tab == 0){
        PushButtonImageOff(&backButton);
        PushButtonTextOff(&backButton);
        PushButtonFillOn(&backButton);
        WidgetPaint((tWidget *)&backButton);
    }

    if(disp_tab == (NUM_PANELS - 2)){
        PushButtonImageOn(&nextButton);
        PushButtonTextOn(&nextButton);
        PushButtonFillOff(&nextButton);
        WidgetPaint((tWidget *)&nextButton);
    }
}

void changeDisplayToNight(){
    toggleLight(1,1);

    CanvasTextSet(&dayOrNight, dayOrNightNames[1]);
    WidgetPaint((tWidget *)&dayOrNight);
}

void changeDisplayToDay(){
    toggleLight(1,0);
    CanvasTextSet(&dayOrNight, dayOrNightNames[0]);
    WidgetPaint((tWidget *)&dayOrNight);
}

void changeDisplayDate(){
    static char timer[10];
    usprintf(timer, "%02d:%02d:%02d",hour,min,sec);
    CanvasTextSet(&dispTime, timer);
    WidgetPaint((tWidget *)&dispTime);
}
/* - - - - - END GUI FUNCTIONALITY - - - - - - */


/* - - - - - - GUI TASK FUNCTIONALITY - - - - - */
void GUI_init(){
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
}

//get set variables
int getUserSetSpeed()           {   return  speed;      }
int getUserSetAccelerometer()   {   return  acc;        }
int getUserSetCurrent()         {   return  current;    }
int getUserSetTemperature()     {   return  temp;       }





