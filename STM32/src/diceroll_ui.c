#include "diceroll_ui.h"

#include "bip39_lookup.h"
#include "fonts.h"
#include "layout.h"
#include "stm32469i_discovery_lcd.h"
#include "ui_model.h"
#include "utf8.h"

#include <stdint.h>
#include <stdio.h>

typedef char button_widths_must_fill_display[
    (DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH +
     (2 * DICEROLL_BIT_BUTTON_WIDTH) == DICEROLL_DISPLAY_WIDTH)
    ? 1 : -1];
typedef char vertical_regions_must_fill_display[
    (DICEROLL_TITLE_HEIGHT + DICEROLL_WORD_GRID_HEIGHT +
     DICEROLL_STATUS_HEIGHT + DICEROLL_BUTTON_HEIGHT ==
     DICEROLL_DISPLAY_HEIGHT)
    ? 1 : -1];
typedef char word_rows_must_fill_grid[
    (6 * DICEROLL_WORD_ROW_HEIGHT == DICEROLL_WORD_GRID_HEIGHT)
    ? 1 : -1];

static uint8_t selected_word;

static int is_combining_mark(uint32_t codepoint)
{
    return codepoint == 0x0300U || codepoint == 0x0301U ||
           codepoint == 0x0303U;
}

static void draw_combining_mark(uint16_t x, uint16_t y, uint32_t codepoint)
{
    const sFONT *font = BSP_LCD_GetFont();
    uint16_t center = (uint16_t)(x + font->Width / 2U);
    uint32_t color = BSP_LCD_GetTextColor();

    if (codepoint == 0x0301U) {
        BSP_LCD_DrawPixel(center - 2U, y + 1U, color);
        BSP_LCD_DrawPixel(center - 1U, y, color);
    } else if (codepoint == 0x0300U) {
        BSP_LCD_DrawPixel(center + 1U, y, color);
        BSP_LCD_DrawPixel(center + 2U, y + 1U, color);
    } else if (codepoint == 0x0303U) {
        BSP_LCD_DrawPixel(center - 2U, y + 1U, color);
        BSP_LCD_DrawPixel(center - 1U, y, color);
        BSP_LCD_DrawPixel(center, y + 1U, color);
        BSP_LCD_DrawPixel(center + 1U, y, color);
        BSP_LCD_DrawPixel(center + 2U, y + 1U, color);
    }
}

static void display_text(uint16_t x, uint16_t y, const char *text)
{
    const char *cursor = text;
    uint16_t previous_x = x;
    uint16_t advance = BSP_LCD_GetFont()->Width;
    uint32_t codepoint;
    int result;
    int have_previous = 0;

    while ((result = diceroll_utf8_next(&cursor, &codepoint)) > 0) {
        if (is_combining_mark(codepoint) && have_previous) {
            draw_combining_mark(previous_x, y, codepoint);
            continue;
        }

        previous_x = x;
        have_previous = 1;
        BSP_LCD_DisplayChar(x, y,
                            (uint8_t)(codepoint >= ' ' && codepoint <= '~'
                                      ? codepoint : '?'));
        x = (uint16_t)(x + advance);
    }

    (void)result;
}

static uint16_t display_text_width(const char *text)
{
    const char *cursor = text;
    uint16_t glyphs = 0;
    uint32_t codepoint;
    int result;

    while ((result = diceroll_utf8_next(&cursor, &codepoint)) > 0) {
        if (!is_combining_mark(codepoint)) {
            ++glyphs;
        }
    }
    return (uint16_t)(glyphs * BSP_LCD_GetFont()->Width);
}

static void display_text_centered(uint16_t y, const char *text)
{
    uint16_t width = display_text_width(text);
    uint16_t x = width < DICEROLL_DISPLAY_WIDTH
                 ? (uint16_t)((DICEROLL_DISPLAY_WIDTH - width) / 2U) : 0U;

    display_text(x, y, text);
}

static void draw_title(void)
{
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_FillRect(0, 0, DICEROLL_DISPLAY_WIDTH, DICEROLL_TITLE_HEIGHT);
    BSP_LCD_SetFont(&Font20);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    display_text_centered(6, "DICE ROLL TO BIP-39");
    BSP_LCD_DrawHLine(0, DICEROLL_TITLE_HEIGHT - 1U, DICEROLL_DISPLAY_WIDTH);
}

static void get_word_cell(uint8_t word_number, uint16_t *x, uint16_t *y)
{
    diceroll_layout_word_cell(word_number, x, y);
}

