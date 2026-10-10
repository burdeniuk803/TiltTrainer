#include "trainer.h"
#include <math.h>
#include <string.h>
static float norm(const float vector[3])
{
  return sqrtf(vector[0]*vector[0]+vector[1]*vector[1]+vector[2]*vector[2]);
}
bool tt_sample_stable(const tt_sample *sample)
{
  for (unsigned i=0;i<3;++i)
    if (!isfinite(sample->acceleration[i]) || !isfinite(sample->angular_rate[i])) return false;
  float length=norm(sample->acceleration);
  return length >= 0.95f && length <= 1.05f && norm(sample->angular_rate) <= 5.0f;
}
void tt_trainer_init(tt_trainer *t)
{
  memset(t,0,sizeof *t); t->mode=TT_WAIT_BASE; t->reason="NO_BASE";
}
void tt_request_base(tt_trainer *t, uint32_t now)
{
  if (t->mode==TT_CALIBRATING || t->mode==TT_SENSOR_ERROR) return;
  t->mode=TT_CALIBRATING; t->valid=false; t->cal_count=0; t->cal_started=now;
  memset(t->cal_mean,0,sizeof t->cal_mean); memset(t->cal_m2,0,sizeof t->cal_m2);
  t->event=TT_CAL_START; t->reason="COLLECTING";
}
static void reject(tt_trainer *t, const char *reason, bool stable)
{
  t->valid=false; t->mode=t->base_valid ? (stable ? TT_TRAINING : TT_UNSTABLE) : TT_WAIT_BASE;
  t->reason=reason; t->event=TT_CAL_REJECT;
}
void tt_trainer_tick(tt_trainer *t, uint32_t now)
{
  if (t->mode==TT_CALIBRATING && (uint32_t)(now-t->cal_started)>=1200U)
    reject(t,"CAL_DEADLINE",false);
}
void tt_trainer_adc(tt_trainer *t, bool ok, uint16_t adc)
{
  t->adc_valid=ok && adc<=4095;
  if (!t->adc_valid) { t->valid=false; return; }
  t->adc=adc; t->limit=5.0f+40.0f*(float)adc/4095.0f;
}
void tt_trainer_sample(tt_trainer *t, const tt_sample *sample, tt_io_result status, uint32_t now)
{
  t->last_sample=now;
  bool stable=status==TT_IO_OK && tt_sample_stable(sample);
  if (t->mode!=TT_CALIBRATING) return;
  if (now==t->cal_started) return; /* A sample from the request tick is not new. */
  if (!stable) { reject(t,status==TT_IO_OK?"MOTION":"READ_ERROR",false); return; }
  ++t->cal_count;
  for (unsigned axis=0;axis<3;++axis) {
    float delta=sample->acceleration[axis]-t->cal_mean[axis];
    t->cal_mean[axis]+=delta/(float)t->cal_count;
    t->cal_m2[axis]+=delta*(sample->acceleration[axis]-t->cal_mean[axis]);
  }
  if (t->cal_count!=50) return;
  for (unsigned axis=0;axis<3;++axis)
    if (t->cal_m2[axis]/50.0f > 0.02f*0.02f) { reject(t,"VARIANCE",true); return; }
  float length=norm(t->cal_mean);
  if (!isfinite(length) || length < 0.001f) { reject(t,"INVALID_MEAN",true); return; }
  for (unsigned axis=0;axis<3;++axis) t->base[axis]=t->cal_mean[axis]/length;
  t->base_valid=true; t->mode=TT_TRAINING; t->event=TT_CAL_OK; t->reason="OK";
}
const char *tt_mode_name(tt_mode mode)
{
  switch (mode) {
    case TT_WAIT_BASE:return "WAIT_BASE"; case TT_CALIBRATING:return "CALIBRATING";
    case TT_TRAINING:return "TRAINING"; case TT_UNSTABLE:return "UNSTABLE";
    case TT_SENSOR_ERROR:return "SENSOR_ERROR";
  }
  return "UNKNOWN";
}
const char *tt_event_name(tt_event event)
{
  switch (event) {
    case TT_NO_EVENT:return "NONE"; case TT_CAL_START:return "CAL_START";
    case TT_CAL_OK:return "CAL_OK"; case TT_CAL_REJECT:return "CAL_REJECT";
    case TT_MODE_CHANGE:return "MODE"; case TT_POSITION_CHANGE:return "POSITION";
  }
  return "UNKNOWN";
}
