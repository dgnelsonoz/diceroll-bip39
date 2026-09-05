#include "diceroll_graphics.h"
#include "diceroll_app.h"
#include "diceroll_ui.h"
#include "bip39_lookup.h"
#include "layout.h"
#include "mnemonic_state.h"
#include "ui_model.h"
#include "utf8.h"
#include "waveshare_platform.h"

#include <stdio.h>
#include <string.h>

enum
{
    LCD_WIDTH = DICEROLL_DISPLAY_WIDTH,
    LCD_HEIGHT = DICEROLL_DISPLAY_HEIGHT,
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

static uint16_t *framebuffer;
static uint8_t selected_word;

typedef char wave_layout_width_must_match[
    LCD_WIDTH == DICEROLL_DISPLAY_WIDTH ? 1 : -1];
typedef char wave_layout_height_must_match[
    LCD_HEIGHT == DICEROLL_DISPLAY_HEIGHT ? 1 : -1];

static uint8_t utf8_character_count( const char *text )
{
    const char *cursor = text;
    uint32_t codepoint;
    uint8_t count = 0U;

    while( diceroll_utf8_next( &cursor, &codepoint ) > 0 )
        if( codepoint != 0x0300U && codepoint != 0x0301U && codepoint != 0x0303U )
            ++count;
    return count;
}

static void draw_word_cell( DicerollCanvas *canvas, const MnemonicState *state,
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
    diceroll_graphics_fill_rect( canvas, ( uint16_t )( x + 1U ), y,
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
    diceroll_graphics_text( canvas, ( uint16_t )( x + 10U ),
                           ( uint16_t )( y + 10U ), label, 1,
                           word == current_word &&
                           !mnemonic_state_entropy_complete( state )
                           ? CYAN : WHITE, BLACK );
    if( has_word )
        diceroll_graphics_text( canvas,
                               ( uint16_t )( x + 190U - strlen( list_number ) * 11U ),
                               ( uint16_t )( y + 10U ), list_number, 1,
                               word == current_word &&
                               !mnemonic_state_entropy_complete( state )
                               ? CYAN : WHITE, BLACK );
    if( word == selected_word )
    {
        diceroll_graphics_draw_rect( canvas, ( uint16_t )( x + 2U ), ( uint16_t )( y + 2U ), 196, 32,
                                    0xffe0U );
    }
}

static void draw_status( DicerollCanvas *canvas, const MnemonicState *state )
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

    diceroll_graphics_fill_rect( canvas, 0, 248, 800, 80, BLACK );
    if( mnemonic_state_entropy_complete( state ) )
    {
        diceroll_graphics_text_centered( canvas, 260,
                                        "PHRASE COMPLETE - 24 WORDS", 1, GREEN, BLACK );
    }
    else
    {
        if( word_boundary )
        {
            uint16_t completed_index;

            if( mnemonic_state_get_word_index( state, current_word,
                                               &completed_index ) == 0 )
                diceroll_format_index_bits( completed_index, bits, false );
        }
        else
            diceroll_format_partial_bits( state, bits, required );
        diceroll_graphics_text20( canvas, 10, 258, "WORD", WHITE, BLACK );
        snprintf( number, sizeof( number ), "%u", current_word );
        diceroll_graphics_text20( canvas,
                                 ( uint16_t )( 116U - strlen( number ) * 14U ),
                                 258, number, WHITE, BLACK );
        diceroll_graphics_text20( canvas, 116, 258, "/", WHITE, BLACK );
        diceroll_graphics_text20( canvas, 130, 258, "24", WHITE, BLACK );
        diceroll_graphics_text20( canvas, 200, 258, "FLIP", WHITE, BLACK );
        snprintf( number, sizeof( number ), "%u", entered );
        diceroll_graphics_text20( canvas,
                                 ( uint16_t )( 306U - strlen( number ) * 14U ),
                                 258, number, WHITE, BLACK );
        diceroll_graphics_text20( canvas, 306, 258, "/", WHITE, BLACK );
        snprintf( number, sizeof( number ), "%u", required );
        diceroll_graphics_text20( canvas, 320, 258, number, WHITE, BLACK );
        diceroll_graphics_text20( canvas, 470, 258, "BITS", WHITE, BLACK );
        diceroll_graphics_text20( canvas, 540, 258, bits, WHITE, BLACK );
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
            diceroll_format_index_bits( detail_index, verification_bits,
                                        detail_word == MNEMONIC_WORD_COUNT );
            snprintf( verification, sizeof( verification ),
                     "WORD %u: %s = INDEX %u = LIST %u = %s",
                     detail_word, verification_bits, detail_index,
                     detail_index + 1U,
                     bip39_get_word_by_index( detail_index ) );
            diceroll_graphics_text( canvas, 20, 296, verification, 1,
                                   LIGHT_GREY, BLACK );
        }
    }
}