static void draw_grid_lines(void)
{
    uint8_t column;
    uint8_t row;

    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    for (column = 1; column < 4; column++) {
        BSP_LCD_DrawVLine((uint16_t)column * DICEROLL_WORD_COLUMN_WIDTH,
                          DICEROLL_WORD_GRID_TOP, DICEROLL_WORD_GRID_HEIGHT);
    }
    for (row = 1; row <= 6; row++) {
        BSP_LCD_DrawHLine(0, DICEROLL_WORD_GRID_TOP +
                          (uint16_t)row * DICEROLL_WORD_ROW_HEIGHT - 1U,
                          DICEROLL_DISPLAY_WIDTH);
    }
}

static void draw_word_cells(const MnemonicState *state)
{
    char entry[24];
    uint8_t word_number;

    for (word_number = 1; word_number <= MNEMONIC_WORD_COUNT; word_number++) {
        uint16_t x;
        uint16_t y;
        uint16_t index;
        int has_word;

        get_word_cell(word_number, &x, &y);
        BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
        BSP_LCD_FillRect(x + 1U, y, DICEROLL_WORD_COLUMN_WIDTH - 1U,
                         DICEROLL_WORD_ROW_HEIGHT - 1U);

        if (word_number <= MNEMONIC_DIRECT_WORDS) {
            has_word = mnemonic_state_get_word_index(state, word_number,
                                                     &index) == 0;
        } else {
            has_word = mnemonic_state_get_final_word_index(state, &index) == 0;
        }

        if (has_word) {
            snprintf(entry, sizeof(entry), "%02u %-8s %04u",
                     (unsigned int)word_number,
                     bip39_get_word_by_index(index),
                     (unsigned int)index + 1U);
        } else if (word_number == mnemonic_state_get_current_word_number(state)) {
            const char *state_label =
                mnemonic_state_get_current_word_bit_count(state) == 0U
                ? "ready" : "in progress";
            snprintf(entry, sizeof(entry), "%02u [%s]",
                     (unsigned int)word_number, state_label);
        } else if (word_number == 24U) {
            snprintf(entry, sizeof(entry), "%02u [checksum]",
                     (unsigned int)word_number);
        } else {
            snprintf(entry, sizeof(entry), "%02u",
                     (unsigned int)word_number);
        }

        BSP_LCD_SetFont(&Font16);
        BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
        BSP_LCD_SetTextColor(word_number ==
                            mnemonic_state_get_current_word_number(state) &&
                            !mnemonic_state_entropy_complete(state)
                            ? LCD_COLOR_CYAN : LCD_COLOR_WHITE);
        display_text(x + 10U, y + 12U, entry);

        if (word_number == selected_word) {
            BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
            BSP_LCD_DrawRect(x + 2U, y + 2U,
                             DICEROLL_WORD_COLUMN_WIDTH - 4U,
                             DICEROLL_WORD_ROW_HEIGHT - 4U);
        }
    }
}

static void draw_status(const MnemonicState *state)
{
    char status[64];
    char bits[MNEMONIC_WORD_BITS + 1];
    char verification[72];
    char verification_bits[MNEMONIC_WORD_BITS + 2];
    uint8_t word_number = mnemonic_state_get_current_word_number(state);
    uint8_t entered = mnemonic_state_get_current_word_bit_count(state);
    uint8_t required = word_number == 24U ? 3U : MNEMONIC_WORD_BITS;
    uint8_t completed = mnemonic_state_get_completed_word_count(state);
    uint8_t detail_word;
    uint16_t detail_index;
    int word_boundary = state->bit_count > 0U &&
                        state->bit_count < MNEMONIC_ENTROPY_BITS &&
                        state->bit_count % MNEMONIC_WORD_BITS == 0U;

    if (word_boundary) {
        --word_number;
        required = word_number == 24U ? 3U : MNEMONIC_WORD_BITS;
        entered = required;
    }

    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_FillRect(0, DICEROLL_STATUS_TOP, DICEROLL_DISPLAY_WIDTH,
                     DICEROLL_STATUS_HEIGHT);

    BSP_LCD_SetFont(&Font20);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    if (mnemonic_state_entropy_complete(state)) {
        BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
        display_text_centered(DICEROLL_STATUS_TOP + 8U,
                              "PHRASE COMPLETE - 24 WORDS");
        completed = MNEMONIC_WORD_COUNT;
    } else {
        if (word_boundary) {
            uint16_t completed_index;

            if (mnemonic_state_get_word_index(state, word_number,
                                               &completed_index) == 0) {
                diceroll_format_index_bits(completed_index, bits, 0);
            }
        } else {
            diceroll_format_partial_bits(state, bits, required);
        }
        snprintf(status, sizeof(status),
                 "WORD %02u/24   FLIP %02u/%02u   BITS: %s",
                 (unsigned int)word_number, (unsigned int)entered,
                 (unsigned int)required, bits);
        BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
        display_text(20, DICEROLL_STATUS_TOP + 8U, status);
    }

    if (completed > 0U) {
        detail_word = selected_word != 0U ? selected_word : completed;
        if (detail_word == MNEMONIC_WORD_COUNT) {
            mnemonic_state_get_final_word_index(state, &detail_index);
        } else {
            mnemonic_state_get_word_index(state, detail_word, &detail_index);
        }
        diceroll_format_index_bits(detail_index, verification_bits,
                                   detail_word == MNEMONIC_WORD_COUNT);
        snprintf(verification, sizeof(verification),
                 "WORD %02u: %s = INDEX %04u = LIST %04u = %s",
                 (unsigned int)detail_word, verification_bits,
                 (unsigned int)detail_index,
                 (unsigned int)detail_index + 1U,
                 bip39_get_word_by_index(detail_index));
        BSP_LCD_SetFont(&Font16);
        BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
        display_text(20, DICEROLL_STATUS_TOP + 48U, verification);
    }
}

