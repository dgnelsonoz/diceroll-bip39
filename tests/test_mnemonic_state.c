#include "mnemonic_state.h"
#include "bip39_lookup.h"
#include "sha256.h"
#include "utf8.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void add_repeating_bits( MnemonicState *state, uint16_t count )
{
    uint16_t ii;

    for( ii = 0; ii < count; ++ii )
        assert( mnemonic_state_add_flip( state, ( uint8_t )( ii & 1U ) ) == 0 );
}

static void test_initial_state_and_validation( void )
{
    MnemonicState state;

    mnemonic_state_init( &state );
    assert( mnemonic_state_get_bit_count( &state ) == 0 );
    assert( mnemonic_state_get_completed_word_count( &state ) == 0 );
    assert( mnemonic_state_get_current_word_number( &state ) == 1 );
    assert( mnemonic_state_get_current_word_bit_count( &state ) == 0 );
    assert( !mnemonic_state_entropy_complete( &state ) );
    assert( mnemonic_state_add_flip( &state, 2 ) == -2 );
    assert( mnemonic_state_get_bit_count( &state ) == 0 );
    assert( mnemonic_state_backspace( &state ) == 0 );
}

static void test_word_completion_and_index( void )
{
    MnemonicState state;
    uint16_t index = 0;

    mnemonic_state_init( &state );
    add_repeating_bits( &state, 11 );

    assert( mnemonic_state_get_completed_word_count( &state ) == 1 );
    assert( mnemonic_state_get_current_word_number( &state ) == 2 );
    assert( mnemonic_state_get_current_word_bit_count( &state ) == 0 );
    assert( mnemonic_state_get_word_index( &state, 1, &index ) == 0 );
    assert( index == 0x2AA );
    assert( mnemonic_state_get_word_index( &state, 2, &index ) == -3 );
}

static void test_backspace_crosses_word_boundaries( void )
{
    MnemonicState state;
    uint16_t ii;

    mnemonic_state_init( &state );
    add_repeating_bits( &state, 16 );
    for( ii = 0; ii < 6; ++ii )
        assert( mnemonic_state_backspace( &state ) == 1 );
    assert( mnemonic_state_get_bit_count( &state ) == 10 );
    assert( mnemonic_state_get_completed_word_count( &state ) == 0 );
    assert( mnemonic_state_get_current_word_bit_count( &state ) == 10 );
}

static void test_backspace_reopens_completed_word( void )
{
    MnemonicState state;
    uint16_t ii;

    mnemonic_state_init( &state );
    add_repeating_bits( &state, 11 );
    for( ii = 0; ii < 11; ++ii )
        assert( mnemonic_state_backspace( &state ) == 1 );
    assert( mnemonic_state_get_bit_count( &state ) == 0 );
    assert( mnemonic_state_backspace( &state ) == 0 );
}

static void test_final_entropy_bits( void )
{
    MnemonicState state;

    mnemonic_state_init( &state );
    add_repeating_bits( &state, 253 );
    assert( mnemonic_state_get_completed_word_count( &state ) == 23 );
    assert( mnemonic_state_get_current_word_number( &state ) == 24 );
    assert( mnemonic_state_get_current_word_bit_count( &state ) == 0 );

    add_repeating_bits( &state, 3 );
    assert( mnemonic_state_get_current_word_bit_count( &state ) == 3 );
    assert( mnemonic_state_entropy_complete( &state ) );
    assert( mnemonic_state_add_flip( &state, 0 ) == -3 );

    assert( mnemonic_state_backspace( &state ) == 0 );
    assert( mnemonic_state_get_bit_count( &state ) == 256 );
    assert( mnemonic_state_entropy_complete( &state ) );
}

