/*
 * GUI.h
 *
 *  Created on: 11 May 2019
 *      Author: root
 */

#ifndef GUI_H_
#define GUI_H_

//user set variables
int getUserSetSpeed();
int getUserSetAccelerometer();
int getUserSetCurrent();
int getUserSetTemperature();

//functions to change display
void changeDisplayToNight();
void changeDisplayToDay();

//init widgets
void GUI_init();



#endif /* GUI_H_ */
