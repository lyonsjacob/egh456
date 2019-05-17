#include <stdint.h>
#include <stdbool.h>

#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/checkbox.h"
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

#include "MotorControl.h"
#include "main.h"


struct Analytic {
    uint16_t value[13];
    uint8_t time;
    bool draw;
};

uint16_t speed,current,acc,temp;    // user set vars
uint32_t sec,min,hour;              // timer vars
uint32_t disp_tab;                  // which tab
bool motorOn = false;

struct Analytic variables[5];


extern tCanvasWidget tabs[];
void paintMotorControl(tWidget *psWidget, tContext *psContext);
void paintGraph(tWidget *psWidget, tContext *psContext);
void OnSliderChange(tWidget *psWidget, int32_t i32Value);
void startMotor(), stopMotor(), onNext(), onBack();
void changeDisplayDate(), turnOnGraphVariable();
void drawAllAnalytics();



//*****************************************************************************
//
// TIMING FUNCTIONALITY
//
//*****************************************************************************

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

    if(motorOn) setRPM(speed);
    drawAllAnalytics();

    ui32SecondsPrev = sTime.tm_sec;

    hour = sTime.tm_hour;
    min = sTime.tm_min;
    sec = sTime.tm_sec;

    return true;
}

void run_timer(){
    changeDisplayDate();
}


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
int NUM_PANELS = 2;

char * namesOfPanels[] = {"    Motor Control    ","    Motor Analytics     "};
Canvas(titleNames, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 70, 0, 200, 30,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, 0, 0, ClrWhite,
       &g_sFontCm20, 0, 0, 0);

//DAY OR NIGHT FUNCTIONALITY
char * dayOrNightNames[] = { "Day", "Night" };
Canvas(dayOrNight, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 270, 0, 50, 30,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, 0, 0, ClrSilver,
       &g_sFontCm14, 0, 0, 0);

//TIMER
Canvas(dispTime, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 5, 0, 50, 30,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, 0, 0, ClrSilver,
       &g_sFontCm14, 0, 0, 0);

