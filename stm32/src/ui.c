#include "ui.h"

#include "bip39_lookup.h"
#include "fonts.h"
#include "layout.h"
#include "stm32469i_discovery_lcd.h"
#include "ui_model.h"
#include "utf8.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef char button_widths_must_fill_display[
    ( DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH +
      ( 2 * DICEROLL_BIT_BUTTON_WIDTH ) == DICEROLL_DISPLAY_WIDTH )
    ? 1 : -1 ];
typedef char vertical_regions_must_fill_display[
    ( DICEROLL_TITLE_HEIGHT + DICEROLL_WORD_GRID_HEIGHT +
      DICEROLL_STATUS_HEIGHT + DICEROLL_BUTTON_HEIGHT ==
      DICEROLL_DISPLAY_HEIGHT )
    ? 1 : -1 ];
typedef char word_rows_must_fill_grid[
    ( 6 * DICEROLL_WORD_ROW_HEIGHT == DICEROLL_WORD_GRID_HEIGHT )
    ? 1 : -1 ];

static uint8_t selected_word;

static int is_combining_mark( uint32_t codepoint )
{
    return codepoint == 0x0300U || codepoint == 0x0301U ||
           codepoint == 0x0303U;
}

static void draw_combining_mark( uint16_t x, uint16_t y, uint32_t codepoint )
{
    const sFONT *font = BSP_LCD_GetFont( );
    uint16_t center = ( uint16_t )( x + font->Width / 2U );
    uint32_t color = BSP_LCD_GetTextColor( );

    if( codepoint == 0x0301U )
    {
        BSP_LCD_DrawPixel( center - 2U, y + 1U, color );
        BSP_LCD_DrawPixel( center - 1U, y, color );
    }
    else if( codepoint == 0x0300U )
    {
        BSP_LCD_DrawPixel( center + 1U, y, color );
        BSP_LCD_DrawPixel( center + 2U, y + 1U, color );
    }
    else if( codepoint == 0x0303U )
    {
        BSP_LCD_DrawPixel( center - 2U, y + 1U, color );
        BSP_LCD_DrawPixel( center - 1U, y, color );
        BSP_LCD_DrawPixel( center, y + 1U, color );
        BSP_LCD_DrawPixel( center + 1U, y, color );
        BSP_LCD_DrawPixel( center + 2U, y + 1U, color );
    }
}

static void display_text( uint16_t x, uint16_t y, const char *text )
{
    const char *cursor = text;
    uint16_t previous_x = x;
    uint16_t advance = BSP_LCD_GetFont( )->Width;
    uint32_t codepoint;
    int result;
    int have_previous = 0;

    while( ( result = diceroll_utf8_next( &cursor, &codepoint ) ) > 0 )
    {
        if( is_combining_mark( codepoint ) && have_previous )
        {
            draw_combining_mark( previous_x, y, codepoint );
            continue;
        }

        previous_x = x;
        have_previous = 1;
        BSP_LCD_DisplayChar( x, y,
                             ( uint8_t )( codepoint >= ' ' && codepoint <= '~'
                                          ? codepoint : '?' ) );
        x = ( uint16_t )( x + advance );
    }

    ( void )result;
}

static uint16_t display_text_width( const char *text )
{
    const char *cursor = text;
    uint16_t glyphs = 0;
    uint32_t codepoint;
    int result;

    while( ( result = diceroll_utf8_next( &cursor, &codepoint ) ) > 0 )
    {
        if( !is_combining_mark( codepoint ) )
            ++glyphs;
    }
    return ( uint16_t )( glyphs * BSP_LCD_GetFont( )->Width );
}

static uint8_t utf8_character_count( const char *text )
{
    const char *cursor = text;
    uint32_t codepoint;
    uint8_t count = 0U;

    while( diceroll_utf8_next( &cursor, &codepoint ) > 0 )
        if( !is_combining_mark( codepoint ) )
            ++count;
    return count;
}

static void display_text_centered( uint16_t y, const char *text )
{
    uint16_t width = display_text_width( text );
    uint16_t x = width < DICEROLL_DISPLAY_WIDTH
                 ? ( uint16_t )( ( DICEROLL_DISPLAY_WIDTH - width ) / 2U ) : 0U;

    display_text( x, y, text );
}

