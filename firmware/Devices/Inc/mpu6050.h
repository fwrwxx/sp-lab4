#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include "bus.h"

#define MPU6050_ADDR 0x68
#define MPU6050_WHO_AM_I_VAL 0x68

typedef struct {
    uint8_t who_am_i;
} mpu6050_t;

bus_status_t MPU6050_Init(mpu6050_t *dev);
bus_status_t MPU6050_ReadAccelX(mpu6050_t *dev, int16_t *raw);
bus_status_t MPU6050_ReadAccelXYZ(mpu6050_t *dev, int16_t *ax, int16_t *ay, int16_t *az);
float MPU6050_AccelRawToG(int16_t raw);

#endif