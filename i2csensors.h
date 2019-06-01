#ifndef i2csensors_H_
#define i2csensors_H_


//user get variables
float getLux();
float getAcc();

//init functions
void initLux();
void initAcc();

//read functions
void readLux();
void readAcc();

#endif /* i2csensors_H_ */
