#include "mnemonic_state.h"
#include "sha256.h"

#include <stddef.h>

static uint8_t get_entropy_bit( const MnemonicState *state, uint16_t bit_position )
{
    uint16_t byte_position = bit_position / 8U;
    uint8_t shift = ( uint8_t )( 7U - ( bit_position % 8U ) );

    return ( uint8_t )( ( state->entropy[ byte_position ] >> shift ) & 1U );
}

static void clear_entropy_bit( MnemonicState *state, uint16_t bit_position )
{
    uint16_t byte_position = bit_position / 8U;
    uint8_t shift = ( uint8_t )( 7U - ( bit_position % 8U ) );

    state->entropy[ byte_position ] &= ( uint8_t )~( 1U << shift );
}

void mnemonic_state_init( MnemonicState *state )
{
    ( void )mnemonic_state_init_words( state, MNEMONIC_WORD_COUNT );
}

int mnemonic_state_init_words( MnemonicState *state, uint8_t word_count )
{
    volatile uint8_t *entropy;
    size_t ii;

    if( state == NULL )
        return -1;
    if( word_count != 12U && word_count != 24U )
        return -2;

    entropy = state->entropy;
    for( ii = 0; ii < MNEMONIC_ENTROPY_BYTES; ++ii )
        entropy[ ii ] = 0;
    state->bit_count = 0;
    state->word_count = word_count;
    state->entropy_bits = word_count == 12U ? 128U : 256U;
    return 0;
}

int mnemonic_state_add_flip( MnemonicState *state, uint8_t bit )
{
    uint16_t byte_position;
    uint8_t shift;

    if( state == NULL )
        return -1;
    if( bit > 1U )
        return -2;
    if( state->bit_count >= state->entropy_bits )
        return -3;

    byte_position = state->bit_count / 8U;
    shift = ( uint8_t )( 7U - ( state->bit_count % 8U ) );
    if( bit != 0U )
        state->entropy[ byte_position ] |= ( uint8_t )( 1U << shift );
    else
        state->entropy[ byte_position ] &= ( uint8_t )~( 1U << shift );
    ++state->bit_count;

    return 0;
}

int mnemonic_state_backspace( MnemonicState *state )
{
    if( state == NULL )
        return -1;
    if( mnemonic_state_entropy_complete( state ) )
        return 0;
    if( state->bit_count == 0U )
        return 0;

    --state->bit_count;
    clear_entropy_bit( state, state->bit_count );

    return 1;
}

uint16_t mnemonic_state_get_bit_count( const MnemonicState *state )
{
    return state == NULL ? 0U : state->bit_count;
}

uint8_t mnemonic_state_get_completed_word_count( const MnemonicState *state )
{
    uint16_t completed;

    if( state == NULL )
        return 0U;

    completed = state->bit_count / MNEMONIC_WORD_BITS;
    if( completed >= state->word_count )
        completed = state->word_count - 1U;
    return ( uint8_t )completed;
}

uint8_t mnemonic_state_get_current_word_number( const MnemonicState *state )
{
    uint8_t completed = mnemonic_state_get_completed_word_count( state );

    if( completed >= state->word_count - 1U )
        return state->word_count;
    return ( uint8_t )( completed + 1U );
}

uint8_t mnemonic_state_get_current_word_bit_count( const MnemonicState *state )
{
    if( state == NULL )
        return 0U;
    if( state->bit_count >= ( state->word_count - 1U ) * MNEMONIC_WORD_BITS )
        return ( uint8_t )( state->bit_count - ( state->word_count - 1U ) * MNEMONIC_WORD_BITS );
    return ( uint8_t )( state->bit_count % MNEMONIC_WORD_BITS );
}

int mnemonic_state_get_word_index( const MnemonicState *state, uint8_t word_number, uint16_t *index )
{
    uint16_t start_bit;
    uint16_t value = 0;
    uint8_t ii;

    if( state == NULL || index == NULL )
        return -1;
    if( word_number == 0U || word_number >= state->word_count )
        return -2;

    start_bit = ( uint16_t )( word_number - 1U ) * MNEMONIC_WORD_BITS;
    if( state->bit_count < start_bit + MNEMONIC_WORD_BITS )
        return -3;

    for( ii = 0; ii < MNEMONIC_WORD_BITS; ++ii )
        value = ( uint16_t )( ( value << 1 ) |
                              get_entropy_bit( state, start_bit + ii ) );
    *index = value;

    return 0;
}

int mnemonic_state_get_final_word_index( const MnemonicState *state, uint16_t *index )
{
    uint8_t digest[ SHA256_DIGEST_SIZE ];
    volatile uint8_t *wipe;
    size_t ii;

    if( state == NULL || index == NULL )
        return -1;
    if( !mnemonic_state_entropy_complete( state ) )
        return -2;

    sha256( state->entropy, state->entropy_bits / 8U, digest );
    if( state->word_count == 12U )
        *index = ( uint16_t )( ( ( uint16_t )( state->entropy[ 15 ] & 0x7fU ) << 4 ) | ( digest[ 0 ] >> 4 ) );
    else
        *index = ( uint16_t )( ( ( uint16_t )( state->entropy[ 31 ] & 0x07U ) << 8 ) | digest[ 0 ] );

    wipe = digest;
    for( ii = 0; ii < SHA256_DIGEST_SIZE; ++ii )
        wipe[ ii ] = 0;

    return 0;
}

int mnemonic_state_entropy_complete( const MnemonicState *state )
{
    return state != NULL && state->bit_count == state->entropy_bits;
}

uint8_t mnemonic_state_get_word_count( const MnemonicState *state )
{
    return state == NULL ? 0U : state->word_count;
}

uint8_t mnemonic_state_get_final_entropy_bit_count( const MnemonicState *state )
{
    return state != NULL && state->word_count == 12U ? 7U : 3U;
}
