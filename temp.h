/*
 * temp.h
 *
 *  Created on: 30 May 2019
 *      Author: jordikitto
 */

#ifndef TEMP_H_
#define TEMP_H_


// Setup
void setup_temp();
void setup_temp_internal();

// Usage
void read_temp_sensors();
int get_temp1(int resolution);
int get_temp2(int resolution);


#endif /* TEMP_H_ */