static void test_secure_restart( void )
{
    MnemonicState state;
    uint16_t ii;

    mnemonic_state_init( &state );
    for( ii = 0; ii < MNEMONIC_ENTROPY_BITS; ++ii )
        assert( mnemonic_state_add_flip( &state, 1 ) == 0 );
    mnemonic_state_init( &state );

    assert( mnemonic_state_get_bit_count( &state ) == 0 );
    for( ii = 0; ii < MNEMONIC_ENTROPY_BYTES; ++ii )
        assert( state.entropy[ ii ] == 0 );
}

static void assert_digest( const uint8_t *message, size_t length, const uint8_t expected[ SHA256_DIGEST_SIZE ] )
{
    uint8_t actual[ SHA256_DIGEST_SIZE ];

    sha256( message, length, actual );
    assert( memcmp( actual, expected, SHA256_DIGEST_SIZE ) == 0 );
}

static void test_sha256_vectors( void )
{
    static const uint8_t empty_digest[ SHA256_DIGEST_SIZE ] =
    {
        0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
        0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
        0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
        0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55
    };
    static const uint8_t abc_digest[ SHA256_DIGEST_SIZE ] =
    {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
    };
    static const uint8_t two_block_digest[ SHA256_DIGEST_SIZE ] =
    {
        0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8,
        0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
        0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
        0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1
    };
    static const char two_block_message[ ] =
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

    assert_digest( ( const uint8_t * )"", 0, empty_digest );
    assert_digest( ( const uint8_t * )"abc", 3, abc_digest );
    assert_digest( ( const uint8_t * )two_block_message,
                   strlen( two_block_message ), two_block_digest );
}

static void test_utf8_decoder( void )
{
    const char *text = "A\xc3\xa9\xe2\x82\xac\xf0\x90\x8d\x88";
    const char *invalid = "\xc0\x80";
    uint32_t codepoint;

    assert( diceroll_utf8_next( &text, &codepoint ) == 1 );
    assert( codepoint == 0x41U );
    assert( diceroll_utf8_next( &text, &codepoint ) == 1 );
    assert( codepoint == 0xe9U );
    assert( diceroll_utf8_next( &text, &codepoint ) == 1 );
    assert( codepoint == 0x20acU );
    assert( diceroll_utf8_next( &text, &codepoint ) == 1 );
    assert( codepoint == 0x10348U );
    assert( diceroll_utf8_next( &text, &codepoint ) == 0 );

    assert( diceroll_utf8_next( &invalid, &codepoint ) == -1 );
    assert( diceroll_utf8_next( NULL, &codepoint ) == -1 );
}

static void load_entropy( MnemonicState *state, const uint8_t entropy[ MNEMONIC_ENTROPY_BYTES ] )
{
    uint16_t bit;

    mnemonic_state_init( state );
    for( bit = 0; bit < MNEMONIC_ENTROPY_BITS; ++bit )
    {
        uint8_t value = ( uint8_t )( ( entropy[ bit / 8U ] >>
                                       ( 7U - ( bit % 8U ) ) ) & 1U );
        assert( mnemonic_state_add_flip( state, value ) == 0 );
    }
}

static uint8_t hex_nibble( char value )
{
    if( value >= '0' && value <= '9' )
        return ( uint8_t )( value - '0' );
    if( value >= 'a' && value <= 'f' )
        return ( uint8_t )( value - 'a' + 10 );
    assert( 0 );
    return 0;
}

