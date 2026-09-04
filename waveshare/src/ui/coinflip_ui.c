#include "coinflip_ui.h"

#include "coinflip_graphics.h"

enum
{
    LCD_WIDTH = 800U,
    LCD_HEIGHT = 480U
};

static const uint16_t GREEN = 0x07e0U;
static const uint16_t DARK_RED = 0x7800U;
static const uint16_t ORANGE = 0xfd20U;

void coinflip_ui_clear_hold_progress( uint16_t *pixels, uint8_t button )
{
    CoinflipCanvas canvas = { pixels, LCD_WIDTH, LCD_HEIGHT };
    if( button == 1U )
        coinflip_graphics_fill_rect( &canvas, 1, 328, 129, 10, DARK_RED );
    else if( button == 2U )
        coinflip_graphics_fill_rect( &canvas, 131, 328, 129, 10, ORANGE );
}

uint16_t coinflip_ui_show_hold_progress( uint16_t *pixels, uint8_t button,
                                        int64_t elapsed_us,
                                        uint16_t previous_progress )
{
    CoinflipCanvas canvas = { pixels, LCD_WIDTH, LCD_HEIGHT };
    uint32_t required_us = button == 1U ? 1000000U : 500000U;
    uint16_t x = button == 1U ? 1U : 131U;
    uint16_t progress;

    if( ( button != 1U && button != 2U ) || elapsed_us <= 0 )
        return previous_progress;

    if( ( uint64_t )elapsed_us > required_us )
        elapsed_us = required_us;
    progress = ( uint16_t )( ( 129U * ( uint64_t )elapsed_us ) / required_us );
    if( progress > previous_progress )
    {
        coinflip_graphics_fill_rect( &canvas, ( uint16_t )( x + previous_progress ), 328, ( uint16_t )( progress - previous_progress ), 10, GREEN );
    }
    return progress;

}
