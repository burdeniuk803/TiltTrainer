#include "mpu6050.h"
#include "trainer.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
/* Actual bytes from hal-events-functional-full.vcd, SHA256 577716d1330f438a
 * 1cd464afce5ae370b9f8a337e242d80f208c88835a11aae1. This replays decoding
 * and application math on the host; it does not rerun the I2C peripheral. */
void test_recorded(void)
{
  const uint8_t upright[14]={0,0,0,0,0x40,0,0xEF,0x5C,0,0,0,0,0,0};
  const uint8_t tilt[14]={0x20,0,0,0,0x36,0x66,0xEF,0x5C,0,0,0,0,0,0};
  const uint8_t motion[14]={0x40,0,0,0,0x40,0,0xEF,0x5C,0,0,0,0,0,0};
  tt_sample base,s,moving;
  tt_imu_decode(upright,&base);tt_imu_decode(tilt,&s);tt_imu_decode(motion,&moving);
  assert(s.acceleration[0]==0.5f && s.acceleration[2]==13926.0f/16384.0f);
  assert(fabsf(s.temperature-24)<0.002f && s.angular_rate[0]==0);
  tt_trainer t;tt_trainer_init(&t);tt_trainer_adc(&t,true,2050);tt_request_base(&t,0);
  for(unsigned i=1;i<=50;++i)tt_trainer_sample(&t,&base,TT_IO_OK,i*20);
  assert(t.base_valid && t.mode==TT_TRAINING);
  for(unsigned i=1;i<=12;++i)tt_trainer_sample(&t,&s,TT_IO_OK,1000+i*20);
  assert(t.valid && fabsf(t.angle-30.5f)<0.05f && tt_led(&t));
  tt_request_base(&t,1240);
  for(unsigned i=1;i<=50;++i)tt_trainer_sample(&t,&s,TT_IO_OK,1240+i*20);
  for(unsigned i=1;i<=5;++i)tt_trainer_sample(&t,&s,TT_IO_OK,2240+i*20);
  assert(t.valid && t.angle<0.1f && !tt_led(&t));
  tt_trainer_sample(&t,&moving,TT_IO_OK,2360);
  assert(t.mode==TT_UNSTABLE && !t.valid && !tt_led(&t));
  puts("PASS recorded Wokwi frames: units, temperature, 30.5 deg, tilted baseline, motion");
}
