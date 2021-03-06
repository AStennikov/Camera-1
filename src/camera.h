#ifndef CAMERA_H_
#define CAMERA_H_

#define IMAGE_ARRAY_SIZE 40000
int image[IMAGE_ARRAY_SIZE];

//initializes pins, clocks and controller
int camera_init(UART *debug);

//tells dcmi controller to start frame capture
void camera_capture_frame();

//returns 1 if image array has at least 1 non-0 element;
int camera_test_image_array();

//sets all values to 0
int camera_clear_image_array();

//these set picture parameters
int camera_set_bin(int new_bin_value);	//binning and skipping is always set to same number
int camera_set_row(int new_row);
int camera_set_column(int new_column);
int camera_set_width(int new_width);
int camera_set_height(int new_height);
int camera_set_shutter_speed(int new_shutter_speed);

//these return picture parameters
int camera_frame_width();
int camera_frame_height();
int camera_bin();

//takes picture. Parameters must be configured before calling this function. No processing is done. Raw data stored in image[] array
int camera_take_picture();

//to convert from 12-bit to 8-bit, we right shift data by new_value number of bits.
int camera_set_rshift(int new_value);

#endif
