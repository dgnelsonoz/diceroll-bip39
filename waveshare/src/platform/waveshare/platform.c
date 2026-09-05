#include "waveshare_platform.h"

#include "layout.h"
#include "rp2350_clock.h"

#include "bsp_gt911.h"
#include "bsp_i2c.h"
#include "bsp_st7262.h"
#include "pico/stdlib.h"
#include "pio_rgb.h"
#include "rp_pico_alloc.h"

enum
{
    TRANSFER_PIXELS = DICEROLL_DISPLAY_WIDTH * 120U,
    SYSTEM_CLOCK_MHZ = 260U
};

static bsp_display_interface_t *display;
static bsp_touch_interface_t *touch;
static uint16_t transfer_buffer1[ TRANSFER_PIXELS ];
static uint16_t transfer_buffer2[ TRANSFER_PIXELS ];
static pio_rgb_info_t rgb =
{
    .width = DICEROLL_DISPLAY_WIDTH,
    .height = DICEROLL_DISPLAY_HEIGHT,
    .transfer_size = TRANSFER_PIXELS,
    .pclk_freq = BSP_LCD_PCLK_FREQ,
    .mode = { false, true, true }
};
static bsp_display_info_t display_info =
{
    .width = DICEROLL_DISPLAY_WIDTH,
    .height = DICEROLL_DISPLAY_HEIGHT,
    .brightness = 100,
    .user_data = &rgb
};
static bsp_touch_info_t touch_info =
{
    .width = DICEROLL_DISPLAY_WIDTH,
    .height = DICEROLL_DISPLAY_HEIGHT,
    .rotation = 0
};

uint16_t *waveshare_platform_display_init( void )
{
    rp2350_set_system_clock( SYSTEM_CLOCK_MHZ );
    rgb.framebuffer1 = rp_mem_malloc( DICEROLL_DISPLAY_WIDTH *
                                      DICEROLL_DISPLAY_HEIGHT *
                                      sizeof( uint16_t ) );
    rgb.framebuffer2 = NULL;
    rgb.transfer_buffer1 = transfer_buffer1;
    rgb.transfer_buffer2 = transfer_buffer2;

    if( rgb.framebuffer1 == NULL )
        panic( "display allocation failed" );
    if( !bsp_display_new_st7262( &display, &display_info ) )
        panic( "display creation failed" );

    display->init();
    return rgb.framebuffer1;
}

void waveshare_platform_touch_init( void )
{
    bsp_i2c_init();
    if( !bsp_touch_new_gt911( &touch, &touch_info ) )
        panic( "touch creation failed" );
    touch->init();
}

bool waveshare_platform_touch_read( uint16_t *x, uint16_t *y )
{
    bsp_touch_data_t touch_data;

    touch->read();
    if( !touch->get_data( &touch_data ) || touch_data.points == 0U )
        return false;

    *x = touch_data.coords[ 0 ].x;
    *y = touch_data.coords[ 0 ].y;
    return true;
}

uint64_t waveshare_platform_time_us( void )
{
    return time_us_64();
}

void waveshare_platform_sleep_ms( uint32_t milliseconds )
{
    sleep_ms( milliseconds );
}
