#include "fonts.h"
#include "bsp_st7262.h"
#include "pio_rgb.h"
#include "rp_pico_alloc.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"

enum { WIDTH = 800, HEIGHT = 480, TRANSFER = WIDTH * 120 };
extern const sFONT Font16Original, Font16TerminusVga, Font16Unifont, Font16Terminus;
static uint16_t transfer1[ TRANSFER ], transfer2[ TRANSFER ];

static void pixel( uint16_t *fb, uint16_t x, uint16_t y, uint16_t color )
{
    if( x < WIDTH && y < HEIGHT ) fb[ y * WIDTH + x ] = color;
}

static void text( uint16_t *fb, const sFONT *font, uint16_t x, uint16_t y,
                 const char *value )
{
    while( *value != '\0' )
    {
        unsigned char character = ( unsigned char )*value++;
        if( character < ' ' || character > '~' ) character = '?';
        const uint8_t *glyph = font->table + ( character - ' ' ) * 32U;
        for( uint16_t row = 0; row < 16; ++row )
            for( uint16_t column = 0; column < 11; ++column )
                if( glyph[ row * 2U + column / 8U ] & ( 0x80U >> ( column % 8U ) ) )
                    pixel( fb, x + column, y + row, 0xffffU );
        x += 12U;
    }
}

int main( void )
{
    static pio_rgb_info_t rgb;
    static bsp_display_interface_t *display;
    static uint16_t *framebuffer;
    static const sFONT *fonts[] = { &Font16Original, &Font16Terminus,
                                    &Font16TerminusVga, &Font16Unifont };
    static const char *names[] = { "ORIGINAL", "TERMINUS", "TERM VGA", "UNIFONT" };
    static const char *samples[] = { "among", "there", "winter", "usual", "little", "hand" };
    static bsp_display_info_t info;

    set_sys_clock_khz( 260000U, true );
    rgb.width = WIDTH; rgb.height = HEIGHT; rgb.transfer_size = TRANSFER;
    rgb.pclk_freq = BSP_LCD_PCLK_FREQ; rgb.mode.double_buffer = false;
    rgb.mode.enabled_transfer = true; rgb.mode.enabled_psram = true;
    rgb.framebuffer1 = rp_mem_malloc( WIDTH * HEIGHT * sizeof( uint16_t ) );
    rgb.framebuffer2 = NULL; rgb.transfer_buffer1 = transfer1; rgb.transfer_buffer2 = transfer2;
    info.width = WIDTH; info.height = HEIGHT; info.brightness = 100; info.user_data = &rgb;
    bsp_display_new_st7262( &display, &info ); display->init(); framebuffer = rgb.framebuffer1;
    for( uint32_t i = 0; i < WIDTH * HEIGHT; ++i ) framebuffer[ i ] = 0;
    for( uint8_t column = 0; column < 4; ++column )
    {
        text( framebuffer, fonts[ column ], column * 200U + 8U, 8U, names[ column ] );
        for( uint8_t row = 0; row < 6; ++row )
            text( framebuffer, fonts[ column ], column * 200U + 8U,
                  45U + row * 65U, samples[ row ] );
    }
    display->flush_dma( NULL, NULL );
    while( true ) sleep_ms( 1000 );
}
