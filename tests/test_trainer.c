#include "trainer.h"
#include "timing.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
void test_imu(void);
static const tt_sample upright={.acceleration={0,0,1}};
static tt_sample pose(float degrees, unsigned axis)
{
  tt_sample s=upright; float rad=degrees/57.2957795131f;
  s.acceleration[axis]=sinf(rad); s.acceleration[2]=cosf(rad); return s;
}
static uint32_t calibrate(tt_trainer *t, tt_sample s, uint32_t start)
{
  tt_request_base(t,start); assert(t->mode==TT_CALIBRATING && !t->valid);
  tt_trainer_sample(t,&s,TT_IO_OK,start); assert(t->cal_count==0);
  for (unsigned i=1;i<=50;++i) {
    tt_trainer_sample(t,&s,TT_IO_OK,start+i*20U);
    if (i<50) assert(t->mode==TT_CALIBRATING);
  }
  assert(t->event==TT_CAL_OK && t->cal_count==50 && t->base_valid && t->mode==TT_TRAINING);
  return start+1000U;
}
static void test_button_clock(void)
{
  tt_button b; tt_button_init(&b,0);
  assert(!tt_button_update(&b,true,0)); assert(!tt_button_update(&b,false,20));
  assert(!tt_button_update(&b,true,21)); assert(!tt_button_update(&b,true,50));
  assert(tt_button_update(&b,true,51)); assert(!tt_button_update(&b,true,2051));
  assert(!tt_button_update(&b,false,2052)); assert(!tt_button_update(&b,true,2072));
  assert(!tt_button_update(&b,true,2110));
  assert(!tt_button_update(&b,false,2111)); assert(!tt_button_update(&b,false,2141));
  assert(!tt_button_update(&b,true,2142)); assert(tt_button_update(&b,true,2172));
  tt_button_init(&b,UINT32_MAX-10U);
  assert(!tt_button_update(&b,true,UINT32_MAX-10U)); assert(tt_button_update(&b,true,19));
  tt_clock c={UINT32_MAX-10U,20}; assert(!tt_due(&c,8)); assert(tt_due(&c,9));
  assert(!tt_due(&c,9)); assert(tt_due(&c,100)); assert(!tt_due(&c,100));
  puts("PASS T03: debounce, bounce, hold, rearm, tick wrap; periodic deadlines");
}
static void test_calibration(void)
{
  tt_trainer t; tt_trainer_init(&t);
  assert(t.mode==TT_WAIT_BASE && !t.base_valid && !t.valid && !tt_led(&t));
  tt_trainer_sample(&t,&upright,TT_IO_OK,20); assert(!t.valid);
  tt_trainer_adc(&t,true,2048);
  uint32_t now=calibrate(&t,upright,100);
  for (unsigned i=1;i<=5;++i) tt_trainer_sample(&t,&upright,TT_IO_OK,now+i*20);
  assert(t.valid && t.angle<0.1f && !tt_led(&t));
  float old[3]; memcpy(old,t.base,sizeof old);
  tt_request_base(&t,1300); tt_request_base(&t,1340);
  assert(t.cal_started==1300 && t.cal_count==0);
  tt_sample moving=upright; moving.angular_rate[0]=10;
  tt_trainer_sample(&t,&moving,TT_IO_OK,1360);
  assert(t.event==TT_CAL_REJECT && t.mode==TT_UNSTABLE && memcmp(old,t.base,sizeof old)==0);
  tt_request_base(&t,1400);
  for(unsigned i=1;i<=50;++i) {
    tt_sample jitter=pose(i%2 ? 2.0f : -2.0f,0);
    tt_trainer_sample(&t,&jitter,TT_IO_OK,1400+i*20);
  }
  assert(t.event==TT_CAL_REJECT && strcmp(t.reason,"VARIANCE")==0 && memcmp(old,t.base,sizeof old)==0);
  tt_request_base(&t,2500); tt_trainer_tick(&t,3699); assert(t.mode==TT_CALIBRATING);
  tt_trainer_tick(&t,3700); assert(t.event==TT_CAL_REJECT && strcmp(t.reason,"CAL_DEADLINE")==0);
  tt_trainer_init(&t); tt_request_base(&t,0);
  tt_sample bad=upright; bad.acceleration[2]=1.2f;
  tt_trainer_sample(&t,&bad,TT_IO_OK,20); assert(t.mode==TT_WAIT_BASE && !t.base_valid);
  tt_request_base(&t,50); tt_trainer_sample(&t,&upright,TT_IO_NACK,70);
  assert(t.event==TT_CAL_REJECT && strcmp(t.reason,"READ_ERROR")==0);
  tt_trainer_init(&t); tt_request_base(&t,0);
  tt_trainer_sample(&t,&upright,TT_IO_OK,20);
  tt_trainer_sample(&t,&upright,TT_IO_OK,61);
  assert(t.event==TT_CAL_REJECT && strcmp(t.reason,"SAMPLE_GAP")==0);
  tt_trainer_init(&t); calibrate(&t,pose(30,0),UINT32_MAX-500U);
  assert(fabsf(t.base[0]-0.5f)<0.0001f);
  puts("PASS T01/T04/T05: 50 fresh samples, inclined baseline, rejection retention, deadline, nested press, wrap");
}
static void test_angles_adc(void)
{
  float max_error=0;
  for(unsigned axis=0;axis<2;++axis) for(int angle=-60;angle<=60;angle+=10) {
    tt_sample s=pose((float)angle,axis); float actual;
    assert(tt_vector_angle(upright.acceleration,s.acceleration,&actual));
    float error=fabsf(actual-fabsf((float)angle)); if(error>max_error)max_error=error;
    assert(error<=1.0f);
  }
  tt_sample base=pose(30,0), other=pose(60,0); float angle;
  assert(tt_vector_angle(base.acceleration,other.acceleration,&angle) && fabsf(angle-30)<0.001f);
  float zero[3]={0}, bad[3]={NAN,0,1}, inf[3]={INFINITY,0,1};
  assert(!tt_vector_angle(zero,upright.acceleration,&angle));
  assert(!tt_vector_angle(upright.acceleration,bad,&angle));
  assert(!tt_vector_angle(inf,upright.acceleration,&angle));
  float roundoff[3]={0.1f,0.2f,0.3f}; assert(tt_vector_angle(roundoff,roundoff,&angle) && angle<0.1f);
  float opposite[3]={0,0,-1}; assert(tt_vector_angle(upright.acceleration,opposite,&angle) && angle==180);
  tt_trainer t; tt_trainer_init(&t); float previous=-1;
  for(unsigned adc=0;adc<=4095;++adc) { tt_trainer_adc(&t,true,(uint16_t)adc); assert(t.limit>previous); previous=t.limit; }
  const unsigned codes[]={0,1024,2048,3072,4095};
  const unsigned tenths[]={50,150,250,350,450};
  for(unsigned i=0;i<5;++i) { tt_trainer_adc(&t,true,(uint16_t)codes[i]); assert((unsigned)(t.limit*10+0.5f)==tenths[i]); }
  tt_trainer_adc(&t,true,4096); assert(!t.adc_valid);
  printf("PASS T06/T07: vector error_max=%.6f deg, 4096 monotonic ADC codes and endpoints\n",max_error);
}
static void test_thresholds(void)
{
  const float entry[]={19.9f,20.0f,20.1f};
  const float exit[]={19.1f,19.0f,18.9f};
  for(unsigned i=0;i<3;++i) {
    tt_boundary b={0}; assert(!tt_boundary_update(&b,entry[i],20,0));
    assert(!tt_boundary_update(&b,entry[i],20,99));
    bool changed=tt_boundary_update(&b,entry[i],20,100);
    assert(changed==(i>0));
    b=(tt_boundary){.position=TT_OUTSIDE};
    assert(!tt_boundary_update(&b,exit[i],20,200));
    changed=tt_boundary_update(&b,exit[i],20,300); assert(changed==(i>0));
  }
  tt_boundary b={0}; tt_boundary_update(&b,21,20,0);
  tt_boundary_update(&b,0,20,40); assert(b.position==TT_WITHIN);
  b=(tt_boundary){.position=TT_OUTSIDE};
  for(unsigned i=0;i<50;++i) assert(!tt_boundary_update(&b,i%2?19.3f:19.7f,20,i*20));
  assert(!tt_boundary_update(&b,18.8f,20,1000)); assert(tt_boundary_update(&b,18.8f,20,1100));
  b=(tt_boundary){0};
  assert(!tt_boundary_update(&b,20,20,UINT32_MAX-50U)); assert(tt_boundary_update(&b,20,20,49));
  assert(strcmp(tt_mode_name((tt_mode)99),"UNKNOWN")==0);
  assert(strcmp(tt_event_name((tt_event)99),"UNKNOWN")==0);
  puts("PASS T08/T09: exact boundaries, 100 ms, hysteresis, 40 ms excursion, overflow");
}
static void test_safety(void)
{
  tt_trainer t; tt_trainer_init(&t); tt_trainer_adc(&t,true,0);
  uint32_t now=calibrate(&t,upright,0); tt_sample tilt=pose(30,0);
  for(unsigned i=1;i<=12;++i) tt_trainer_sample(&t,&tilt,TT_IO_OK,now+i*20);
  assert(t.valid && t.boundary.position==TT_OUTSIDE && tt_led(&t));
  now+=240;
  tt_trainer_sample(&t,&tilt,TT_IO_NACK,now+20); assert(!t.valid && !tt_led(&t));
  tt_trainer_sample(&t,&tilt,TT_IO_OK,now+40); assert(t.read_errors==0);
  for(unsigned i=1;i<=3;++i) tt_trainer_sample(&t,&tilt,TT_IO_TIMEOUT,now+40+i*20);
  assert(t.mode==TT_SENSOR_ERROR && !t.base_valid && !t.valid && !tt_led(&t));
  tt_request_base(&t,now+110); assert(t.mode==TT_SENSOR_ERROR);
  tt_trainer_sample(&t,&upright,TT_IO_OK,now+120); assert(t.mode==TT_SENSOR_ERROR);
  tt_sensor_start_result(&t,TT_IO_BAD_ID); assert(t.mode==TT_SENSOR_ERROR);
  tt_sensor_start_result(&t,TT_IO_OK); assert(t.mode==TT_WAIT_BASE && !t.base_valid);
  now=calibrate(&t,upright,2000);
  for(unsigned i=1;i<=6;++i) tt_trainer_sample(&t,&tilt,TT_IO_OK,now+i*20);
  tt_trainer_adc(&t,false,0); assert(!t.valid && !tt_led(&t));
  tt_trainer_sample(&t,&tilt,TT_IO_OK,now+140); assert(!t.valid);
  tt_trainer_adc(&t,true,0); tt_trainer_sample(&t,&tilt,TT_IO_OK,now+160); assert(t.valid);
  tt_sample moving=upright; moving.acceleration[2]=1.2f;
  tt_trainer_sample(&t,&moving,TT_IO_OK,now+180); assert(t.mode==TT_UNSTABLE && !t.valid && !tt_led(&t));
  now+=200;
  tt_trainer_sample(&t,&upright,TT_IO_OK,now);
  for(unsigned i=1;i<50;++i) { tt_trainer_sample(&t,&upright,TT_IO_OK,now+i*20); assert(t.mode==TT_UNSTABLE && !t.valid); }
  tt_trainer_sample(&t,&upright,TT_IO_OK,now+1000); assert(t.mode==TT_TRAINING && t.valid && t.angle<0.1f);
  moving=upright; moving.angular_rate[0]=10;
  tt_trainer_sample(&t,&moving,TT_IO_OK,now+1020); assert(t.mode==TT_UNSTABLE);
  tt_trainer_sample(&t,&upright,TT_IO_OK,now+1040);
  tt_trainer_sample(&t,&upright,TT_IO_OK,now+1100); assert(t.stable_since==now+1100);
  tt_sample bad=upright; bad.acceleration[0]=NAN;
  tt_trainer_sample(&t,&bad,TT_IO_OK,now+1120); assert(t.mode==TT_UNSTABLE && !t.valid);
  tt_sample zero={0}; assert(!tt_sample_stable(&zero));
  tt_sample edge=upright; edge.angular_rate[0]=3;edge.angular_rate[1]=4; assert(tt_sample_stable(&edge));
  edge.angular_rate[2]=0.1f; assert(!tt_sample_stable(&edge));
  puts("PASS T10/T11/T12: LED validity, failed reads, ADC fault, sensor recovery, 1 s stable, stale-filter removal");
}
static void test_concurrency(void)
{
  tt_clock m={0,20},a={0,100},u={0,200}; tt_button button; tt_button_init(&button,0);
  tt_trainer t; tt_trainer_init(&t);
  unsigned m_count=0,a_count=0,u_count=0,presses=0,max_m=0,max_u=0;
  uint32_t last_m=0,last_u=0;
  for(uint32_t now=0;now<=60000;++now) {
    bool pressed=now%10000>=100 && now%10000<250;
    if(tt_button_update(&button,pressed,now)) { ++presses;tt_request_base(&t,now); }
    tt_trainer_tick(&t,now);
    if(tt_due(&a,now)) { ++a_count;tt_trainer_adc(&t,true,(uint16_t)(now/100%4096)); }
    if(tt_due(&m,now)) {
      unsigned delta=now-last_m; if(delta>max_m)max_m=delta;last_m=now;++m_count;
      tt_sample s=pose(now%10000<2000?0:30,0);tt_trainer_sample(&t,&s,TT_IO_OK,now);
    }
    if(tt_due(&u,now)) { unsigned delta=now-last_u;if(delta>max_u)max_u=delta;last_u=now;++u_count; }
  }
  assert(m_count==3000 && a_count==600 && u_count==300 && presses==6);
  assert(max_m<=30 && max_u<=250);
  printf("PASS T13 host scheduler: duration=60000 ms MPU=%u max=%u ms ADC=%u UART=%u max=%u ms presses=%u\n",m_count,max_m,a_count,u_count,max_u,presses);
}
int main(void)
{
  test_imu();test_button_clock();test_calibration();test_angles_adc();test_thresholds();test_safety();test_concurrency();
  puts("PASS all TiltTrainer host groups"); return 0;
}
