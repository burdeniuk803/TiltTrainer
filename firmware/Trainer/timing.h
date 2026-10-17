#ifndef TT_TIMING_H
#define TT_TIMING_H
#include <stdbool.h>
#include <stdint.h>
typedef struct { uint32_t last, period; } tt_clock;
bool tt_due(tt_clock *clock, uint32_t now);
typedef struct { bool candidate, stable, armed; uint32_t changed_at; } tt_button;
void tt_button_init(tt_button *button, uint32_t now);
bool tt_button_update(tt_button *button, bool pressed, uint32_t now);
#endif
