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
bool tt_vector_angle(const float base[3], const float vector[3], float *angle)
{
  float a=norm(base), b=norm(vector);
  if (!isfinite(a) || !isfinite(b) || a < 0.001f || b < 0.001f) return false;
  float dot=0;
  for (unsigned axis=0;axis<3;++axis) dot+=(base[axis]/a)*(vector[axis]/b);
  if (!isfinite(dot)) return false;
  if (dot > 1.0f) dot=1.0f;
  if (dot < -1.0f) dot=-1.0f;
  *angle=acosf(dot)*57.2957795131f;
  return isfinite(*angle);
}
static void clear_window(tt_trainer *t)
{
  t->window_count=0; t->window_index=0; t->valid=false;
}
static void estimate(tt_trainer *t, const tt_sample *sample)
{
  memcpy(t->window[t->window_index],sample->acceleration,sizeof sample->acceleration);
  t->window_index=(t->window_index+1U)%5U;
  if (t->window_count < 5U) ++t->window_count;
  if (t->window_count != 5U) { t->valid=false; return; }
  float mean[3]={0};
  for (unsigned i=0;i<5;++i) for (unsigned axis=0;axis<3;++axis) mean[axis]+=t->window[i][axis]/5.0f;
  t->valid=t->adc_valid && tt_vector_angle(t->base,mean,&t->angle);
}
bool tt_boundary_update(tt_boundary *b, float angle, float limit, uint32_t now)
{
  tt_position target=b->position;
  if (b->position==TT_WITHIN && angle>=limit) target=TT_OUTSIDE;
  if (b->position==TT_OUTSIDE && angle<=limit-1.0f) target=TT_WITHIN;
  if (target==b->position) { b->pending=false; return false; }
  if (!b->pending || b->target!=target) {
    b->target=target; b->started=now; b->pending=true;
  }
  if ((uint32_t)(now-b->started) < 100U) return false;
  b->position=target; b->pending=false; return true;
}
static void unstable(tt_trainer *t, const char *reason)
{
  clear_window(t); t->recovering=false; t->boundary.pending=false;
  if (t->mode!=TT_UNSTABLE) t->event=TT_MODE_CHANGE;
  t->mode=TT_UNSTABLE; t->reason=reason;
}
void tt_sensor_start_result(tt_trainer *t, tt_io_result result)
{
  bool adc_valid=t->adc_valid; uint16_t adc=t->adc;
  tt_trainer_init(t); tt_trainer_adc(t,adc_valid,adc);
  t->mode=result==TT_IO_OK ? TT_WAIT_BASE : TT_SENSOR_ERROR;
  t->reason=result==TT_IO_OK ? "NO_BASE" : tt_io_name(result);
  t->event=TT_MODE_CHANGE;
}
bool tt_led(const tt_trainer *t)
{
  return t->mode==TT_TRAINING && t->valid && t->boundary.position==TT_OUTSIDE;
}
void tt_trainer_init(tt_trainer *t)
{
  memset(t,0,sizeof *t); t->mode=TT_WAIT_BASE; t->reason="NO_BASE";
}
void tt_request_base(tt_trainer *t, uint32_t now)
{
  if (t->mode==TT_CALIBRATING || t->mode==TT_SENSOR_ERROR) return;
  clear_window(t);
  t->mode=TT_CALIBRATING; t->valid=false; t->cal_count=0; t->cal_started=now;
  memset(t->cal_mean,0,sizeof t->cal_mean); memset(t->cal_m2,0,sizeof t->cal_m2);
  t->event=TT_CAL_START; t->reason="COLLECTING";
}
static void reject(tt_trainer *t, const char *reason, bool stable)
{
  clear_window(t); t->recovering=false; t->boundary.pending=false;
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
  if (!t->adc_valid) { t->valid=false; t->boundary.pending=false; return; }
  t->adc=adc; t->limit=5.0f+40.0f*(float)adc/4095.0f;
}
void tt_trainer_sample(tt_trainer *t, const tt_sample *sample, tt_io_result status, uint32_t now)
{
  uint32_t previous=t->last_sample; t->last_sample=now;
  if (t->mode==TT_SENSOR_ERROR) return;
  bool stable=status==TT_IO_OK && tt_sample_stable(sample);
  if (status!=TT_IO_OK) {
    clear_window(t); t->boundary.pending=false; t->recovering=false;
    if (++t->read_errors>=3) {
      t->mode=TT_SENSOR_ERROR; t->valid=false; t->base_valid=false;
      t->reason=tt_io_name(status); t->event=TT_MODE_CHANGE; return;
    }
    if (t->mode==TT_CALIBRATING) reject(t,"READ_ERROR",false);
    return;
  }
  t->read_errors=0;
  if (t->mode==TT_TRAINING || t->mode==TT_UNSTABLE) {
    if (!stable) { unstable(t,"MOTION"); return; }
    if (t->mode==TT_UNSTABLE) {
      if (!t->recovering || (uint32_t)(now-previous)>30U) {
        t->recovering=true; t->stable_since=now; clear_window(t);
      }
      estimate(t,sample);
      if ((uint32_t)(now-t->stable_since)<1000U) { t->valid=false; return; }
      t->mode=TT_TRAINING; t->reason="OK"; t->event=TT_MODE_CHANGE;
      t->boundary=(tt_boundary){0};
    } else estimate(t,sample);
    if (t->valid) {
      if (tt_boundary_update(&t->boundary,t->angle,t->limit,now)) t->event=TT_POSITION_CHANGE;
    } else t->boundary.pending=false;
    return;
  }
  if (t->mode!=TT_CALIBRATING) return;
  if (t->cal_count && now==previous) return;
  if (t->cal_count && (uint32_t)(now-previous)>30U) { reject(t,"SAMPLE_GAP",false); return; }
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
  clear_window(t);
  t->boundary=(tt_boundary){0};
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
