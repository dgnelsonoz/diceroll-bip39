#include "rp2350_clock.h"

#include "hardware/clocks.h"

void rp2350_set_system_clock( uint32_t frequency_mhz )
{
    set_sys_clock_khz( frequency_mhz * 1000U, true );
    clock_configure( clk_peri, 0,
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                    frequency_mhz * 1000U * 1000U,
                    frequency_mhz * 1000U * 1000U );
}