static void draw_title( void )
{
    BSP_LCD_SetTextColor( LCD_COLOR_BLACK );
    BSP_LCD_FillRect( 0, 0, DICEROLL_DISPLAY_WIDTH, DICEROLL_TITLE_HEIGHT );
    BSP_LCD_SetFont( &Font24 );
    BSP_LCD_SetTextColor( LCD_COLOR_WHITE );
    BSP_LCD_SetBackColor( LCD_COLOR_BLACK );
    display_text_centered( 3, "DICE ROLL OR COIN FLIP TO BIP-39" );
    BSP_LCD_DrawHLine( 0, DICEROLL_TITLE_HEIGHT - 1U, DICEROLL_DISPLAY_WIDTH );
}

static uint8_t word_grid_columns( const MnemonicState *state )
{
    return mnemonic_state_get_word_count( state ) == 12U ? 3U : 4U;
}

static uint8_t word_grid_rows( const MnemonicState *state )
{
    return mnemonic_state_get_word_count( state ) == 12U ? 4U : 6U;
}

static uint16_t word_cell_width( const MnemonicState *state )
{
    return DICEROLL_DISPLAY_WIDTH / word_grid_columns( state );
}

static uint16_t word_cell_height( const MnemonicState *state )
{
    return DICEROLL_WORD_GRID_HEIGHT / word_grid_rows( state );
}

static const char *word_number_separator( const MnemonicState *state, uint8_t word_number )
{
    if( mnemonic_state_get_word_count( state ) == 12U )
        return word_number == 9U ? "  " : " ";
    return word_number > 6U && word_number < 10U ? "  " : " ";
}

static void get_word_cell( const MnemonicState *state, uint8_t word_number, uint16_t *x, uint16_t *y )
{
    uint8_t rows = word_grid_rows( state );
    uint8_t zero_based = word_number - 1U;

    *x = ( uint16_t )( zero_based / rows ) * word_cell_width( state );
    *y = ( uint16_t )( DICEROLL_WORD_GRID_TOP + ( zero_based % rows ) * word_cell_height( state ) );
}

static void draw_grid_lines( const MnemonicState *state )
{
    uint8_t column;
    uint8_t row;
    uint8_t columns = word_grid_columns( state );
    uint8_t rows = word_grid_rows( state );
    uint16_t cell_width = word_cell_width( state );
    uint16_t cell_height = word_cell_height( state );

    BSP_LCD_SetTextColor( LCD_COLOR_WHITE );
    for( column = 1; column < columns; ++column )
        BSP_LCD_DrawVLine( ( uint16_t )column * cell_width,
                           DICEROLL_WORD_GRID_TOP, DICEROLL_WORD_GRID_HEIGHT );
    for( row = 1; row <= rows; ++row )
        BSP_LCD_DrawHLine( 0, DICEROLL_WORD_GRID_TOP +
                           ( uint16_t )row * cell_height - 1U,
                           DICEROLL_DISPLAY_WIDTH );
}

