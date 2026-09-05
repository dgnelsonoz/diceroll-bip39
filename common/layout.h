#ifndef DICEROLL_LAYOUT_H
#define DICEROLL_LAYOUT_H

#include <stdint.h>

enum
{
    DICEROLL_DISPLAY_WIDTH = 800U,
    DICEROLL_DISPLAY_HEIGHT = 480U,
    DICEROLL_TITLE_HEIGHT = 32U,
    DICEROLL_WORD_GRID_TOP = DICEROLL_TITLE_HEIGHT,
    DICEROLL_WORD_GRID_HEIGHT = 216U,
    DICEROLL_STATUS_TOP = DICEROLL_WORD_GRID_TOP + DICEROLL_WORD_GRID_HEIGHT,
    DICEROLL_STATUS_HEIGHT = 80U,
    DICEROLL_BUTTON_TOP = DICEROLL_STATUS_TOP + DICEROLL_STATUS_HEIGHT,
    DICEROLL_BUTTON_HEIGHT = DICEROLL_DISPLAY_HEIGHT - DICEROLL_BUTTON_TOP,
    DICEROLL_WORD_COLUMN_WIDTH = 200U,
    DICEROLL_WORD_ROW_HEIGHT = 36U,
    DICEROLL_RESTART_WIDTH = 130U,
    DICEROLL_BACK_WIDTH = 130U,
    DICEROLL_BIT_BUTTON_WIDTH = 270U
};

typedef enum
{
    DICEROLL_BUTTON_NONE = 0,
    DICEROLL_BUTTON_RESTART,
    DICEROLL_BUTTON_BACK,
    DICEROLL_BUTTON_ZERO,
    DICEROLL_BUTTON_ONE
} DicerollButton;

static inline void diceroll_layout_word_cell( uint8_t word_number,
                                              uint16_t *x, uint16_t *y )
{
    uint8_t zero_based = ( uint8_t )( word_number - 1U );

    *x = ( uint16_t )( zero_based / 6U ) * DICEROLL_WORD_COLUMN_WIDTH;
    *y = ( uint16_t )( DICEROLL_WORD_GRID_TOP +
                       ( uint16_t )( zero_based % 6U ) * DICEROLL_WORD_ROW_HEIGHT );
}

static inline DicerollButton diceroll_layout_button_at( uint16_t x,
                                                         uint16_t y )
{
    if( y < DICEROLL_BUTTON_TOP ||
        y >= DICEROLL_BUTTON_TOP + DICEROLL_BUTTON_HEIGHT ||
        x >= DICEROLL_DISPLAY_WIDTH )
        return DICEROLL_BUTTON_NONE;

    if( x < DICEROLL_RESTART_WIDTH )
        return DICEROLL_BUTTON_RESTART;
    if( x < DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH )
        return DICEROLL_BUTTON_BACK;
    if( x < DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH +
            DICEROLL_BIT_BUTTON_WIDTH )
        return DICEROLL_BUTTON_ZERO;
    return DICEROLL_BUTTON_ONE;
}

#endif
