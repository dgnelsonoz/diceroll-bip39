#ifndef STM32_UI_H
#define STM32_UI_H

#include "layout.h"
#include "mnemonic_state.h"

#include <stdint.h>

void ui_draw(const MnemonicState *state);
void ui_update(const MnemonicState *state);
void ui_draw_error(const char *message);
DicerollButton ui_hit_test(uint16_t x, uint16_t y);
int ui_select_word_at(const MnemonicState *state,
                               uint16_t x, uint16_t y);
void ui_show_hold_progress(DicerollButton button,
                                    uint32_t elapsed_ms,
                                    uint32_t required_ms);
void ui_clear_hold_progress(DicerollButton button,
                                     int phrase_complete);

#endif
