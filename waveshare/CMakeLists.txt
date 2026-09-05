cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards)
set(PICO_BOARD waveshare_rp2350_touch_lcd_4.3b CACHE STRING "Target board")

include(cmake/pico_sdk_import.cmake)
project(bip39_waveshare C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(PICO_SDK_VERSION_STRING VERSION_LESS "2.2.0")
    message(FATAL_ERROR "Pico SDK 2.2.0 or newer is required")
endif()

pico_sdk_init()

# Required by the Waveshare board for reliable cold starts.
add_compile_definitions(PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)

add_library(bip39_core STATIC
    ../common/mnemonic_state.c
    ../common/sha256.c
)
target_include_directories(bip39_core PUBLIC include ../common)

add_library(waveshare_psram STATIC
    vendor/waveshare/psram/rp_pico_alloc.c
    vendor/waveshare/psram/tlsf.c
)
target_include_directories(waveshare_psram PUBLIC vendor/waveshare/psram)
target_link_libraries(waveshare_psram PUBLIC
    pico_stdlib hardware_exception hardware_flash hardware_gpio hardware_spi
    hardware_sync pico_flash)

add_library(waveshare_display STATIC
    vendor/waveshare/bsp/bsp_dma_channel_irq.c
    vendor/waveshare/bsp/bsp_st7262.c
    vendor/waveshare/bsp/pio_rgb.c)
target_include_directories(waveshare_display PUBLIC vendor/waveshare/bsp)
target_link_libraries(waveshare_display PUBLIC
    pico_stdlib hardware_dma hardware_irq hardware_pio hardware_pwm)
pico_generate_pio_header(waveshare_display
    ${CMAKE_CURRENT_LIST_DIR}/vendor/waveshare/bsp/pio_rgb.pio)

add_library(waveshare_touch STATIC
    vendor/waveshare/bsp/bsp_gt911.c
    vendor/waveshare/bsp/bsp_i2c.c)
target_include_directories(waveshare_touch PUBLIC vendor/waveshare/bsp)
target_link_libraries(waveshare_touch PUBLIC
    pico_stdlib hardware_i2c hardware_irq)

add_library(bip39_fonts STATIC
    vendor/waveshare/fonts/font12.c
    vendor/waveshare/fonts/font16_unifont.c
    vendor/waveshare/fonts/font16_compare.c
    vendor/waveshare/fonts/font20.c
    vendor/waveshare/fonts/font24.c)
target_include_directories(bip39_fonts PUBLIC vendor/waveshare/fonts)

add_library(bip39_graphics STATIC
    src/ui/graphics.c
    src/ui/coinflip_ui.c
    ../common/utf8.c)
target_include_directories(bip39_graphics PUBLIC include ../common)
target_link_libraries(bip39_graphics PUBLIC bip39_fonts)

set(COINFLIP_LANGUAGES
    english
    czech
    french
    italian
    portuguese
    spanish)

function(add_bip39_variant language target_name)
    add_library(bip39_lookup_${language} STATIC
        ../common/bip39_lookup.c)
    target_include_directories(bip39_lookup_${language} PUBLIC include ../common)
    target_compile_definitions(bip39_lookup_${language} PRIVATE
        COINFLIP_WORDLIST_FILE=\"wordlists/${language}.inc\")

    add_executable(${target_name}
        src/main.c
        src/app/coinflip_app.c
        src/platform/rp2350/clock.c)
    target_include_directories(${target_name} PRIVATE include)
    target_link_libraries(${target_name} PRIVATE
        bip39_core bip39_lookup_${language} bip39_graphics
        waveshare_display waveshare_touch waveshare_psram pico_stdlib
        hardware_clocks)
    target_compile_definitions(${target_name} PRIVATE PICO_EMBED_XIP_SETUP=1)
    pico_enable_stdio_usb(${target_name} 0)
    pico_enable_stdio_uart(${target_name} 0)
    pico_add_extra_outputs(${target_name})
endfunction()

add_bip39_variant(english bip39_waveshare)
foreach(language IN LISTS COINFLIP_LANGUAGES)
    if(NOT language STREQUAL "english")
        add_bip39_variant(${language} bip39_waveshare_${language})
    endif()
endforeach()

add_executable(bip39_waveshare_font_compare
    font_compare.c
    vendor/waveshare/fonts/font16_compare.c)
target_include_directories(bip39_waveshare_font_compare PRIVATE
    include vendor/waveshare/bsp vendor/waveshare/fonts)
target_link_libraries(bip39_waveshare_font_compare PRIVATE
    waveshare_display waveshare_psram pico_stdlib hardware_clocks)
pico_enable_stdio_usb(bip39_waveshare_font_compare 0)
pico_enable_stdio_uart(bip39_waveshare_font_compare 0)
pico_add_extra_outputs(bip39_waveshare_font_compare)
