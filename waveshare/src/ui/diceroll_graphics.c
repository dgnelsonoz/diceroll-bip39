#include "diceroll_graphics.h"

#include "fonts.h"
#include "utf8.h"

#include <stddef.h>
#include <string.h>

extern const sFONT Font16Default;

static void set_pixel( DicerollCanvas *canvas, uint16_t x, uint16_t y, uint16_t color )
{
    if( canvas == NULL || canvas->pixels == NULL || x >= canvas->width || y >= canvas->height )
        return;

    canvas->pixels[ ( uint32_t )y * canvas->width + x ] = color;
}

void diceroll_graphics_clear( DicerollCanvas *canvas, uint16_t color )
{
    if( canvas == NULL || canvas->pixels == NULL )
        return;

    for( uint32_t pixel = 0; pixel < ( uint32_t )canvas->width * canvas->height; ++pixel )
        canvas->pixels[ pixel ] = color;
}

void diceroll_graphics_fill_rect( DicerollCanvas *canvas, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color )
{
    if( canvas == NULL || x >= canvas->width || y >= canvas->height )
        return;

    uint32_t x_end = ( uint32_t )x + width;
    uint32_t y_end = ( uint32_t )y + height;

    if( x_end > canvas->width )
        x_end = canvas->width;

    if( y_end > canvas->height )
        y_end = canvas->height;

    for( uint16_t row = y; row < y_end; ++row )
    {
        for( uint16_t column = x; column < x_end; ++column )
            set_pixel( canvas, column, row, color );
    }
}

void diceroll_graphics_draw_rect( DicerollCanvas *canvas, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color )
{
    if( width == 0U || height == 0U )
        return;

    diceroll_graphics_fill_rect( canvas, x, y, width, 1, color );
    diceroll_graphics_fill_rect( canvas, x, ( uint16_t )( y + height - 1U ), width, 1, color );
    diceroll_graphics_fill_rect( canvas, x, y, 1, height, color );
    diceroll_graphics_fill_rect( canvas, ( uint16_t )( x + width - 1U ), y, 1, height, color );
}

static void draw_character( DicerollCanvas *canvas, const sFONT *font, uint16_t x, uint16_t y, char character, uint8_t scale, uint16_t foreground, uint16_t background )
{
    const uint16_t bytes_per_row = ( uint16_t )( ( font->Width + 7U ) / 8U );
    const size_t glyph_size = ( size_t )font->Height * bytes_per_row;
    const uint8_t *glyph;

    if( character < ' ' || character > '~' )
        character = '?';

    glyph = font->table + ( size_t )( character - ' ' ) * glyph_size;

    for( uint16_t row = 0; row < font->Height; ++row )
    {
        for( uint16_t column = 0; column < font->Width; ++column )
        {
            const uint8_t bits = glyph[ row * bytes_per_row + column / 8U ];
            const uint16_t color = ( bits & ( 0x80U >> ( column % 8U ) ) )
                                   ? foreground : background;

            diceroll_graphics_fill_rect( canvas, ( uint16_t )( x + column * scale ), ( uint16_t )( y + row * scale ), scale, scale, color );
        }
    }
}

static void draw_combining_mark( DicerollCanvas *canvas, const sFONT *font,
                                uint16_t x, uint16_t y, uint32_t codepoint,
                                uint16_t color )
{
    uint16_t center = ( uint16_t )( x + font->Width / 2U );

    if( codepoint == 0x0301U )
    {
        diceroll_graphics_fill_rect( canvas, ( uint16_t )( center + 1U ),
                                    ( uint16_t )( y + 1U ),
                                    2U, 2U, color );
        diceroll_graphics_fill_rect( canvas, center, ( uint16_t )( y + 2U ),
                                    2U, 2U, color );
    }
    else if( codepoint == 0x0300U )
    {
        diceroll_graphics_fill_rect( canvas, ( uint16_t )( center - 2U ), y,
                                    2U, 2U, color );
        diceroll_graphics_fill_rect( canvas, center, ( uint16_t )( y + 2U ),
                                    2U, 2U, color );
    }
    else if( codepoint == 0x0303U )
    {
        diceroll_graphics_fill_rect( canvas, ( uint16_t )( center - 2U ),
                                    ( uint16_t )( y + 1U ), 2U, 1U, color );
        diceroll_graphics_fill_rect( canvas, ( uint16_t )( center ),
                                    ( uint16_t )( y + 2U ), 2U, 1U, color );
        diceroll_graphics_fill_rect( canvas, ( uint16_t )( center + 2U ),
                                    ( uint16_t )( y + 1U ), 2U, 1U, color );
    }
}