static void draw_word_cells( const MnemonicState *state )
{
    for( uint8_t word_number = 1U; word_number <= mnemonic_state_get_word_count( state ); ++word_number )
    {
        char label[ 24 ];
        char list_number[ 6 ];
        uint16_t x;
        uint16_t y;
        uint16_t index;
        uint16_t cell_width = word_cell_width( state );
        uint16_t cell_height = word_cell_height( state );
        int large_text = mnemonic_state_get_word_count( state ) == 12U;
        uint8_t current_word = mnemonic_state_get_current_word_number( state );
        int has_word = word_number < mnemonic_state_get_word_count( state )
                       ? mnemonic_state_get_word_index( state, word_number, &index ) == 0
                       : mnemonic_state_get_final_word_index( state, &index ) == 0;

        get_word_cell( state, word_number, &x, &y );
        if( x / cell_width == word_grid_columns( state ) - 1U )
            cell_width = DICEROLL_DISPLAY_WIDTH - x;
        BSP_LCD_SetTextColor( LCD_COLOR_BLACK );
        BSP_LCD_FillRect( x + 1U, y, cell_width - 1U, cell_height - 1U );

        if( has_word )
        {
            const char *word_text = bip39_get_word_by_index( index );
            uint8_t characters = utf8_character_count( word_text );
            const char *separator = word_number_separator( state, word_number );
            int offset = snprintf( label, sizeof( label ), "%u%s%s", word_number,
                                   separator, word_text );

            while( characters++ < 8U && offset < ( int )sizeof( label ) - 1 )
                label[ offset++ ] = ' ';
            label[ offset ] = '\0';
            snprintf( list_number, sizeof( list_number ), "%u", index + 1U );
        }
        else if( word_number == current_word && !mnemonic_state_entropy_complete( state ) )
            snprintf( label, sizeof( label ), "%u%s[%s]", word_number,
                      word_number_separator( state, word_number ),
                      mnemonic_state_get_current_word_bit_count( state ) == 0U
                      ? "ready" : "in progress" );
        else if( word_number == mnemonic_state_get_word_count( state ) )
            snprintf( label, sizeof( label ), "%u%s[checksum]", word_number,
                      word_number_separator( state, word_number ) );
        else
            snprintf( label, sizeof( label ), "%u", word_number );

        BSP_LCD_SetFont( large_text ? &Font20 : &Font16 );
        BSP_LCD_SetBackColor( LCD_COLOR_BLACK );
        BSP_LCD_SetTextColor( word_number == current_word &&
                              !mnemonic_state_entropy_complete( state )
                              ? LCD_COLOR_CYAN : LCD_COLOR_WHITE );
        display_text( x + ( large_text ? 8U : 10U ), y + ( large_text ? 16U : 10U ), label );
        if( has_word )
            display_text( ( uint16_t )( x + cell_width - ( large_text ? 8U : 10U ) - strlen( list_number ) * ( large_text ? Font20.Width : Font16.Width ) ),
                          y + ( large_text ? 16U : 10U ), list_number );

        if( word_number == selected_word )
        {
            BSP_LCD_SetTextColor( LCD_COLOR_YELLOW );
            BSP_LCD_DrawRect( x + 2U, y + 2U,
                              cell_width - 4U, cell_height - 4U );
        }
    }
}