static void fill_button(uint16_t x, uint16_t width, uint32_t color)
{
    BSP_LCD_SetTextColor(color);
    BSP_LCD_FillRect(x, DICEROLL_BUTTON_TOP, width, DICEROLL_BUTTON_HEIGHT);
}

static uint32_t button_color(DicerollButton button, int phrase_complete)
{
    if (phrase_complete && button != DICEROLL_BUTTON_RESTART) {
        return LCD_COLOR_GRAY;
    }

    switch (button) {
    case DICEROLL_BUTTON_RESTART:
        return LCD_COLOR_DARKRED;
    case DICEROLL_BUTTON_BACK:
        return LCD_COLOR_ORANGE;
    case DICEROLL_BUTTON_ZERO:
        return LCD_COLOR_LIGHTGRAY;
    case DICEROLL_BUTTON_ONE:
        return LCD_COLOR_DARKGRAY;
    default:
        return LCD_COLOR_BLACK;
    }
}

static void button_bounds(DicerollButton button, uint16_t *x, uint16_t *width)
{
    switch (button) {
    case DICEROLL_BUTTON_RESTART:
        *x = 0;
        *width = DICEROLL_RESTART_WIDTH;
        break;
    case DICEROLL_BUTTON_BACK:
        *x = DICEROLL_RESTART_WIDTH;
        *width = DICEROLL_BACK_WIDTH;
        break;
    case DICEROLL_BUTTON_ZERO:
        *x = DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH;
        *width = DICEROLL_BIT_BUTTON_WIDTH;
        break;
    case DICEROLL_BUTTON_ONE:
        *x = DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH +
             DICEROLL_BIT_BUTTON_WIDTH;
        *width = DICEROLL_BIT_BUTTON_WIDTH;
        break;
    default:
        *x = 0;
        *width = 0;
        break;
    }
}

