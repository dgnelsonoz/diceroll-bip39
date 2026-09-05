#include "diceroll_ui.h"

#include "diceroll_graphics.h"

static const uint16_t LIGHT_RED = 0xd800U;
static const uint16_t DARK_RED = 0x7800U;
static const uint16_t ORANGE = 0xfd20U;
static const uint16_t LIGHT_ORANGE = 0xffa0U;

void diceroll_ui_clear_hold_progress( uint16_t *pixels,
                                      DicerollButton button )
{
    DicerollCanvas canvas =
    {
        pixels, DICEROLL_DISPLAY_WIDTH, DICEROLL_DISPLAY_HEIGHT
    };

    if( button == DICEROLL_BUTTON_RESTART )
        diceroll_graphics_fill_rect( &canvas, 1, DICEROLL_BUTTON_TOP,
                                    DICEROLL_RESTART_WIDTH - 1U, 10,
                                    DARK_RED );
    else if( button == DICEROLL_BUTTON_BACK )
        diceroll_graphics_fill_rect( &canvas,
                                    DICEROLL_RESTART_WIDTH + 1U,
                                    DICEROLL_BUTTON_TOP,
                                    DICEROLL_BACK_WIDTH - 1U, 10, ORANGE );
}

uint16_t diceroll_ui_show_hold_progress( uint16_t *pixels,
                                        DicerollButton button,
                                        int64_t elapsed_us,
                                        uint16_t previous_progress )
{
    DicerollCanvas canvas =
    {
        pixels, DICEROLL_DISPLAY_WIDTH, DICEROLL_DISPLAY_HEIGHT
    };
    uint32_t required_us = button == DICEROLL_BUTTON_RESTART
                           ? 1000000U : 500000U;
    uint16_t x = button == DICEROLL_BUTTON_RESTART
                 ? 1U : DICEROLL_RESTART_WIDTH + 1U;
    uint16_t progress;

    if( ( button != DICEROLL_BUTTON_RESTART &&
          button != DICEROLL_BUTTON_BACK ) || elapsed_us <= 0 )
        return previous_progress;

    if( ( uint64_t )elapsed_us > required_us )
        elapsed_us = required_us;
    progress = ( uint16_t )( ( ( DICEROLL_RESTART_WIDTH - 1U ) *
                               ( uint64_t )elapsed_us ) / required_us );
    if( progress > previous_progress )
    {
        diceroll_graphics_fill_rect( &canvas,
                                    ( uint16_t )( x + previous_progress ),
                                    DICEROLL_BUTTON_TOP,
                                    ( uint16_t )( progress - previous_progress ),
                                    10, button == DICEROLL_BUTTON_RESTART
                                    ? LIGHT_RED : LIGHT_ORANGE );
    }
    return progress;

}
