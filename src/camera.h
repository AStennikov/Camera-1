#ifndef CAMERA_H_
#define CAMERA_H_


int image[5000];

//initializes pins, clocks and controller
int camera_init(UART *debug);

//tells dcmi controller to start frame capture
void camera_capture_frame();

//returns 1 if image array has at least 1 non-0 element;
int camera_test_image_array();

//these set picture parameters
int camera_set_row(int new_row);
int camera_set_column(int new_column);
int camera_set_width(int new_width);
int camera_set_height(int new_height);
int camera_set_shutter_speed(int new_shutter_speed);

//these return picture parameters
int camera_frame_width();
int camera_frame_height();

//takes picture. Parameters must be configured before calling this function. No processing is done. Raw data stored in image[] array
int camera_take_picture();


#endif