static void draw_status( const MnemonicState *state )
{
    char bits[ MNEMONIC_WORD_BITS + 1U ];
    char number[ 4 ];
    uint8_t word_number = mnemonic_state_get_current_word_number( state );
    uint8_t entered = mnemonic_state_get_current_word_bit_count( state );
    uint8_t word_count = mnemonic_state_get_word_count( state );
    uint8_t required = word_number == word_count ? mnemonic_state_get_final_entropy_bit_count( state ) : MNEMONIC_WORD_BITS;
    uint8_t completed = mnemonic_state_get_completed_word_count( state );
    int word_boundary = state->bit_count > 0U &&
                        !mnemonic_state_entropy_complete( state ) &&
                        state->bit_count % MNEMONIC_WORD_BITS == 0U;

    if( word_boundary )
    {
        --word_number;
        required = word_number == word_count ? mnemonic_state_get_final_entropy_bit_count( state ) : MNEMONIC_WORD_BITS;
        entered = required;
    }

    BSP_LCD_SetTextColor( LCD_COLOR_BLACK );
    BSP_LCD_FillRect( 0, DICEROLL_STATUS_TOP, DICEROLL_DISPLAY_WIDTH,
                      DICEROLL_STATUS_HEIGHT );

    BSP_LCD_SetFont( &Font20 );
    BSP_LCD_SetBackColor( LCD_COLOR_BLACK );
    if( mnemonic_state_entropy_complete( state ) )
    {
        BSP_LCD_SetTextColor( LCD_COLOR_GREEN );
        display_text_centered( DICEROLL_STATUS_TOP + 12U, word_count == 12U ? "PHRASE COMPLETE - 12 WORDS" : "PHRASE COMPLETE - 24 WORDS" );
        completed = word_count;
    }
    else
    {
        if( word_boundary )
        {
            uint16_t completed_index;

            if( mnemonic_state_get_word_index( state, word_number,
                                               &completed_index ) == 0 )
                diceroll_format_index_bits( completed_index, bits, 0 );
        }
        else
            diceroll_format_partial_bits( state, bits, required );

        BSP_LCD_SetTextColor( LCD_COLOR_WHITE );
        display_text( 10, DICEROLL_STATUS_TOP + 10U, "WORD" );
        snprintf( number, sizeof( number ), "%u", word_number );
        display_text( ( uint16_t )( 100U - strlen( number ) * Font20.Width ),
                      DICEROLL_STATUS_TOP + 10U, number );
        display_text( 100, DICEROLL_STATUS_TOP + 10U, "/" );
        display_text( 114, DICEROLL_STATUS_TOP + 10U, word_count == 12U ? "12" : "24" );
        display_text( 200, DICEROLL_STATUS_TOP + 10U, "ROLL/FLIP" );
        snprintf( number, sizeof( number ), "%u", entered );
        display_text( ( uint16_t )( 364U - strlen( number ) * Font20.Width ),
                      DICEROLL_STATUS_TOP + 10U, number );
        display_text( 364, DICEROLL_STATUS_TOP + 10U, "/" );
        snprintf( number, sizeof( number ), "%u", required );
        display_text( 378, DICEROLL_STATUS_TOP + 10U, number );
        display_text( 470, DICEROLL_STATUS_TOP + 10U, "BITS" );
        display_text( 540, DICEROLL_STATUS_TOP + 10U, bits );
    }

    if( completed > 0U )
    {
        char verification[ 72 ];
        char verification_bits[ MNEMONIC_WORD_BITS + 2U ];
        uint8_t detail_word = selected_word != 0U ? selected_word : completed;
        uint16_t detail_index;
        int result = detail_word == word_count
                     ? mnemonic_state_get_final_word_index( state, &detail_index )
                     : mnemonic_state_get_word_index( state, detail_word, &detail_index );

        if( result == 0 )
        {
            diceroll_format_index_bits( detail_index, verification_bits,
                                        detail_word == word_count );
            snprintf( verification, sizeof( verification ),
                      "WORD %u: %s = INDEX %u = LIST %u = %s",
                      detail_word, verification_bits, detail_index,
                      detail_index + 1U, bip39_get_word_by_index( detail_index ) );
            BSP_LCD_SetFont( &Font16 );
            BSP_LCD_SetTextColor( LCD_COLOR_LIGHTGRAY );
            display_text( 20, DICEROLL_STATUS_TOP + 48U, verification );
        }
    }
}

static void fill_button( uint16_t x, uint16_t width, uint32_t color )
{
    BSP_LCD_SetTextColor( color );
    BSP_LCD_FillRect( x, DICEROLL_BUTTON_TOP, width, DICEROLL_BUTTON_HEIGHT );
}

static uint32_t button_color( DicerollButton button, int phrase_complete )
{
    ( void )phrase_complete;

    switch( button )
    {
    case DICEROLL_BUTTON_RESTART:
        return LCD_COLOR_DARKRED;
    case DICEROLL_BUTTON_BACK:
        return LCD_COLOR_ORANGE;
    case DICEROLL_BUTTON_ZERO:
        return LCD_COLOR_LIGHTGRAY;
    case DICEROLL_BUTTON_ONE:
        return LCD_COLOR_DARKGRAY;
    default:
        return LCD_COLOR_BLACK;
    }
}

static void button_bounds( DicerollButton button, uint16_t *x, uint16_t *width )
{
    switch( button )
    {
    case DICEROLL_BUTTON_RESTART:
        *x = 0;
        *width = DICEROLL_RESTART_WIDTH;
        break;
    case DICEROLL_BUTTON_BACK:
        *x = DICEROLL_RESTART_WIDTH;
        *width = DICEROLL_BACK_WIDTH;
        break;
    case DICEROLL_BUTTON_ZERO:
        *x = DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH;
        *width = DICEROLL_BIT_BUTTON_WIDTH;
        break;
    case DICEROLL_BUTTON_ONE:
        *x = DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH +
             DICEROLL_BIT_BUTTON_WIDTH;
        *width = DICEROLL_BIT_BUTTON_WIDTH;
        break;
    default:
        *x = 0;
        *width = 0;
        break;
    }
}

