#include "timing.h"
bool tt_due(tt_clock *clock, uint32_t now)
{
  if ((uint32_t)(now - clock->last) < clock->period) return false;
  clock->last = now;
  return true;
}
void tt_button_init(tt_button *button, uint32_t now)
{
  *button = (tt_button){.armed=true, .changed_at=now};
}
bool tt_button_update(tt_button *button, bool pressed, uint32_t now)
{
  if (pressed != button->candidate) {
    button->candidate = pressed;
    button->changed_at = now;
  }
  if (pressed == button->stable || (uint32_t)(now - button->changed_at) < 30U) return false;
  button->stable = pressed;
  if (!pressed) { button->armed = true; return false; }
  if (!button->armed) return false;
  button->armed = false;
  return true;
}
