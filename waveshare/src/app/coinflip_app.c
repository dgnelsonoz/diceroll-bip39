#include "coinflip_graphics.h"
#include "coinflip_app.h"
#include "coinflip_ui.h"
#include "bip39_lookup.h"
#include "layout.h"
#include "mnemonic_state.h"
#include "rp2350_clock.h"
#include "utf8.h"

#include "bsp_gt911.h"
#include "bsp_i2c.h"
#include "bsp_st7262.h"
#include "pico/stdlib.h"
#include "pio_rgb.h"
#include "rp_pico_alloc.h"

#include <stdio.h>
#include <stdlib.h>

enum
{
    LCD_WIDTH = DICEROLL_DISPLAY_WIDTH,
    LCD_HEIGHT = DICEROLL_DISPLAY_HEIGHT,
    TRANSFER_PIXELS = DICEROLL_DISPLAY_WIDTH * 120U,
    TEST_SYSTEM_CLOCK_MHZ = 260U,
    TOUCH_RELEASE_SAMPLES = 3U
};

static const uint16_t BLACK = 0x0000U;
static const uint16_t WHITE = 0xffffU;
static const uint16_t CYAN = 0x07ffU;
static const uint16_t GREEN = 0x07e0U;
static const uint16_t DARK_RED = 0x7800U;
static const uint16_t ORANGE = 0xfd20U;
static const uint16_t LIGHT_GREY = 0xc618U;
static const uint16_t DARK_GREY = 0x4208U;

static bsp_display_interface_t *display;
static uint16_t *framebuffer;
static uint8_t selected_word;
static uint16_t transfer_buffer1[ TRANSFER_PIXELS ];
static uint16_t transfer_buffer2[ TRANSFER_PIXELS ];

typedef char wave_layout_width_must_match[
    LCD_WIDTH == DICEROLL_DISPLAY_WIDTH ? 1 : -1];
typedef char wave_layout_height_must_match[
    LCD_HEIGHT == DICEROLL_DISPLAY_HEIGHT ? 1 : -1];

static void format_partial_bits( const MnemonicState *state, char *bits,
                                uint8_t target_bits )
{
    uint8_t entered = mnemonic_state_get_current_word_bit_count( state );
    uint16_t start = ( uint16_t )( state->bit_count - entered );

    for( uint8_t bit = 0; bit < target_bits; ++bit )
    {
        if( bit < entered )
        {
            uint16_t position = ( uint16_t )( start + bit );
            uint8_t value = ( uint8_t )( ( state->entropy[ position / 8U ] >> ( 7U - position % 8U ) ) & 1U );
            bits[ bit ] = value != 0U ? '1' : '0';
        }
        else
            bits[ bit ] = '-';
    }
    bits[ target_bits ] = '\0';
}

static void format_index_bits( uint16_t index, char *bits, bool checksum_word )
{
    uint8_t output = 0;
    for( uint8_t source_bit = 0; source_bit < MNEMONIC_WORD_BITS; ++source_bit )
    {
        if( checksum_word && source_bit == 3U )
            bits[ output++ ] = '|';
        bits[ output++ ] = ( index & ( 1U << ( 10U - source_bit ) ) ) != 0U
                         ? '1' : '0';
    }
    bits[ output ] = '\0';
}

static uint8_t utf8_character_count( const char *text )
{
    const char *cursor = text;
    uint32_t codepoint;
    uint8_t count = 0U;

    while( coinflip_utf8_next( &cursor, &codepoint ) > 0 )
        if( codepoint != 0x0300U && codepoint != 0x0301U && codepoint != 0x0303U )
            ++count;
    return count;
}