static void assert_mnemonic( const char *entropy_hex, const char *expected_mnemonic )
{
    uint8_t entropy[ MNEMONIC_ENTROPY_BYTES ];
    MnemonicState state;
    char actual[ 256 ] = { 0 };
    size_t used = 0;
    uint16_t index;
    uint8_t word;

    assert( strlen( entropy_hex ) == MNEMONIC_ENTROPY_BYTES * 2U );
    for( word = 0; word < MNEMONIC_ENTROPY_BYTES; ++word )
    {
        entropy[ word ] = ( uint8_t )( ( hex_nibble( entropy_hex[ word * 2U ] ) << 4 ) |
                                       hex_nibble( entropy_hex[ word * 2U + 1U ] ) );
    }

    load_entropy( &state, entropy );
    for( word = 1; word <= MNEMONIC_DIRECT_WORDS; ++word )
    {
        assert( mnemonic_state_get_word_index( &state, word, &index ) == 0 );
        used += ( size_t )snprintf( actual + used, sizeof( actual ) - used,
                                    "%s%s", word == 1U ? "" : " ",
                                    bip39_get_word_by_index( index ) );
        assert( used < sizeof( actual ) );
    }
    assert( mnemonic_state_get_final_word_index( &state, &index ) == 0 );
    used += ( size_t )snprintf( actual + used, sizeof( actual ) - used, " %s",
                                bip39_get_word_by_index( index ) );
    assert( used < sizeof( actual ) );
    assert( strcmp( actual, expected_mnemonic ) == 0 );
}

static void test_bip39_vectors( void )
{
    static const struct
    {
        const char *entropy;
        const char *mnemonic;
    } vectors[ ] =
    {
        {
            "00000000000000000000000000000000"
            "00000000000000000000000000000000",
            "abandon abandon abandon abandon abandon abandon abandon abandon "
            "abandon abandon abandon abandon abandon abandon abandon abandon "
            "abandon abandon abandon abandon abandon abandon abandon art"
        },
        {
            "7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f"
            "7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",
            "legal winner thank year wave sausage worth useful legal winner "
            "thank year wave sausage worth useful legal winner thank year "
            "wave sausage worth title"
        },
        {
            "80808080808080808080808080808080"
            "80808080808080808080808080808080",
            "letter advice cage absurd amount doctor acoustic avoid letter "
            "advice cage absurd amount doctor acoustic avoid letter advice "
            "cage absurd amount doctor acoustic bless"
        },
        {
            "ffffffffffffffffffffffffffffffff"
            "ffffffffffffffffffffffffffffffff",
            "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo "
            "zoo zoo zoo zoo zoo zoo zoo vote"
        },
        {
            "68a79eaca2324873eacc50cb9c6eca8c"
            "c68ea5d936f98787c60c7ebc74e6ce7c",
            "hamster diagram private dutch cause delay private meat slide "
            "toddler razor book happy fancy gospel tennis maple dilemma loan "
            "word shrug inflict delay length"
        },
        {
            "9f6a2878b2520799a44ef18bc7df394e"
            "7061a224d2c33cd015b157d746869863",
            "panda eyebrow bullet gorilla call smoke muffin taste mesh "
            "discover soft ostrich alcohol speed nation flash devote level "
            "hobby quick inner drive ghost inside"
        },
        {
            "066dca1a2bb7e8a1db2832148ce9933e"
            "ea0f3ac9548d793112d9a95c9407efad",
            "all hour make first leader extend hole alien behind guard gospel "
            "lava path output census museum junior mass reopen famous sing "
            "advance salt reform"
        },
        {
            "f585c11aec520db57dd353c69554b21a"
            "89b20fb0650966fa0a9d6f74fd989d8f",
            "void come effort suffer camp survey warrior heavy shoot primary "
            "clutch crush open amazing screen patrol group space point ten "
            "exist slush involve unfold"
        }
    };
    size_t ii;

    for( ii = 0; ii < sizeof( vectors ) / sizeof( vectors[ 0 ] ); ++ii )
        assert_mnemonic( vectors[ ii ].entropy, vectors[ ii ].mnemonic );
}

int main( void )
{
    test_initial_state_and_validation( );
    test_word_completion_and_index( );
    test_backspace_crosses_word_boundaries( );
    test_backspace_reopens_completed_word( );
    test_final_entropy_bits( );
    test_secure_restart( );
    test_sha256_vectors( );
    test_utf8_decoder( );
    test_bip39_vectors( );

    puts( "mnemonic_state tests passed" );
    return 0;
}
