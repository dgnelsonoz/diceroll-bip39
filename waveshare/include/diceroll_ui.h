#ifndef DICEROLL_UI_H
#define DICEROLL_UI_H

#include <stdint.h>

#include "layout.h"

uint16_t diceroll_ui_show_hold_progress( uint16_t *pixels,
                                        DicerollButton button,
                                        int64_t elapsed_us,
                                        uint16_t previous_progress );
void diceroll_ui_clear_hold_progress( uint16_t *pixels,
                                      DicerollButton button );

#endif