static void update_state_regions( const MnemonicState *state,
                                 uint8_t previous_word,
                                 uint8_t previous_entered )
{
    DicerollCanvas canvas = { framebuffer, LCD_WIDTH, LCD_HEIGHT };
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
                diceroll_format_index_bits( previous_index, previous_bits, false );
                diceroll_graphics_text20( &canvas, 540, 258, previous_bits,
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
            diceroll_format_index_bits( previous_index, previous_bits, false );
            diceroll_graphics_text20( &canvas, 540, 258, previous_bits,
                                     WHITE, BLACK );
        }
        return;
    }

    snprintf( number, sizeof( number ), "%u", entered );
    diceroll_graphics_fill_rect( &canvas, 260, 258, 46, 24, BLACK );
    diceroll_graphics_text20( &canvas,
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

            diceroll_graphics_fill_rect( &canvas, 540, 258, 200, 24, BLACK );
            diceroll_format_partial_bits( state, partial_bits, required );
            diceroll_graphics_text20( &canvas, 540, 258, partial_bits,
                                     WHITE, BLACK );
        }
        else
        {
            diceroll_graphics_text20( &canvas, ( uint16_t )( 540U + previous_entered * 14U ),
                                     258, bit, WHITE, BLACK );
        }
    }
    else if( entered < previous_entered )
    {
        diceroll_graphics_text20( &canvas, ( uint16_t )( 540U + entered * 14U ),
                                 258, bit, WHITE, BLACK );
    }
}

static void clear_word_selection( const MnemonicState *state )
{
    DicerollCanvas canvas = { framebuffer, LCD_WIDTH, LCD_HEIGHT };
    uint8_t previous_selected = selected_word;

    if( previous_selected == 0U )
        return;

    selected_word = 0U;
    draw_word_cell( &canvas, state, previous_selected );
    draw_status( &canvas, state );
}