static void draw_buttons(int phrase_complete)
{
    uint16_t back_x = DICEROLL_RESTART_WIDTH;
    uint16_t zero_x = DICEROLL_RESTART_WIDTH + DICEROLL_BACK_WIDTH;
    uint16_t one_x = zero_x + DICEROLL_BIT_BUTTON_WIDTH;

    fill_button(0, DICEROLL_RESTART_WIDTH,
                button_color(DICEROLL_BUTTON_RESTART, phrase_complete));
    fill_button(back_x, DICEROLL_BACK_WIDTH,
                button_color(DICEROLL_BUTTON_BACK, phrase_complete));
    fill_button(zero_x, DICEROLL_BIT_BUTTON_WIDTH,
                button_color(DICEROLL_BUTTON_ZERO, phrase_complete));
    fill_button(one_x, DICEROLL_BIT_BUTTON_WIDTH,
                button_color(DICEROLL_BUTTON_ONE, phrase_complete));

    BSP_LCD_SetTextColor(phrase_complete ? LCD_COLOR_DARKGRAY : LCD_COLOR_BLACK);
    BSP_LCD_DrawVLine(back_x, DICEROLL_BUTTON_TOP, DICEROLL_BUTTON_HEIGHT);
    BSP_LCD_DrawVLine(zero_x, DICEROLL_BUTTON_TOP, DICEROLL_BUTTON_HEIGHT);
    BSP_LCD_DrawVLine(one_x, DICEROLL_BUTTON_TOP, DICEROLL_BUTTON_HEIGHT);

    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_DARKRED);
    display_text(10, 350, "HOLD 1 SEC");
    BSP_LCD_SetFont(&Font20);
    display_text(16, 395, "RESTART");

    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(button_color(DICEROLL_BUTTON_BACK,
                                     phrase_complete));
    display_text(173, 350, "HOLD");
    BSP_LCD_SetFont(&Font20);
    display_text(165, 395, "BACK");

    BSP_LCD_SetFont(&Font20);
    BSP_LCD_SetTextColor(phrase_complete ? LCD_COLOR_DARKGRAY : LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(button_color(DICEROLL_BUTTON_ZERO,
                                     phrase_complete));
    display_text(360, 350, "HEADS");
    BSP_LCD_SetFont(&Font24);
    display_text(386, 395, "0");

    BSP_LCD_SetFont(&Font20);
    BSP_LCD_SetTextColor(phrase_complete ? LCD_COLOR_DARKGRAY : LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(button_color(DICEROLL_BUTTON_ONE,
                                     phrase_complete));
    display_text(630, 350, "TAILS");
    BSP_LCD_SetFont(&Font24);
    display_text(656, 395, "1");
}

void diceroll_ui_draw(const MnemonicState *state)
{
    selected_word = 0U;
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    draw_title();
    draw_grid_lines();
    draw_word_cells(state);
    draw_status(state);
    draw_buttons(mnemonic_state_entropy_complete(state));
}

void diceroll_ui_update(const MnemonicState *state)
{
    selected_word = 0U;
    draw_word_cells(state);
    draw_status(state);
    draw_buttons(mnemonic_state_entropy_complete(state));
}

void diceroll_ui_draw_error(const char *message)
{
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetFont(&Font24);
    BSP_LCD_SetTextColor(LCD_COLOR_RED);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    display_text_centered(190, "HARDWARE ERROR");
    BSP_LCD_SetFont(&Font20);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    display_text_centered(240, message);
    display_text_centered(275, "CHECK POWER AND RESTART");
}

DicerollButton diceroll_ui_hit_test(uint16_t x, uint16_t y)
{
    return diceroll_layout_button_at( x, y );
}

int diceroll_ui_select_word_at(const MnemonicState *state,
                               uint16_t x, uint16_t y)
{
    uint8_t column;
    uint8_t row;
    uint8_t word_number;
    uint8_t completed;

    if (state == NULL || x >= DICEROLL_DISPLAY_WIDTH ||
        y < DICEROLL_WORD_GRID_TOP ||
        y >= DICEROLL_WORD_GRID_TOP + DICEROLL_WORD_GRID_HEIGHT) {
        return 0;
    }

    column = (uint8_t)(x / DICEROLL_WORD_COLUMN_WIDTH);
    row = (uint8_t)((y - DICEROLL_WORD_GRID_TOP) /
                    DICEROLL_WORD_ROW_HEIGHT);
    word_number = (uint8_t)(column * 6U + row + 1U);
    completed = mnemonic_state_get_completed_word_count(state);
    if (mnemonic_state_entropy_complete(state)) {
        completed = MNEMONIC_WORD_COUNT;
    }
    if (word_number > completed) {
        return 0;
    }

    selected_word = word_number;
    draw_word_cells(state);
    draw_status(state);
    return 1;
}

void diceroll_ui_show_hold_progress(DicerollButton button,
                                    uint32_t elapsed_ms,
                                    uint32_t required_ms)
{
    uint16_t x;
    uint16_t width;
    uint16_t progress;

    button_bounds(button, &x, &width);
    if (width == 0U || required_ms == 0U) {
        return;
    }
    if (elapsed_ms > required_ms) {
        elapsed_ms = required_ms;
    }
    progress = (uint16_t)(((uint32_t)(width - 1U) * elapsed_ms) / required_ms);

    BSP_LCD_SetTextColor(button_color(button, 0));
    BSP_LCD_FillRect(x + 1U, DICEROLL_BUTTON_TOP, width - 1U, 10U);
    if (progress > 0U) {
        BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
        BSP_LCD_FillRect(x + 1U, DICEROLL_BUTTON_TOP, progress, 10U);
    }
}

void diceroll_ui_clear_hold_progress(DicerollButton button,
                                     int phrase_complete)
{
    uint16_t x;
    uint16_t width;

    button_bounds(button, &x, &width);
    if (width == 0U) {
        return;
    }
    BSP_LCD_SetTextColor(button_color(button, phrase_complete));
    BSP_LCD_FillRect(x + 1U, DICEROLL_BUTTON_TOP, width - 1U, 10U);
}
