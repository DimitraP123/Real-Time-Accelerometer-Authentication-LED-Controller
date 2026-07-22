#ifndef ACCEL_DRIVER_H
#define ACCEL_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    int16_t x_axis;
    int16_t y_axis;
    int16_t z_axis;
} accel_sample_t;

void configure_accelerometer(void);

void accel_data_request(void);

bool accel_data_retrieve(accel_sample_t *out);

bool accelerometer_data_available(void);

#endif