static void draw_diceroll_screen( uint16_t *pixels, const MnemonicState *state )
{
    DicerollCanvas canvas = { pixels, LCD_WIDTH, LCD_HEIGHT };

    diceroll_graphics_clear( &canvas, BLACK );
    diceroll_graphics_text24_centered( &canvas, 3, "DICE ROLL TO BIP-39",
                                      WHITE, BLACK );
    diceroll_graphics_fill_rect( &canvas, 0, DICEROLL_TITLE_HEIGHT - 1U,
                                DICEROLL_DISPLAY_WIDTH, 1, WHITE );

    for( uint16_t column = 1; column < 4; ++column )
        diceroll_graphics_fill_rect( &canvas,
                                    column * DICEROLL_WORD_COLUMN_WIDTH,
                                    DICEROLL_WORD_GRID_TOP, 1,
                                    DICEROLL_WORD_GRID_HEIGHT, WHITE );
    for( uint16_t row = 1; row <= 6; ++row )
    {
        diceroll_graphics_fill_rect( &canvas, 0,
                                    ( uint16_t )( DICEROLL_WORD_GRID_TOP +
                                                  row * DICEROLL_WORD_ROW_HEIGHT - 1U ),
                                    DICEROLL_DISPLAY_WIDTH, 1, WHITE );
    }
    for( uint8_t word = 1; word <= 24; ++word )
        draw_word_cell( &canvas, state, word );

    draw_status( &canvas, state );

    diceroll_graphics_fill_rect( &canvas, 0, DICEROLL_BUTTON_TOP,
                                DICEROLL_RESTART_WIDTH,
                                DICEROLL_BUTTON_HEIGHT,
                                DARK_RED );
    diceroll_graphics_fill_rect( &canvas, DICEROLL_RESTART_WIDTH,
                                DICEROLL_BUTTON_TOP, DICEROLL_BACK_WIDTH,
                                DICEROLL_BUTTON_HEIGHT,
                                ORANGE );
    diceroll_graphics_fill_rect( &canvas,
                                DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH,
                                DICEROLL_BUTTON_TOP, DICEROLL_BIT_BUTTON_WIDTH,
                                DICEROLL_BUTTON_HEIGHT,
                                LIGHT_GREY );
    diceroll_graphics_fill_rect( &canvas,
                                DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH +
                                DICEROLL_BIT_BUTTON_WIDTH,
                                DICEROLL_BUTTON_TOP, DICEROLL_BIT_BUTTON_WIDTH,
                                DICEROLL_BUTTON_HEIGHT,
                                DARK_GREY );
    diceroll_graphics_fill_rect( &canvas, DICEROLL_RESTART_WIDTH,
                                DICEROLL_BUTTON_TOP, 1,
                                DICEROLL_BUTTON_HEIGHT, BLACK );
    diceroll_graphics_fill_rect( &canvas,
                                DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH,
                                DICEROLL_BUTTON_TOP, 1,
                                DICEROLL_BUTTON_HEIGHT, BLACK );
    diceroll_graphics_fill_rect( &canvas,
                                DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH +
                                DICEROLL_BIT_BUTTON_WIDTH,
                                DICEROLL_BUTTON_TOP, 1,
                                DICEROLL_BUTTON_HEIGHT, BLACK );

    diceroll_graphics_text12( &canvas, 51, 350, "HOLD", WHITE,
                           DARK_RED );
    diceroll_graphics_text_default( &canvas, 26, 390, "RESTART", WHITE,
                           DARK_RED );
    diceroll_graphics_text12( &canvas, 181, 350, "HOLD", BLACK,
                           ORANGE );
    diceroll_graphics_text_default( &canvas, 168, 390, "BACK", BLACK,
                           ORANGE );
    diceroll_graphics_text24( &canvas, 352, 354, "HEADS", BLACK,
                             LIGHT_GREY );
    diceroll_graphics_text24( &canvas, 387, 406, "0", BLACK,
                             LIGHT_GREY );
    diceroll_graphics_text24( &canvas, 622, 354, "TAILS", WHITE,
                             DARK_GREY );
    diceroll_graphics_text24( &canvas, 657, 406, "1", WHITE,
                             DARK_GREY );

}

static void present( const MnemonicState *state )
{
    /* Diceroll uses a single continuously scanned framebuffer. Updating it in
       place avoids a full-frame double-buffer switch on every coin flip. */
    draw_diceroll_screen( framebuffer, state );
}