static void draw_word_cell( CoinflipCanvas *canvas, const MnemonicState *state,
                           uint8_t word )
{
    char label[ 24 ];
    char list_number[ 6 ];
    uint16_t index;
    uint16_t x;
    uint16_t y;
    uint8_t current_word = mnemonic_state_get_current_word_number( state );
    int has_word = word < 24U
                   ? mnemonic_state_get_word_index( state, word, &index ) == 0
                   : mnemonic_state_get_final_word_index( state, &index ) == 0;

    diceroll_layout_word_cell( word, &x, &y );
    coinflip_graphics_fill_rect( canvas, ( uint16_t )( x + 1U ), y,
                                DICEROLL_WORD_COLUMN_WIDTH - 1U,
                                DICEROLL_WORD_ROW_HEIGHT - 1U, BLACK );
    if( has_word )
    {
        const char *word_text = bip39_get_word_by_index( index );
        uint8_t characters = utf8_character_count( word_text );
        const char *separator = word <= 6U ? " " : word < 10U ? "  " : " ";
        int offset = snprintf( label, sizeof( label ), "%u%s%s", word,
                               separator, word_text );
        while( characters++ < 8U && offset < ( int )sizeof( label ) - 1 )
            label[ offset++ ] = ' ';
        ( void )offset;
        snprintf( list_number, sizeof( list_number ), "%u", index + 1U );
    }
    else if( word == current_word &&
             !mnemonic_state_entropy_complete( state ) )
    {
        snprintf( label, sizeof( label ), "%u%s[%s]", word,
                 word <= 6U ? " " : word < 10U ? "  " : " ",
                 mnemonic_state_get_current_word_bit_count( state ) == 0U
                 ? "ready" : "in progress" );
    }
    else if( word == 24U )
        snprintf( label, sizeof( label ), "%u%s[CHECKSUM]", word,
                 word <= 6U ? " " : word < 10U ? "  " : " " );
    else
        snprintf( label, sizeof( label ), "%u", word );
    coinflip_graphics_text( canvas, ( uint16_t )( x + 10U ),
                           ( uint16_t )( y + 10U ), label, 1,
                           word == current_word &&
                           !mnemonic_state_entropy_complete( state )
                           ? CYAN : WHITE, BLACK );
    if( has_word )
        coinflip_graphics_text( canvas,
                               ( uint16_t )( x + 190U - strlen( list_number ) * 11U ),
                               ( uint16_t )( y + 10U ), list_number, 1,
                               word == current_word &&
                               !mnemonic_state_entropy_complete( state )
                               ? CYAN : WHITE, BLACK );
    if( word == selected_word )
    {
        coinflip_graphics_draw_rect( canvas, ( uint16_t )( x + 2U ), ( uint16_t )( y + 2U ), 196, 32,
                                    0xffe0U );
    }
}

static void draw_status( CoinflipCanvas *canvas, const MnemonicState *state )
{
    char bits[ 12 ];
    char number[ 3 ];
    uint8_t current_word = mnemonic_state_get_current_word_number( state );
    uint8_t entered = mnemonic_state_get_current_word_bit_count( state );
    uint8_t required = current_word == 24U ? 3U : 11U;
    uint8_t completed = mnemonic_state_get_completed_word_count( state );
    bool word_boundary = state->bit_count > 0U &&
                         state->bit_count < MNEMONIC_ENTROPY_BITS &&
                         state->bit_count % MNEMONIC_WORD_BITS == 0U;

    if( word_boundary )
    {
        --current_word;
        required = current_word == 24U ? 3U : 11U;
        entered = required;
    }

    coinflip_graphics_fill_rect( canvas, 0, 248, 800, 80, BLACK );
    if( mnemonic_state_entropy_complete( state ) )
    {
        coinflip_graphics_text_centered( canvas, 260,
                                        "PHRASE COMPLETE - 24 WORDS", 1, GREEN, BLACK );
    }
    else
    {
        if( word_boundary )
        {
            uint16_t completed_index;

            if( mnemonic_state_get_word_index( state, current_word,
                                               &completed_index ) == 0 )
                format_index_bits( completed_index, bits, false );
        }
        else
            format_partial_bits( state, bits, required );
        coinflip_graphics_text20( canvas, 10, 258, "WORD", WHITE, BLACK );
        snprintf( number, sizeof( number ), "%u", current_word );
        coinflip_graphics_text20( canvas,
                                 ( uint16_t )( 116U - strlen( number ) * 14U ),
                                 258, number, WHITE, BLACK );
        coinflip_graphics_text20( canvas, 116, 258, "/", WHITE, BLACK );
        coinflip_graphics_text20( canvas, 130, 258, "24", WHITE, BLACK );
        coinflip_graphics_text20( canvas, 200, 258, "FLIP", WHITE, BLACK );
        snprintf( number, sizeof( number ), "%u", entered );
        coinflip_graphics_text20( canvas,
                                 ( uint16_t )( 306U - strlen( number ) * 14U ),
                                 258, number, WHITE, BLACK );
        coinflip_graphics_text20( canvas, 306, 258, "/", WHITE, BLACK );
        snprintf( number, sizeof( number ), "%u", required );
        coinflip_graphics_text20( canvas, 320, 258, number, WHITE, BLACK );
        coinflip_graphics_text20( canvas, 470, 258, "BITS", WHITE, BLACK );
        coinflip_graphics_text20( canvas, 540, 258, bits, WHITE, BLACK );
    }

    if( mnemonic_state_entropy_complete( state ) )
        completed = MNEMONIC_WORD_COUNT;
    if( completed > 0U )
    {
        char verification[ 72 ];
        char verification_bits[ MNEMONIC_WORD_BITS + 2U ];
        uint8_t detail_word = selected_word != 0U ? selected_word : completed;
        uint16_t detail_index;
        int result = detail_word == MNEMONIC_WORD_COUNT
                     ? mnemonic_state_get_final_word_index( state, &detail_index )
                     : mnemonic_state_get_word_index( state, detail_word,
                         &detail_index );
        if( result == 0 )
        {
            format_index_bits( detail_index, verification_bits,
                              detail_word == MNEMONIC_WORD_COUNT );
            snprintf( verification, sizeof( verification ),
                     "WORD %u: %s = INDEX %u = LIST %u = %s",
                     detail_word, verification_bits, detail_index,
                     detail_index + 1U,
                     bip39_get_word_by_index( detail_index ) );
            coinflip_graphics_text( canvas, 20, 296, verification, 1,
                                   LIGHT_GREY, BLACK );
        }
    }
}

