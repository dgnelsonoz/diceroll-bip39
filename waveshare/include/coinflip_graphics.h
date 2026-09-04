#ifndef COINFLIP_GRAPHICS_H
#define COINFLIP_GRAPHICS_H

#include <stdint.h>

typedef struct
{
    uint16_t *pixels;
    uint16_t width;
    uint16_t height;
} CoinflipCanvas;

void coinflip_graphics_clear( CoinflipCanvas *canvas, uint16_t color );
void coinflip_graphics_fill_rect( CoinflipCanvas *canvas,
                                 uint16_t x, uint16_t y,
                                 uint16_t width, uint16_t height,
                                 uint16_t color );
void coinflip_graphics_draw_rect( CoinflipCanvas *canvas,
                                 uint16_t x, uint16_t y,
                                 uint16_t width, uint16_t height,
                                 uint16_t color );
void coinflip_graphics_text( CoinflipCanvas *canvas,
                            uint16_t x, uint16_t y, const char *text,
                            uint8_t scale, uint16_t foreground,
                            uint16_t background );
void coinflip_graphics_text20( CoinflipCanvas *canvas,
                              uint16_t x, uint16_t y, const char *text,
                              uint16_t foreground, uint16_t background );
void coinflip_graphics_text24( CoinflipCanvas *canvas,
                              uint16_t x, uint16_t y, const char *text,
                              uint16_t foreground, uint16_t background );
void coinflip_graphics_text24_centered( CoinflipCanvas *canvas,
                                       uint16_t y, const char *text,
                                       uint16_t foreground,
                                       uint16_t background );
void coinflip_graphics_text_centered( CoinflipCanvas *canvas,
                                     uint16_t y, const char *text,
                                     uint8_t scale, uint16_t foreground,
                                     uint16_t background );

#endif
