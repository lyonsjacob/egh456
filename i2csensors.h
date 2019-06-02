#ifndef i2csensors_H_
#define i2csensors_H_


//user get variables
int getLux(int res);
int getAcc(int res);

//init functions
void initLux();
void initAcc();

//read functions
void readLux();
void readAcc();

#endif /* i2csensors_H_ */