static void update_state_regions( const MnemonicState *state,
                                 uint8_t previous_word,
                                 uint8_t previous_entered )
{
    CoinflipCanvas canvas = { framebuffer, LCD_WIDTH, LCD_HEIGHT };
    uint8_t current_word = mnemonic_state_get_current_word_number( state );
    uint8_t entered = mnemonic_state_get_current_word_bit_count( state );
    char number[ 3 ];
    char bit[ 2 ] = { '-', '\0' };

    /* The 256th flip completes the entropy. Word 24 then becomes the three
       user-entered bits plus the eight-bit SHA-256 checksum. */
    if( mnemonic_state_entropy_complete( state ) )
    {
        draw_word_cell( &canvas, state, 24U );
        draw_status( &canvas, state );
        return;

    }

    if( current_word != previous_word )
    {
        draw_word_cell( &canvas, state, previous_word );
        draw_word_cell( &canvas, state, current_word );
        draw_status( &canvas, state );

        if( current_word > previous_word && previous_word <= MNEMONIC_DIRECT_WORDS )
        {
            char previous_bits[ MNEMONIC_WORD_BITS + 1U ];
            uint16_t previous_index;

            if( mnemonic_state_get_word_index( state, previous_word,
                                               &previous_index ) == 0 )
            {
                format_index_bits( previous_index, previous_bits, false );
                coinflip_graphics_text20( &canvas, 540, 258, previous_bits,
                                         WHITE, BLACK );
            }
        }
        return;

    }

    /* The word-cell label changes only at the READY/IN PROGRESS boundary. */
    if( ( previous_entered == 0U ) != ( entered == 0U ) )
        draw_word_cell( &canvas, state, current_word );

    if( previous_entered == 0U && entered > 0U )
    {
        draw_status( &canvas, state );
        return;
    }

    if( entered == 0U && previous_entered > 0U && current_word > 1U )
    {
        char previous_bits[ MNEMONIC_WORD_BITS + 1U ];
        uint16_t previous_index;

        draw_status( &canvas, state );
        if( mnemonic_state_get_word_index( state, current_word - 1U,
                                           &previous_index ) == 0 )
        {
            format_index_bits( previous_index, previous_bits, false );
            coinflip_graphics_text20( &canvas, 540, 258, previous_bits,
                                     WHITE, BLACK );
        }
        return;
    }

    snprintf( number, sizeof( number ), "%u", entered );
    coinflip_graphics_fill_rect( &canvas, 260, 258, 46, 24, BLACK );
    coinflip_graphics_text20( &canvas,
                             ( uint16_t )( 306U - strlen( number ) * 14U ),
                             258, number, WHITE, BLACK );

    if( entered > previous_entered )
    {
        uint16_t position = ( uint16_t )( state->bit_count - 1U );
        bit[ 0 ] = ( ( state->entropy[ position / 8U ] >> ( 7U - position % 8U ) ) & 1U ) != 0U ? '1' : '0';

        if( previous_entered == 0U )
        {
            char partial_bits[ MNEMONIC_WORD_BITS + 1U ];
            uint8_t required = current_word == MNEMONIC_WORD_COUNT ? 3U : 11U;

            coinflip_graphics_fill_rect( &canvas, 540, 258, 200, 24, BLACK );
            format_partial_bits( state, partial_bits, required );
            coinflip_graphics_text20( &canvas, 540, 258, partial_bits,
                                     WHITE, BLACK );
        }
        else
        {
            coinflip_graphics_text20( &canvas, ( uint16_t )( 540U + previous_entered * 14U ),
                                     258, bit, WHITE, BLACK );
        }
    }
    else if( entered < previous_entered )
    {
        coinflip_graphics_text20( &canvas, ( uint16_t )( 540U + entered * 14U ),
                                 258, bit, WHITE, BLACK );
    }
}