static void draw_buttons( int phrase_complete )
{
    uint16_t back_x = DICEROLL_RESTART_WIDTH;
    uint16_t zero_x = DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH;
    uint16_t one_x = zero_x + DICEROLL_BIT_BUTTON_WIDTH;

    fill_button( 0, DICEROLL_RESTART_WIDTH,
                 button_color( DICEROLL_BUTTON_RESTART, phrase_complete ) );
    fill_button( back_x, DICEROLL_BACK_WIDTH,
                 button_color( DICEROLL_BUTTON_BACK, phrase_complete ) );
    fill_button( zero_x, DICEROLL_BIT_BUTTON_WIDTH,
                 button_color( DICEROLL_BUTTON_ZERO, phrase_complete ) );
    fill_button( one_x, DICEROLL_BIT_BUTTON_WIDTH,
                 button_color( DICEROLL_BUTTON_ONE, phrase_complete ) );

    BSP_LCD_SetTextColor( LCD_COLOR_BLACK );
    BSP_LCD_DrawVLine( back_x, DICEROLL_BUTTON_TOP, DICEROLL_BUTTON_HEIGHT );
    BSP_LCD_DrawVLine( zero_x, DICEROLL_BUTTON_TOP, DICEROLL_BUTTON_HEIGHT );
    BSP_LCD_DrawVLine( one_x, DICEROLL_BUTTON_TOP, DICEROLL_BUTTON_HEIGHT );

    BSP_LCD_SetFont( &Font12 );
    BSP_LCD_SetTextColor( LCD_COLOR_WHITE );
    BSP_LCD_SetBackColor( LCD_COLOR_DARKRED );
    display_text( 51, 350, "HOLD" );
    BSP_LCD_SetFont( &Font16 );
    display_text( 26, 390, "RESTART" );

    BSP_LCD_SetFont( &Font12 );
    BSP_LCD_SetTextColor( LCD_COLOR_BLACK );
    BSP_LCD_SetBackColor( button_color( DICEROLL_BUTTON_BACK, phrase_complete ) );
    display_text( 181, 350, "HOLD" );
    BSP_LCD_SetFont( &Font16 );
    display_text( 173, 390, "BACK" );

    BSP_LCD_SetFont( &Font24 );
    BSP_LCD_SetTextColor( LCD_COLOR_BLACK );
    BSP_LCD_SetBackColor( button_color( DICEROLL_BUTTON_ZERO, phrase_complete ) );
    display_text( 352, 354, "HEADS" );
    display_text( 387, 406, "0" );

    BSP_LCD_SetTextColor( LCD_COLOR_WHITE );
    BSP_LCD_SetBackColor( button_color( DICEROLL_BUTTON_ONE, phrase_complete ) );
    display_text( 622, 354, "TAILS" );
    display_text( 657, 406, "1" );
}

void ui_draw( const MnemonicState *state )
{
    selected_word = 0U;
    BSP_LCD_Clear( LCD_COLOR_BLACK );
    draw_title( );
    draw_grid_lines( state );
    draw_word_cells( state );
    draw_status( state );
    draw_buttons( mnemonic_state_entropy_complete( state ) );
}

void ui_update( const MnemonicState *state )
{
    selected_word = 0U;
    draw_word_cells( state );
    draw_status( state );
    draw_buttons( mnemonic_state_entropy_complete( state ) );
}

void ui_draw_error( const char *message )
{
    BSP_LCD_Clear( LCD_COLOR_BLACK );
    BSP_LCD_SetFont( &Font24 );
    BSP_LCD_SetTextColor( LCD_COLOR_RED );
    BSP_LCD_SetBackColor( LCD_COLOR_BLACK );
    display_text_centered( 190, "HARDWARE ERROR" );
    BSP_LCD_SetFont( &Font20 );
    BSP_LCD_SetTextColor( LCD_COLOR_WHITE );
    display_text_centered( 240, message );
    display_text_centered( 275, "CHECK POWER AND RESTART" );
}