void diceroll_app_run( void )
{
    MnemonicState state;
    bool touch_down = false;
    bool action_done = false;
    DicerollButton held_button = DICEROLL_BUTTON_NONE;
    uint8_t release_samples = 0;
    uint16_t hold_progress = 0;
    uint16_t touch_x = 0U;
    uint16_t touch_y = 0U;
    uint64_t press_started = 0U;

    mnemonic_state_init( &state );
    framebuffer = waveshare_platform_display_init();
    present( &state );
    waveshare_platform_touch_init();

    while( true )
    {
        bool pressed = waveshare_platform_touch_read( &touch_x, &touch_y );

        if( pressed )
            release_samples = 0;

        if( pressed && !touch_down )
        {
            touch_down = true;
            action_done = false;
            press_started = waveshare_platform_time_us();
            held_button = DICEROLL_BUTTON_NONE;
            hold_progress = 0;

            if( touch_y >= DICEROLL_BUTTON_TOP )
            {
                held_button = diceroll_layout_button_at( touch_x, touch_y );
                if( ( held_button == DICEROLL_BUTTON_RESTART &&
                      mnemonic_state_get_bit_count( &state ) == 0U ) ||
                    ( held_button == DICEROLL_BUTTON_BACK &&
                      ( mnemonic_state_get_bit_count( &state ) == 0U ||
                         mnemonic_state_entropy_complete( &state ) ) ) ||
                    ( held_button >= DICEROLL_BUTTON_ZERO &&
                         mnemonic_state_entropy_complete( &state ) ) )
                {
                    held_button = DICEROLL_BUTTON_NONE;
                    action_done = true;
                }
            }
            else if( touch_y >= DICEROLL_WORD_GRID_TOP &&
                     touch_y < DICEROLL_STATUS_TOP )
            {
                uint8_t column = ( uint8_t )( touch_x /
                                               DICEROLL_WORD_COLUMN_WIDTH );
                uint8_t row = ( uint8_t )( ( touch_y -
                                             DICEROLL_WORD_GRID_TOP ) /
                                           DICEROLL_WORD_ROW_HEIGHT );
                uint8_t word = ( uint8_t )( column * 6U + row + 1U );
                uint8_t completed = mnemonic_state_entropy_complete( &state )
                                    ? MNEMONIC_WORD_COUNT
                                    : mnemonic_state_get_completed_word_count( &state );
                if( word <= completed )
                {
                    DicerollCanvas canvas = { framebuffer, LCD_WIDTH,
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

            if( held_button == DICEROLL_BUTTON_ZERO )
            {
                uint8_t previous_word = mnemonic_state_get_current_word_number( &state );
                uint8_t previous_entered = mnemonic_state_get_current_word_bit_count( &state );
                clear_word_selection( &state );
                mnemonic_state_add_flip( &state, 0 );
                action_done = true;
                update_state_regions( &state, previous_word, previous_entered );
            }
            else if( held_button == DICEROLL_BUTTON_ONE )
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
            int64_t held_us = ( int64_t )( waveshare_platform_time_us() -
                                           press_started );
            if( ( held_button == DICEROLL_BUTTON_RESTART ||
                  held_button == DICEROLL_BUTTON_BACK ) &&
                ( touch_y < DICEROLL_BUTTON_TOP ||
                  ( held_button == DICEROLL_BUTTON_RESTART &&
                    touch_x >= DICEROLL_RESTART_WIDTH ) ||
                  ( held_button == DICEROLL_BUTTON_BACK &&
                    ( touch_x < DICEROLL_RESTART_WIDTH ||
                      touch_x >= DICEROLL_RESTART_WIDTH +
                                 DICEROLL_BACK_WIDTH ) ) ) )
            {
                diceroll_ui_clear_hold_progress( framebuffer, held_button );
                action_done = true;
                continue;

            }
            hold_progress = diceroll_ui_show_hold_progress(
                                framebuffer, held_button, held_us, hold_progress );
            if( held_button == DICEROLL_BUTTON_RESTART &&
                held_us >= 1000000 )
            {
                mnemonic_state_init( &state );
                selected_word = 0U;
                action_done = true;
                present( &state );
            }
            else if( held_button == DICEROLL_BUTTON_BACK &&
                     held_us >= 500000 )
            {
                uint8_t previous_word = mnemonic_state_get_current_word_number( &state );
                uint8_t previous_entered = mnemonic_state_get_current_word_bit_count( &state );
                mnemonic_state_backspace( &state );
                action_done = true;
                diceroll_ui_clear_hold_progress( framebuffer, held_button );
                update_state_regions( &state, previous_word, previous_entered );
            }
        }
        else if( !pressed && touch_down )
        {
            if( release_samples < TOUCH_RELEASE_SAMPLES )
                ++release_samples;

            if( release_samples >= TOUCH_RELEASE_SAMPLES )
            {
                diceroll_ui_clear_hold_progress( framebuffer, held_button );
                touch_down = false;
                action_done = false;
                held_button = DICEROLL_BUTTON_NONE;
                hold_progress = 0;
            }
        }
        waveshare_platform_sleep_ms( 5U );
    }
}
