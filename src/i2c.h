#ifndef I2C_H_
#define I2C_H_





//initializes i2c
void i2c_init();


////////////////////////////
// IMAGE SENSOR FUNCTIONS //
////////////////////////////

//reads register
int sensor_get(int reg);

//writes register
void sensor_set(int reg, int data);




#endif
