#include "mpu6050.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct { uint8_t id, frame[14]; int calls, fail; tt_io_result error; } fake;
static tt_io_result read_reg(void *ctx, uint8_t address, uint8_t reg, uint8_t *out, size_t size)
{
  fake *f = ctx;
  assert(address == 0x68);
  if (++f->calls == f->fail) return f->error;
  if (reg == 0x75) { assert(size == 1); *out = f->id; }
  else { assert(reg == 0x3B && size == 14); memcpy(out, f->frame, 14); }
  return TT_IO_OK;
}
static tt_io_result write_reg(void *ctx, uint8_t address, uint8_t reg, uint8_t value)
{
  fake *f = ctx;
  static const uint8_t expected[][2] = {{0x6B,1},{0x1A,3},{0x19,19},{0x1B,0},{0x1C,0}};
  assert(address == 0x68);
  if (++f->calls == f->fail) return f->error;
  assert(f->calls >= 2 && f->calls <= 6);
  assert(reg == expected[f->calls-2][0] && value == expected[f->calls-2][1]);
  return TT_IO_OK;
}
void test_imu(void)
{
  fake f = {.id=0x68}; tt_imu imu = {&f,read_reg,write_reg,0x68}; uint8_t id;
  assert(tt_imu_start(&imu,&id) == TT_IO_OK && id == 0x68 && f.calls == 6);
  for (int kind=0; kind<2; ++kind) for (int step=1; step<=6; ++step) {
    f.calls=0; f.fail=step; f.error=kind ? TT_IO_TIMEOUT : TT_IO_NACK;
    assert(tt_imu_start(&imu,&id) == f.error && f.calls == step);
  }
  f.calls=0; f.fail=0; f.id=0x69;
  assert(tt_imu_start(&imu,&id) == TT_IO_BAD_ID && f.calls == 1);
  const uint8_t raw[14]={0xE0,0,0x20,0,0x40,0,0xEF,0x5C,0xFF,0x7D,0,0x83,0,0};
  memcpy(f.frame,raw,14); tt_sample s={0};
  assert(tt_imu_read(&imu,&s)==TT_IO_OK);
  assert(s.acceleration[0]==-0.5f && s.acceleration[1]==0.5f && s.acceleration[2]==1.0f);
  assert(s.angular_rate[0]==-1 && s.angular_rate[1]==1 && fabsf(s.temperature-24)<0.01f);
  tt_sample before=s; f.fail=f.calls+1; f.error=TT_IO_TIMEOUT;
  assert(tt_imu_read(&imu,&s)==TT_IO_TIMEOUT && memcmp(&s,&before,sizeof s)==0);
  assert(strcmp(tt_io_name(TT_IO_BAD_ID),"BAD_ID")==0);
  assert(strcmp(tt_io_name((tt_io_result)99),"UNKNOWN")==0);
  puts("PASS imu: register sequence, signed units, NACK/timeout/ID, output preservation");
}
#ifndef TT_TEST_ALL
int main(void) { test_imu(); return 0; }
#endif