static int is_combining_mark( uint32_t codepoint )
{
    return codepoint == 0x0300U || codepoint == 0x0301U ||
           codepoint == 0x0303U;
}

static size_t utf8_glyph_count( const char *text )
{
    const char *cursor = text;
    uint32_t codepoint;
    size_t count = 0U;
    int result;

    while( ( result = diceroll_utf8_next( &cursor, &codepoint ) ) > 0 )
    {
        if( !is_combining_mark( codepoint ) )
            ++count;
    }
    if( result < 0 )
        ++count;
    return count;
}

static uint16_t draw_utf8_text( DicerollCanvas *canvas, const sFONT *font,
                               uint16_t x, uint16_t y, const char *text,
                               uint8_t scale, uint16_t foreground,
                               uint16_t background )
{
    const char *cursor = text;
    uint32_t codepoint;
    uint16_t last_x = x;
    uint8_t last_width = 0U;

    while( diceroll_utf8_next( &cursor, &codepoint ) > 0 )
    {
        if( is_combining_mark( codepoint ) )
        {
            if( last_width != 0U )
                draw_combining_mark( canvas, font, last_x, y, codepoint,
                                    foreground );
            continue;
        }

        last_x = x;
        last_width = ( uint8_t )( font->Width * scale );
        draw_character( canvas, font, x, y,
                       codepoint >= ' ' && codepoint <= '~'
                       ? ( char )codepoint : '?', scale, foreground,
                       background );
        x = ( uint16_t )( x + last_width );
    }
    return x;
}

void diceroll_graphics_text( DicerollCanvas *canvas, uint16_t x, uint16_t y, const char *text, uint8_t scale, uint16_t foreground, uint16_t background )
{
    if( text == NULL || scale == 0U )
        return;

    draw_utf8_text( canvas, &Font16, x, y, text, scale, foreground, background );
}

void diceroll_graphics_text_default( DicerollCanvas *canvas, uint16_t x,
                                    uint16_t y, const char *text,
                                    uint16_t foreground, uint16_t background )
{
    if( text == NULL )
        return;
    draw_utf8_text( canvas, &Font16Default, x, y, text, 1, foreground, background );
}

void diceroll_graphics_text12( DicerollCanvas *canvas, uint16_t x,
                              uint16_t y, const char *text,
                              uint16_t foreground, uint16_t background )
{
    if( text == NULL )
        return;
    draw_utf8_text( canvas, &Font12, x, y, text, 1, foreground, background );
}


void diceroll_graphics_text20( DicerollCanvas *canvas, uint16_t x, uint16_t y, const char *text, uint16_t foreground, uint16_t background )
{
    if( text == NULL )
        return;

    draw_utf8_text( canvas, &Font20, x, y, text, 1, foreground, background );
}

void diceroll_graphics_text24( DicerollCanvas *canvas, uint16_t x, uint16_t y, const char *text, uint16_t foreground, uint16_t background )
{
    if( text == NULL )
        return;

    draw_utf8_text( canvas, &Font24, x, y, text, 1, foreground, background );
}

void diceroll_graphics_text24_centered( DicerollCanvas *canvas, uint16_t y, const char *text, uint16_t foreground, uint16_t background )
{
    size_t width;
    uint16_t x;

    if( canvas == NULL || text == NULL )
        return;

    width = utf8_glyph_count( text ) * Font24.Width;
    x = width < canvas->width ? ( uint16_t )( ( canvas->width - width ) / 2U ) : 0U;

    diceroll_graphics_text24( canvas, x, y, text, foreground, background );
}

void diceroll_graphics_text_centered( DicerollCanvas *canvas, uint16_t y, const char *text, uint8_t scale, uint16_t foreground, uint16_t background )
{
    size_t width;
    uint16_t x;

    if( canvas == NULL || text == NULL || scale == 0U )
        return;

    width = utf8_glyph_count( text ) * Font16.Width * scale;
    x = width < canvas->width ? ( uint16_t )( ( canvas->width - width ) / 2U ) : 0U;

    diceroll_graphics_text( canvas, x, y, text, scale, foreground, background );
}