//DYNAMIC SPEED
Canvas(dispSpeed, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 94, 34, 50, 30,
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


//*****************************************************************************
//
// Analytics widgets
//
//*****************************************************************************

Canvas(analytics, tabs+1, 0, 0, &g_sKentec320x240x16_SSD2119, 0,
       30, 320, 160, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, paintGraph);

tCheckBoxWidget set_variables[] =
{
        CheckBoxStruct(tabs + 1, set_variables + 1, 0, &g_sKentec320x240x16_SSD2119,
                       10, 30, 60, 20, CB_STYLE_TEXT, 12, 0, ClrLightGreen, ClrLightGreen, &g_sFontCm12,
                       "Power", 0, turnOnGraphVariable),
        CheckBoxStruct(tabs + 1, set_variables + 2, 0, &g_sKentec320x240x16_SSD2119,
                       70, 30, 60, 20, CB_STYLE_TEXT, 12, 0, ClrLightGoldenrodYellow, ClrLightGoldenrodYellow, &g_sFontCm12,
                      "Light", 0, turnOnGraphVariable),
        CheckBoxStruct(tabs + 1, set_variables + 3, 0, &g_sKentec320x240x16_SSD2119,
                       130, 30, 60, 20, CB_STYLE_TEXT, 12, 0, ClrOrange, ClrOrange, &g_sFontCm12,
                       "Temp", 0, turnOnGraphVariable),
        CheckBoxStruct(tabs + 1, set_variables + 4, 0, &g_sKentec320x240x16_SSD2119,
                       190, 30, 60, 20, CB_STYLE_TEXT, 12, 0, ClrCyan, ClrCyan, &g_sFontCm12,
                       "Accel", 0, turnOnGraphVariable),
        CheckBoxStruct(tabs + 1, &analytics, 0, &g_sKentec320x240x16_SSD2119,
                       250, 30, 60, 20, CB_STYLE_TEXT, 12, 0, ClrSnow, ClrSnow, &g_sFontCm12,
                      "Speed", 0, turnOnGraphVariable),
};


tCanvasWidget tabs[] = {
        CanvasStruct(0, 0, sliders, &g_sKentec320x240x16_SSD2119, 0,
                     30, 320, 160, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
        CanvasStruct(0, 0, set_variables, &g_sKentec320x240x16_SSD2119, 0, 30,
                     320, 160, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
};


void paintGraph(tWidget *psWidget, tContext *psContext){
    GrContextFontSet(psContext, &g_sFontCm12);
    GrContextForegroundSet(psContext, ClrSilver);

    // draw axis // mapping:: 10px = 1 seconds, 10px = 100 value Y axis
    GrLineDraw(psContext, 50, 160, 300, 160);
    GrLineDraw(psContext, 50, 60, 50, 159);
    int x;
    for(x=1;x<=12;x++){
        static char val[5];
        usprintf(val, "%d",x);
        GrStringDraw(psContext, val, -1, 50+x*20, 164, 0);
    }

    int y;
    for(y=1;y<=5;y++){
        int yy =5;
        yy-=y;
        static char val[5];
        usprintf(val, "%d",y*500);
        GrStringDraw(psContext, val, -1, 20, 60+yy*20, 0);
    }

    //clear lines
    tRectangle sRect;
    sRect.i16XMin = 51;
    sRect.i16YMin = 60;
    sRect.i16XMax = GrContextDpyWidthGet(psContext) - 1;
    sRect.i16YMax = 159;
    GrContextForegroundSet(psContext, ClrBlack);
    GrRectFill(psContext, &sRect);

    int i,j;
    for(i=0;i<5;i++){
        if(variables[i].draw){
            for(j=0;j<14;j++){
                if(j<variables[i].time){
                    if(i==0) GrContextForegroundSet(psContext, ClrLightGreen);
                    if(i==1) GrContextForegroundSet(psContext, ClrLightGoldenrodYellow);
                    if(i==2) GrContextForegroundSet(psContext, ClrOrange);
                    if(i==3) GrContextForegroundSet(psContext, ClrCyan);
                    if(i==4) GrContextForegroundSet(psContext, ClrSnow);

                    GrLineDraw(psContext, 51+j*20, 160-((int)variables[i].value[j]/20), 50+(j+1)*20, 160-((int)variables[i].value[j+1]/20));
                } else {
                    break;
                }
            }
        }
    }
}

void turnOnGraphVariable(tWidget *psWidget, uint32_t bSelected){
    uint32_t ui32Idx;

    for(ui32Idx = 0; ui32Idx < 5; ui32Idx++){
        if((psWidget == (tWidget *)(set_variables + ui32Idx)) && (variables[ui32Idx].draw == false)){
            variables[ui32Idx].draw = true;
            variables[ui32Idx].time = 0;
            break;
        }

        if((psWidget == (tWidget *)(set_variables + ui32Idx)) && (variables[ui32Idx].draw == true)){
            variables[ui32Idx].draw = false;
            variables[ui32Idx].time = 0;
            break;
        }
    }
}

void paintMotorControl(tWidget *psWidget, tContext *psContext){

    GrContextFontSet(psContext, &g_sFontCm16);
    GrContextForegroundSet(psContext, ClrSilver);


    GrStringDraw(psContext, "Set Speed:", -1, 30, 40, 0);
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

    motorOn = true;
    setRPM(speed);
    toggleLight(0,1);

    WidgetRemove((tWidget *)&startButton);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&stopButton);
    WidgetPaint((tWidget *)&stopButton);
}

void stopMotor(){

    motorOn = false;
    setRPM(0);
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
        WidgetRemove((tWidget *)(&dispSpeed));
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
        WidgetAdd(WIDGET_ROOT, (tWidget *)&dispSpeed);
        WidgetPaint((tWidget *)&dispSpeed);
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

void changeSpeedDisplay(int disp){
    static char spd[10];
    usprintf(spd, "%d", disp);
    CanvasTextSet(&dispSpeed, spd);
    WidgetPaint((tWidget *)&dispSpeed);
}

//called every 1 second
void drawAllAnalytics(){

    int i, toSet, value;
    for(i=0;i<5;i++){
        if(i==4){
            toSet = getRPM();
            if(disp_tab == 0) changeSpeedDisplay(toSet);


            // FOR JAKE TO DEBUG
            //value = FUNC
            /*static char jakesString[10];
            usprintf(jakesString, "%d", value);
            CanvasTextSet(&titleNames, jakesString);
            WidgetPaint((tWidget *)&titleNames);*/
        }

        variables[i].value[variables[i].time] = toSet;
    }

    if(disp_tab == 1) WidgetPaint((tWidget *)(&analytics));

    for(i=0;i<5;i++){
        variables[i].time++;
        if(variables[i].time >= 13) variables[i].time = 0;
    }
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
    WidgetAdd(WIDGET_ROOT, (tWidget *)&dispSpeed);

    // get this working...
    changeDisplayDate();

    // run get lux here and change this line
    changeDisplayToNight();//changeDisplayToDay();

    CanvasTextSet(&titleNames, namesOfPanels[0]);
    WidgetPaint(WIDGET_ROOT);
}

//get set variables
int getUserSetAccelerometer()   {   return  acc;        }
int getUserSetCurrent()         {   return  current;    }
int getUserSetTemperature()     {   return  temp;       }





