#ifndef CAMERA_H_
#define CAMERA_H_


int image[5000];

//initializes pins, clocks and controller
void camera_init(UART *debug);


//tells dcmi controller to start frame capture
void camera_capture_frame();

//returns 1 if image array has at least 1 non-0 element;
int camera_test_image_array();


#endif
