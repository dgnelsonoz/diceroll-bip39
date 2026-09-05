#include "ui_model.h"

void diceroll_format_partial_bits( const MnemonicState *state, char *bits, uint8_t target_bits )
{
    uint8_t entered = mnemonic_state_get_current_word_bit_count( state );
    uint16_t start = ( uint16_t )( state->bit_count - entered );

    for( uint8_t bit = 0U; bit < target_bits; ++bit )
    {
        if( bit < entered )
        {
            uint16_t position = ( uint16_t )( start + bit );
            uint8_t value = ( uint8_t )( ( state->entropy[ position / 8U ] >>
                                           ( 7U - position % 8U ) ) & 1U );
            bits[ bit ] = value != 0U ? '1' : '0';
        }
        else
            bits[ bit ] = '-';
    }
    bits[ target_bits ] = '\0';
}

void diceroll_format_index_bits( uint16_t index, char *bits, int checksum_word )
{
    uint8_t output = 0U;

    for( uint8_t source_bit = 0U; source_bit < MNEMONIC_WORD_BITS;
            ++source_bit )
    {
        if( checksum_word && source_bit == 3U )
            bits[ output++ ] = '|';
        bits[ output++ ] = ( index & ( 1U << ( 10U - source_bit ) ) ) != 0U
                           ? '1' : '0';
    }
    bits[ output ] = '\0';
}
