#ifndef DICEROLL_UI_H
#define DICEROLL_UI_H

#include "layout.h"
#include "mnemonic_state.h"

#include <stdint.h>

void diceroll_ui_draw(const MnemonicState *state);
void diceroll_ui_update(const MnemonicState *state);
void diceroll_ui_draw_error(const char *message);
DicerollButton diceroll_ui_hit_test(uint16_t x, uint16_t y);
int diceroll_ui_select_word_at(const MnemonicState *state,
                               uint16_t x, uint16_t y);
void diceroll_ui_show_hold_progress(DicerollButton button,
                                    uint32_t elapsed_ms,
                                    uint32_t required_ms);
void diceroll_ui_clear_hold_progress(DicerollButton button,
                                     int phrase_complete);

#endif
