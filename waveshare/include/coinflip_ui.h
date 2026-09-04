#ifndef COINFLIP_UI_H
#define COINFLIP_UI_H

#include <stdint.h>

uint16_t coinflip_ui_show_hold_progress( uint16_t *pixels, uint8_t button,
                                        int64_t elapsed_us,
                                        uint16_t previous_progress );
void coinflip_ui_clear_hold_progress( uint16_t *pixels, uint8_t button );

#endif