static void clear_word_selection( const MnemonicState *state )
{
    CoinflipCanvas canvas = { framebuffer, LCD_WIDTH, LCD_HEIGHT };
    uint8_t previous_selected = selected_word;

    if( previous_selected == 0U )
        return;

    selected_word = 0U;
    draw_word_cell( &canvas, state, previous_selected );
    draw_status( &canvas, state );
}

static void draw_coinflip_screen( uint16_t *pixels, const MnemonicState *state )
{
    CoinflipCanvas canvas = { pixels, LCD_WIDTH, LCD_HEIGHT };

    coinflip_graphics_clear( &canvas, BLACK );
    coinflip_graphics_text24_centered( &canvas, 3, "DICE ROLL TO BIP-39",
                                      WHITE, BLACK );
    coinflip_graphics_fill_rect( &canvas, 0, DICEROLL_TITLE_HEIGHT - 1U,
                                DICEROLL_DISPLAY_WIDTH, 1, WHITE );

    for( uint16_t column = 1; column < 4; ++column )
        coinflip_graphics_fill_rect( &canvas,
                                    column * DICEROLL_WORD_COLUMN_WIDTH,
                                    DICEROLL_WORD_GRID_TOP, 1,
                                    DICEROLL_WORD_GRID_HEIGHT, WHITE );
    for( uint16_t row = 1; row <= 6; ++row )
    {
        coinflip_graphics_fill_rect( &canvas, 0,
                                    ( uint16_t )( DICEROLL_WORD_GRID_TOP +
                                                  row * DICEROLL_WORD_ROW_HEIGHT - 1U ),
                                    DICEROLL_DISPLAY_WIDTH, 1, WHITE );
    }
    for( uint8_t word = 1; word <= 24; ++word )
        draw_word_cell( &canvas, state, word );

    draw_status( &canvas, state );

    coinflip_graphics_fill_rect( &canvas, 0, DICEROLL_BUTTON_TOP,
                                DICEROLL_RESTART_WIDTH,
                                DICEROLL_BUTTON_HEIGHT,
                                DARK_RED );
    coinflip_graphics_fill_rect( &canvas, DICEROLL_RESTART_WIDTH,
                                DICEROLL_BUTTON_TOP, DICEROLL_BACK_WIDTH,
                                DICEROLL_BUTTON_HEIGHT,
                                ORANGE );
    coinflip_graphics_fill_rect( &canvas,
                                DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH,
                                DICEROLL_BUTTON_TOP, DICEROLL_BIT_BUTTON_WIDTH,
                                DICEROLL_BUTTON_HEIGHT,
                                LIGHT_GREY );
    coinflip_graphics_fill_rect( &canvas,
                                DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH +
                                DICEROLL_BIT_BUTTON_WIDTH,
                                DICEROLL_BUTTON_TOP, DICEROLL_BIT_BUTTON_WIDTH,
                                DICEROLL_BUTTON_HEIGHT,
                                DARK_GREY );
    coinflip_graphics_fill_rect( &canvas, DICEROLL_RESTART_WIDTH,
                                DICEROLL_BUTTON_TOP, 1,
                                DICEROLL_BUTTON_HEIGHT, BLACK );
    coinflip_graphics_fill_rect( &canvas,
                                DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH,
                                DICEROLL_BUTTON_TOP, 1,
                                DICEROLL_BUTTON_HEIGHT, BLACK );
    coinflip_graphics_fill_rect( &canvas,
                                DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH +
                                DICEROLL_BIT_BUTTON_WIDTH,
                                DICEROLL_BUTTON_TOP, 1,
                                DICEROLL_BUTTON_HEIGHT, BLACK );

    coinflip_graphics_text12( &canvas, 51, 350, "HOLD", WHITE,
                           DARK_RED );
    coinflip_graphics_text_default( &canvas, 26, 390, "RESTART", WHITE,
                           DARK_RED );
    coinflip_graphics_text12( &canvas, 181, 350, "HOLD", BLACK,
                           ORANGE );
    coinflip_graphics_text_default( &canvas, 168, 390, "BACK", BLACK,
                           ORANGE );
    coinflip_graphics_text24( &canvas, 352, 354, "HEADS", BLACK,
                             LIGHT_GREY );
    coinflip_graphics_text24( &canvas, 387, 406, "0", BLACK,
                             LIGHT_GREY );
    coinflip_graphics_text24( &canvas, 622, 354, "TAILS", WHITE,
                             DARK_GREY );
    coinflip_graphics_text24( &canvas, 657, 406, "1", WHITE,
                             DARK_GREY );

}

