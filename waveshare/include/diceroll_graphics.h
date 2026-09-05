#ifndef DICEROLL_GRAPHICS_H
#define DICEROLL_GRAPHICS_H

#include <stdint.h>

typedef struct
{
    uint16_t *pixels;
    uint16_t width;
    uint16_t height;
} DicerollCanvas;

void diceroll_graphics_clear( DicerollCanvas *canvas, uint16_t color );
void diceroll_graphics_fill_rect( DicerollCanvas *canvas,
                                 uint16_t x, uint16_t y,
                                 uint16_t width, uint16_t height,
                                 uint16_t color );
void diceroll_graphics_draw_rect( DicerollCanvas *canvas,
                                 uint16_t x, uint16_t y,
                                 uint16_t width, uint16_t height,
                                 uint16_t color );
void diceroll_graphics_text( DicerollCanvas *canvas,
                            uint16_t x, uint16_t y, const char *text,
                            uint8_t scale, uint16_t foreground,
                            uint16_t background );
void diceroll_graphics_text_default( DicerollCanvas *canvas,
                                    uint16_t x, uint16_t y, const char *text,
                                    uint16_t foreground, uint16_t background );
void diceroll_graphics_text12( DicerollCanvas *canvas, uint16_t x, uint16_t y,
                              const char *text, uint16_t foreground,
                              uint16_t background );
void diceroll_graphics_text20( DicerollCanvas *canvas,
                              uint16_t x, uint16_t y, const char *text,
                              uint16_t foreground, uint16_t background );
void diceroll_graphics_text24( DicerollCanvas *canvas,
                              uint16_t x, uint16_t y, const char *text,
                              uint16_t foreground, uint16_t background );
void diceroll_graphics_text24_centered( DicerollCanvas *canvas,
                                       uint16_t y, const char *text,
                                       uint16_t foreground,
                                       uint16_t background );
void diceroll_graphics_text_centered( DicerollCanvas *canvas,
                                     uint16_t y, const char *text,
                                     uint8_t scale, uint16_t foreground,
                                     uint16_t background );

#endif
