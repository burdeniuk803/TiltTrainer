#ifndef TT_TRAINER_H
#define TT_TRAINER_H
#include "mpu6050.h"
#include <stdbool.h>
#include <stdint.h>
typedef enum { TT_WAIT_BASE, TT_CALIBRATING, TT_TRAINING, TT_UNSTABLE, TT_SENSOR_ERROR } tt_mode;
typedef enum { TT_WITHIN, TT_OUTSIDE } tt_position;
typedef enum { TT_NO_EVENT, TT_CAL_START, TT_CAL_OK, TT_CAL_REJECT, TT_MODE_CHANGE, TT_POSITION_CHANGE } tt_event;
typedef struct {
  tt_mode mode;
  bool base_valid, valid, adc_valid;
  float base[3], angle, limit;
  float window[5][3];
  unsigned window_count, window_index;
  uint16_t adc;
  unsigned cal_count, read_errors;
  float cal_mean[3], cal_m2[3];
  uint32_t cal_started, last_sample;
  const char *reason;
  tt_event event;
} tt_trainer;
void tt_trainer_init(tt_trainer *trainer);
void tt_request_base(tt_trainer *trainer, uint32_t now);
void tt_trainer_tick(tt_trainer *trainer, uint32_t now);
void tt_trainer_sample(tt_trainer *trainer, const tt_sample *sample, tt_io_result status, uint32_t now);
void tt_trainer_adc(tt_trainer *trainer, bool ok, uint16_t adc);
bool tt_sample_stable(const tt_sample *sample);
bool tt_vector_angle(const float base[3], const float vector[3], float *angle);
const char *tt_mode_name(tt_mode mode);
const char *tt_event_name(tt_event event);
#endif