static void present( const MnemonicState *state )
{
    /* Coinflip uses a single continuously scanned framebuffer. Updating it in
       place avoids a full-frame double-buffer switch on every coin flip. */
    draw_coinflip_screen( framebuffer, state );
}

void coinflip_app_run( void )
{
    bsp_touch_interface_t *touch;
    bsp_touch_data_t touch_data;
    pio_rgb_info_t rgb =
    {
        .width = LCD_WIDTH, .height = LCD_HEIGHT,
        .transfer_size = TRANSFER_PIXELS,
        .pclk_freq = BSP_LCD_PCLK_FREQ,
        .mode = { false, true, true },
    };
    bsp_display_info_t display_info =
    {
        .width = LCD_WIDTH, .height = LCD_HEIGHT,
        .brightness = 100, .user_data = &rgb,
    };
    bsp_touch_info_t touch_info =
    {
        .width = LCD_WIDTH, .height = LCD_HEIGHT, .rotation = 0,
    };
    MnemonicState state;
    bool touch_down = false;
    bool action_done = false;
    uint8_t held_button = 0;
    uint8_t release_samples = 0;
    uint16_t hold_progress = 0;
    absolute_time_t press_started = nil_time;

    rp2350_set_system_clock( TEST_SYSTEM_CLOCK_MHZ );
    mnemonic_state_init( &state );
    rgb.framebuffer1 = rp_mem_malloc( LCD_WIDTH * LCD_HEIGHT * sizeof( uint16_t ) );
    rgb.framebuffer2 = NULL;
    rgb.transfer_buffer1 = transfer_buffer1;
    rgb.transfer_buffer2 = transfer_buffer2;
    if( rgb.framebuffer1 == NULL )
        panic( "display allocation failed" );
    framebuffer = rgb.framebuffer1;
    if( !bsp_display_new_st7262( &display, &display_info ) )
        panic( "display creation failed" );
    display->init();
    present( &state );

    bsp_i2c_init();
    if( !bsp_touch_new_gt911( &touch, &touch_info ) )
        panic( "touch creation failed" );
    touch->init();

    while( true )
    {
        touch->read();
        bool pressed = touch->get_data( &touch_data ) && touch_data.points > 0U;

        if( pressed )
            release_samples = 0;

        if( pressed && !touch_down )
        {
            touch_down = true;
            action_done = false;
            press_started = get_absolute_time();
            held_button = 0;
            hold_progress = 0;

            if( touch_data.coords[ 0 ].y >= DICEROLL_BUTTON_TOP )
            {
                uint16_t x = touch_data.coords[ 0 ].x;
                held_button = ( uint8_t )diceroll_layout_button_at(
                    x, touch_data.coords[ 0 ].y );
                if( ( held_button == 1U && mnemonic_state_get_bit_count( &state ) == 0U ) ||
                    ( held_button == 2U && ( mnemonic_state_get_bit_count( &state ) == 0U ||
                         mnemonic_state_entropy_complete( &state ) ) ) || ( held_button >= 3U &&
                         mnemonic_state_entropy_complete( &state ) ) )
                {
                    held_button = 0;
                    action_done = true;
                }
            }
            else if( touch_data.coords[ 0 ].y >= DICEROLL_WORD_GRID_TOP &&
                     touch_data.coords[ 0 ].y < DICEROLL_STATUS_TOP )
            {
                uint8_t column = ( uint8_t )( touch_data.coords[ 0 ].x /
                                               DICEROLL_WORD_COLUMN_WIDTH );
                uint8_t row = ( uint8_t )( ( touch_data.coords[ 0 ].y -
                                             DICEROLL_WORD_GRID_TOP ) /
                                           DICEROLL_WORD_ROW_HEIGHT );
                uint8_t word = ( uint8_t )( column * 6U + row + 1U );
                uint8_t completed = mnemonic_state_entropy_complete( &state )
                                    ? MNEMONIC_WORD_COUNT
                                    : mnemonic_state_get_completed_word_count( &state );
                if( word <= completed )
                {
                    CoinflipCanvas canvas = { framebuffer, LCD_WIDTH,
                                              LCD_HEIGHT
                                            };
                    uint8_t previous_selected = selected_word;
                    selected_word = word;
                    if( previous_selected != 0U &&
                            previous_selected != selected_word )
                    {
                        draw_word_cell( &canvas, &state, previous_selected );
                    }
                    draw_word_cell( &canvas, &state, selected_word );
                    draw_status( &canvas, &state );
                    action_done = true;
                }
            }

            if( held_button == 3U )
            {
                uint8_t previous_word = mnemonic_state_get_current_word_number( &state );
                uint8_t previous_entered = mnemonic_state_get_current_word_bit_count( &state );
                clear_word_selection( &state );
                mnemonic_state_add_flip( &state, 0 );
                action_done = true;
                update_state_regions( &state, previous_word, previous_entered );
            }
            else if( held_button == 4U )
            {
                uint8_t previous_word = mnemonic_state_get_current_word_number( &state );
                uint8_t previous_entered = mnemonic_state_get_current_word_bit_count( &state );
                clear_word_selection( &state );
                mnemonic_state_add_flip( &state, 1 );
                action_done = true;
                update_state_regions( &state, previous_word, previous_entered );
            }
        }
        else if( pressed && touch_down && !action_done )
        {
            int64_t held_us = absolute_time_diff_us( press_started,
                                                    get_absolute_time() );
            if( ( held_button == 1U || held_button == 2U ) && ( touch_data.coords[ 0 ].y < 328U || ( held_button == 1U && touch_data.coords[ 0 ].x >= 130U ) || ( held_button == 2U && ( touch_data.coords[ 0 ].x < 130U ||
                       touch_data.coords[ 0 ].x >= 260U ) ) ) )
            {
                coinflip_ui_clear_hold_progress( framebuffer, held_button );
                action_done = true;
                continue;

            }
            hold_progress = coinflip_ui_show_hold_progress(
                                framebuffer, held_button, held_us, hold_progress );
            if( held_button == 1U && held_us >= 1000000 )
            {
                mnemonic_state_init( &state );
                selected_word = 0U;
                action_done = true;
                present( &state );
            }
            else if( held_button == 2U && held_us >= 500000 )
            {
                uint8_t previous_word = mnemonic_state_get_current_word_number( &state );
                uint8_t previous_entered = mnemonic_state_get_current_word_bit_count( &state );
                mnemonic_state_backspace( &state );
                action_done = true;
                coinflip_ui_clear_hold_progress( framebuffer, held_button );
                update_state_regions( &state, previous_word, previous_entered );
            }
        }
        else if( !pressed && touch_down )
        {
            if( release_samples < TOUCH_RELEASE_SAMPLES )
                ++release_samples;

            if( release_samples >= TOUCH_RELEASE_SAMPLES )
            {
                coinflip_ui_clear_hold_progress( framebuffer, held_button );
                touch_down = false;
                action_done = false;
                held_button = 0;
                hold_progress = 0;
            }
        }
        sleep_ms( 5 );
    }
}
