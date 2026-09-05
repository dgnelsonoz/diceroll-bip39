#ifndef WAVESHARE_PLATFORM_H
#define WAVESHARE_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

uint16_t *waveshare_platform_display_init( void );
void waveshare_platform_touch_init( void );
bool waveshare_platform_touch_read( uint16_t *x, uint16_t *y );
uint64_t waveshare_platform_time_us( void );
void waveshare_platform_sleep_ms( uint32_t milliseconds );

#endif
