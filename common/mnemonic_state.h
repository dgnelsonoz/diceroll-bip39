#ifndef MNEMONIC_STATE_H
#define MNEMONIC_STATE_H

#include <stdint.h>

enum
{
    MNEMONIC_ENTROPY_BITS = 256,
    MNEMONIC_ENTROPY_BYTES = MNEMONIC_ENTROPY_BITS / 8,
    MNEMONIC_DIRECT_WORDS = 23,
    MNEMONIC_WORD_BITS = 11,
    MNEMONIC_WORD_COUNT = 24
};

typedef struct
{
    uint8_t entropy[ MNEMONIC_ENTROPY_BYTES ];
    uint16_t bit_count;
} MnemonicState;

void mnemonic_state_init( MnemonicState *state );
int mnemonic_state_add_flip( MnemonicState *state, uint8_t bit );
int mnemonic_state_backspace( MnemonicState *state );
uint16_t mnemonic_state_get_bit_count( const MnemonicState *state );
uint8_t mnemonic_state_get_completed_word_count( const MnemonicState *state );
uint8_t mnemonic_state_get_current_word_number( const MnemonicState *state );
uint8_t mnemonic_state_get_current_word_bit_count( const MnemonicState *state );
int mnemonic_state_get_word_index( const MnemonicState *state, uint8_t word_number, uint16_t *index );
int mnemonic_state_get_final_word_index( const MnemonicState *state, uint16_t *index );
int mnemonic_state_entropy_complete( const MnemonicState *state );

#endif
