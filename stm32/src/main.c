#include "stm32f4xx_hal.h"
#include "stm32469i_discovery.h"
#include "stm32469i_discovery_lcd.h"
#include "mnemonic_state.h"
#include "ui.h"
#include "stm32469i_discovery_ts.h"

enum
{
    TOUCH_POLL_MS = 20,
    BACK_HOLD_MS = 500,
    RESTART_HOLD_MS = 1000
};

static void SystemClock_Config( void );
static void FatalError( void );
static void WaitForTouchRelease( void );
static uint8_t SelectWordCount( void );
static int WaitForProtectedButton( DicerollButton button, uint32_t required_ms, int phrase_complete );

int main( void )
{
    MnemonicState mnemonic;

    HAL_Init( );
    SystemClock_Config( );

    BSP_LED_Init( LED4 );
    if( BSP_LCD_Init( ) != LCD_OK )
        FatalError( );

    BSP_LCD_LayerDefaultInit( 0, LCD_FB_START_ADDRESS );
    BSP_LCD_SelectLayer( 0 );
    if( BSP_TS_Init( 800, 480 ) != TS_OK )
    {
        ui_draw_error( "TOUCHSCREEN NOT DETECTED" );
        FatalError( );
    }
    mnemonic_state_init_words( &mnemonic, SelectWordCount( ) );
    ui_draw( &mnemonic );

    while( 1 )
    {
        TS_StateTypeDef ts_state;

        BSP_TS_GetState( &ts_state );
        if( ts_state.touchDetected )
        {
            DicerollButton button = ui_hit_test( ts_state.touchX[ 0 ], ts_state.touchY[ 0 ] );
            int phrase_complete = mnemonic_state_entropy_complete( &mnemonic );

            if( button == DICEROLL_BUTTON_RESTART )
            {
                if( WaitForProtectedButton( button, RESTART_HOLD_MS,
                                            phrase_complete ) )
                {
                    mnemonic_state_init_words( &mnemonic, mnemonic_state_get_word_count( &mnemonic ) );
                    mnemonic_state_init_words( &mnemonic, SelectWordCount( ) );
                    ui_draw( &mnemonic );
                    WaitForTouchRelease( );
                }
            }
            else if( button == DICEROLL_BUTTON_BACK &&
                     !phrase_complete &&
                     mnemonic_state_get_bit_count( &mnemonic ) > 0U )
            {
                if( WaitForProtectedButton( button, BACK_HOLD_MS,
                                            phrase_complete ) )
                {
                    mnemonic_state_backspace( &mnemonic );
                    ui_update( &mnemonic );
                    WaitForTouchRelease( );
                }
            }
            else if( !phrase_complete &&
                     ( button == DICEROLL_BUTTON_ZERO ||
                       button == DICEROLL_BUTTON_ONE ) )
            {
                uint8_t bit = button == DICEROLL_BUTTON_ONE ? 1U : 0U;

                if( mnemonic_state_add_flip( &mnemonic, bit ) == 0 )
                    ui_update( &mnemonic );
                WaitForTouchRelease( );
            }
            else if( button == DICEROLL_BUTTON_NONE &&
                     ui_select_word_at( &mnemonic, ts_state.touchX[ 0 ], ts_state.touchY[ 0 ] ) )
                WaitForTouchRelease( );
            else
                WaitForTouchRelease( );
        }

        HAL_Delay( TOUCH_POLL_MS );
    }
}

static uint8_t SelectWordCount( void )
{
    TS_StateTypeDef ts_state;
    uint8_t word_count = 0U;

    ui_draw_word_count_selection( );
    while( word_count == 0U )
    {
        BSP_TS_GetState( &ts_state );
        if( ts_state.touchDetected )
            word_count = ui_word_count_at( ts_state.touchX[ 0 ], ts_state.touchY[ 0 ] );
        HAL_Delay( TOUCH_POLL_MS );
    }
    WaitForTouchRelease( );
    return word_count;
}

static void FatalError( void )
{
    while( 1 )
    {
        BSP_LED_Toggle( LED4 );
        HAL_Delay( 250 );
    }
}

static void WaitForTouchRelease( void )
{
    TS_StateTypeDef ts_state;

    do
    {
        BSP_TS_GetState( &ts_state );
        if( ts_state.touchDetected )
            HAL_Delay( TOUCH_POLL_MS );
    }
    while( ts_state.touchDetected );
}

static int WaitForProtectedButton( DicerollButton button, uint32_t required_ms, int phrase_complete )
{
    TS_StateTypeDef ts_state;
    uint32_t started = HAL_GetTick( );
    uint32_t elapsed = 0;

    while( elapsed < required_ms )
    {
        BSP_TS_GetState( &ts_state );
        if( !ts_state.touchDetected ||
                ui_hit_test( ts_state.touchX[ 0 ],
                             ts_state.touchY[ 0 ] ) != button )
        {
            ui_clear_hold_progress( button, phrase_complete );
            if( ts_state.touchDetected )
                WaitForTouchRelease( );
            return 0;
        }

        elapsed = HAL_GetTick( ) - started;
        ui_show_hold_progress( button, elapsed, required_ms );
        HAL_Delay( TOUCH_POLL_MS );
    }

    return 1;
}


static void SystemClock_Config( void )
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;
    HAL_StatusTypeDef ret = HAL_OK;

    /* Enable Power Control clock */
    __HAL_RCC_PWR_CLK_ENABLE( );

    /*   The voltage scaling allows optimizing the power consumption when the device is
         clocked below the maximum system frequency, to update the voltage scaling value
        regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG( PWR_REGULATOR_VOLTAGE_SCALE1 );

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
#if defined( USE_STM32469I_DISCO_REVA )
    RCC_OscInitStruct.PLL.PLLM = 25;
#else
    RCC_OscInitStruct.PLL.PLLM = 8;
#endif /* USE_STM32469I_DISCO_REVA */
    RCC_OscInitStruct.PLL.PLLN = 360;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    RCC_OscInitStruct.PLL.PLLR = 6;

    ret = HAL_RCC_OscConfig( &RCC_OscInitStruct );
    if( ret != HAL_OK )
    {
        while( 1 )
        {
        }
    }

    /* Activate the OverDrive to reach the 180 MHz Frequency */
    ret = HAL_PWREx_EnableOverDrive( );
    if( ret != HAL_OK )
    {
        while( 1 )
        {
        }
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
    RCC_ClkInitStruct.ClockType = ( RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 );
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    ret = HAL_RCC_ClockConfig( &RCC_ClkInitStruct, FLASH_LATENCY_5 );
    if( ret != HAL_OK )
    {
        while( 1 )
        {
        }
    }
}
