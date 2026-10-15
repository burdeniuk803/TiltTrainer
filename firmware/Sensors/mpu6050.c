#include "mpu6050.h"
#include <string.h>
static int16_t signed_word(const uint8_t *bytes)
{
  uint16_t word = (uint16_t)((uint16_t)bytes[0] << 8) | bytes[1];
  return (int16_t)(word <= 32767U ? (int32_t)word : (int32_t)word - 65536);
}
tt_io_result tt_imu_start(tt_imu *imu, uint8_t *observed_id)
{
  uint8_t id = 0;
  tt_io_result result = imu->read(imu->context, imu->address, 0x75, &id, 1);
  *observed_id = id;
  if (result != TT_IO_OK) return result;
  if (id != 0x68) return TT_IO_BAD_ID;
  /* PLL X gyro clock, 50 Hz with DLPF, +/-250 dps and +/-2 g. */
  static const uint8_t registers[][2] = {
    {0x6B, 0x01}, {0x1A, 0x03}, {0x19, 0x13}, {0x1B, 0x00}, {0x1C, 0x00}
  };
  for (size_t i = 0; i < sizeof registers / sizeof registers[0]; ++i) {
    result = imu->write(imu->context, imu->address, registers[i][0], registers[i][1]);
    if (result != TT_IO_OK) return result;
  }
  return TT_IO_OK;
}
void tt_imu_decode(const uint8_t raw[14], tt_sample *sample)
{
  memcpy(sample->raw, raw, 14);
  for (size_t i = 0; i < 3; ++i) {
    sample->acceleration[i] = (float)signed_word(raw + 2 * i) / 16384.0f;
    sample->angular_rate[i] = (float)signed_word(raw + 8 + 2 * i) / 131.0f;
  }
  sample->temperature = (float)signed_word(raw + 6) / 340.0f + 36.53f;
}
tt_io_result tt_imu_read(tt_imu *imu, tt_sample *sample)
{
  uint8_t raw[14];
  tt_io_result result = imu->read(imu->context, imu->address, 0x3B, raw, sizeof raw);
  if (result == TT_IO_OK) tt_imu_decode(raw, sample);
  return result;
}
const char *tt_io_name(tt_io_result result)
{
  switch (result) {
    case TT_IO_OK: return "OK";
    case TT_IO_NACK: return "NACK";
    case TT_IO_TIMEOUT: return "TIMEOUT";
    case TT_IO_BAD_ID: return "BAD_ID";
    case TT_IO_BUS_ERROR: return "BUS_ERROR";
  }
  return "UNKNOWN";
}
