#ifndef COINFLIP_PLATFORM_H
#define COINFLIP_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

enum
{
    COINFLIP_DISPLAY_WIDTH = 800U,
    COINFLIP_DISPLAY_HEIGHT = 480U
};

bool coinflip_display_init( void );
uint16_t *coinflip_display_framebuffer( void );
void coinflip_display_fill( uint16_t color );
void coinflip_display_fill_rect( uint16_t x, uint16_t y,
                                uint16_t width, uint16_t height,
                                uint16_t color );

bool coinflip_touch_init( void );
bool coinflip_touch_read( uint16_t *x, uint16_t *y );

#endif
