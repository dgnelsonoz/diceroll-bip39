#ifndef DICEROLL_UI_MODEL_H
#define DICEROLL_UI_MODEL_H

#include <stdint.h>

#include "mnemonic_state.h"

void diceroll_format_partial_bits( const MnemonicState *state, char *bits,
                                   uint8_t target_bits );
void diceroll_format_index_bits( uint16_t index, char *bits,
                                 int checksum_word );

#endif
