#ifndef TT_I2C_BUS_H
#define TT_I2C_BUS_H
#include "mpu6050.h"
tt_io_result tt_bus_read(void *, uint8_t, uint8_t, uint8_t *, size_t);
tt_io_result tt_bus_write(void *, uint8_t, uint8_t, uint8_t);
#endif
