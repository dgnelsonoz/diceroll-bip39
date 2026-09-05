#ifndef WAVESHARE_UI_H
#define WAVESHARE_UI_H

#include <stdint.h>

#include "layout.h"

uint16_t ui_show_hold_progress( uint16_t *pixels,
                                        DicerollButton button,
                                        int64_t elapsed_us,
                                        uint16_t previous_progress );
void ui_clear_hold_progress( uint16_t *pixels,
                                      DicerollButton button );

#endif
