#ifndef TT_MPU6050_H
#define TT_MPU6050_H
#include <stddef.h>
#include <stdint.h>
typedef enum { TT_IO_OK, TT_IO_NACK, TT_IO_TIMEOUT, TT_IO_BAD_ID, TT_IO_BUS_ERROR } tt_io_result;
typedef struct {
  void *context;
  tt_io_result (*read)(void *, uint8_t, uint8_t, uint8_t *, size_t);
  tt_io_result (*write)(void *, uint8_t, uint8_t, uint8_t);
  uint8_t address;
} tt_imu;
typedef struct {
  float acceleration[3];
  float angular_rate[3];
  float temperature;
  uint8_t raw[14];
} tt_sample;
tt_io_result tt_imu_start(tt_imu *imu, uint8_t *observed_id);
tt_io_result tt_imu_read(tt_imu *imu, tt_sample *sample);
void tt_imu_decode(const uint8_t raw[14], tt_sample *sample);
const char *tt_io_name(tt_io_result result);
#endif