void ui_draw_word_count_selection( void )
{
    draw_title( );
    BSP_LCD_SetTextColor( LCD_COLOR_BLACK );
    BSP_LCD_FillRect( 0, DICEROLL_TITLE_HEIGHT, DICEROLL_DISPLAY_WIDTH, DICEROLL_DISPLAY_HEIGHT - DICEROLL_TITLE_HEIGHT );
    BSP_LCD_SetTextColor( LCD_COLOR_LIGHTGRAY );
    BSP_LCD_FillRect( 80, 130, 300, 220 );
    BSP_LCD_SetTextColor( LCD_COLOR_DARKGRAY );
    BSP_LCD_FillRect( 420, 130, 300, 220 );
    BSP_LCD_SetFont( &Font24 );
    BSP_LCD_SetBackColor( LCD_COLOR_LIGHTGRAY );
    BSP_LCD_SetTextColor( LCD_COLOR_BLACK );
    display_text( 148, 225, "12 WORDS" );
    BSP_LCD_SetBackColor( LCD_COLOR_DARKGRAY );
    BSP_LCD_SetTextColor( LCD_COLOR_WHITE );
    display_text( 488, 225, "24 WORDS" );
}

uint8_t ui_word_count_at( uint16_t x, uint16_t y )
{
    if( y < 130U || y >= 350U )
        return 0U;
    if( x >= 80U && x < 380U )
        return 12U;
    if( x >= 420U && x < 720U )
        return 24U;
    return 0U;
}

DicerollButton ui_hit_test( uint16_t x, uint16_t y )
{
    return diceroll_layout_button_at( x, y );
}

int ui_select_word_at( const MnemonicState *state, uint16_t x, uint16_t y )
{
    uint8_t column;
    uint8_t row;
    uint8_t word_number;
    uint8_t completed;

    if( state == NULL || x >= DICEROLL_DISPLAY_WIDTH ||
            y < DICEROLL_WORD_GRID_TOP ||
            y >= DICEROLL_WORD_GRID_TOP + DICEROLL_WORD_GRID_HEIGHT )
    {
        return 0;
    }

    uint8_t rows = word_grid_rows( state );
    column = ( uint8_t )( ( uint32_t )x * word_grid_columns( state ) / DICEROLL_DISPLAY_WIDTH );
    row = ( uint8_t )( ( y - DICEROLL_WORD_GRID_TOP ) / word_cell_height( state ) );
    word_number = ( uint8_t )( column * rows + row + 1U );
    completed = mnemonic_state_get_completed_word_count( state );
    if( mnemonic_state_entropy_complete( state ) )
        completed = mnemonic_state_get_word_count( state );
    if( word_number > completed )
        return 0;

    selected_word = word_number;
    draw_word_cells( state );
    draw_status( state );
    return 1;
}

void ui_show_hold_progress( DicerollButton button, uint32_t elapsed_ms, uint32_t required_ms )
{
    uint16_t x;
    uint16_t width;
    uint16_t progress;

    button_bounds( button, &x, &width );
    if( width == 0U || required_ms == 0U )
        return;
    if( elapsed_ms > required_ms )
        elapsed_ms = required_ms;
    progress = ( uint16_t )( ( ( uint32_t )( width - 1U ) * elapsed_ms ) / required_ms );

    BSP_LCD_SetTextColor( button_color( button, 0 ) );
    BSP_LCD_FillRect( x + 1U, DICEROLL_BUTTON_TOP, width - 1U, 10U );
    if( progress > 0U )
    {
        BSP_LCD_SetTextColor( button == DICEROLL_BUTTON_RESTART
                              ? 0xffd80000U : 0xfffff500U );
        BSP_LCD_FillRect( x + 1U, DICEROLL_BUTTON_TOP, progress, 10U );
    }
}

void ui_clear_hold_progress( DicerollButton button, int phrase_complete )
{
    uint16_t x;
    uint16_t width;

    button_bounds( button, &x, &width );
    if( width == 0U )
        return;
    BSP_LCD_SetTextColor( button_color( button, phrase_complete ) );
    BSP_LCD_FillRect( x + 1U, DICEROLL_BUTTON_TOP, width - 1U, 10U );
}
