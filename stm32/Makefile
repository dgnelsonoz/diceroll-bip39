BUILD_DIR ?= build/$(WORDLIST)
RELEASE_DIR = release
VERSION ?= dev
LANGUAGE ?= english
WORDLIST ?= $(LANGUAGE)
PROJECT ?= bip39-stm32-$(WORDLIST)

LANGUAGES = english czech french italian portuguese spanish

CUBE ?= ../../STM32CubeF4

CC = arm-none-eabi-gcc
HOST_CC ?= cc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CPU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
DEFS = -DSTM32F469xx -DUSE_HAL_DRIVER
OPT = -Og

CFLAGS = $(CPU) $(DEFS) $(OPT) -Wall -ffunction-sections -fdata-sections
CFLAGS += -Iinc
CFLAGS += -I../common
CFLAGS += -DDICEROLL_WORDLIST_FILE=\"wordlists/$(WORDLIST).inc\"
CFLAGS += -I$(CUBE)/Drivers/CMSIS/Device/ST/STM32F4xx/Include
CFLAGS += -I$(CUBE)/Drivers/CMSIS/Include
CFLAGS += -I$(CUBE)/Drivers/STM32F4xx_HAL_Driver/Inc
CFLAGS += -I$(CUBE)/Drivers/BSP/STM32469I-Discovery
CFLAGS += -I$(CUBE)/Drivers/BSP/Components/Common
CFLAGS += -I$(CUBE)/Utilities/Fonts
CFLAGS += -I$(CUBE)/Drivers/BSP/Components/ft6x06

LDSCRIPT = STM32F469NIHX_FLASH.ld
LDFLAGS = $(CPU) -T$(LDSCRIPT) -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map
LDFLAGS += -Wl,--no-warn-rwx-segments

LDLIBS = -lc -lm -lnosys

VPATH = \
	src \
	../common \
	$(CUBE)/Drivers/STM32F4xx_HAL_Driver/Src \
	$(CUBE)/Drivers/BSP/STM32469I-Discovery \
	$(CUBE)/Drivers/BSP/Components/nt35510 \
	$(CUBE)/Drivers/BSP/Components/ft6x06

SRCS = \
	main.c \
	bip39_lookup.c \
	mnemonic_state.c \
	ui.c \
	sha256.c \
	ui_model.c \
	utf8.c \
	stm32f4xx_it.c \
	system_stm32f4xx.c \
	startup_stm32f469nihx.s \
	stm32469i_discovery_ts.c \
	ft6x06.c \
	syscalls.c \
	stm32f4xx_hal.c \
	stm32f4xx_hal_cortex.c \
	stm32f4xx_hal_gpio.c \
	stm32f4xx_hal_rcc.c \
	stm32f4xx_hal_rcc_ex.c \
	stm32f4xx_hal_pwr.c \
	stm32f4xx_hal_pwr_ex.c \
	stm32469i_discovery.c \
	stm32469i_discovery_lcd.c \
	stm32f4xx_hal_dma2d.c \
	stm32f4xx_hal_ltdc.c \
	stm32f4xx_hal_ltdc_ex.c \
	stm32f4xx_hal_dsi.c \
	stm32f4xx_hal_dma.c \
	stm32f4xx_hal_sdram.c \
	stm32f4xx_ll_fmc.c \
	stm32469i_discovery_sdram.c \
	nt35510.c \
	stm32f4xx_hal_i2c.c

OBJS = $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))
OBJS := $(OBJS:.s=.o)

all:
	@set -e; \
	for language in $(LANGUAGES); do \
		$(MAKE) WORDLIST=$$language \
			BUILD_DIR=build/$$language \
			PROJECT=bip39-stm32-$$language variant; \
	done

variant: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).bin

audit: all
	@set -e; \
	for language in $(LANGUAGES); do \
		elf=build/$$language/bip39-stm32-$$language.elf; \
		if nm -C "$$elf" | \
			rg -q 'HAL_FLASH_(Program|Ex_Erase|Unlock|Lock)|FLASH_Program'; then \
			echo "ERROR: $$elf contains flash-write symbols"; exit 1; \
		fi; \
		echo "Flash-write audit passed: $$elf"; \
	done

$(BUILD_DIR)/$(PROJECT).elf: $(OBJS) $(LDSCRIPT)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(PROJECT).bin: $(BUILD_DIR)/$(PROJECT).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPU) -c $< -o $@

flash: variant
	STM32_Programmer_CLI -c port=SWD -w $(BUILD_DIR)/$(PROJECT).elf -v -rst

release:
	@set -e; \
	for language in $(LANGUAGES); do \
		$(MAKE) WORDLIST=$$language \
			BUILD_DIR=build/$$language \
			PROJECT=bip39-stm32-$$language release-variant; \
	done

release-variant: variant
	@mkdir -p $(RELEASE_DIR)
	cp $(BUILD_DIR)/$(PROJECT).elf $(RELEASE_DIR)/$(PROJECT)-$(VERSION).elf
	cp $(BUILD_DIR)/$(PROJECT).bin $(RELEASE_DIR)/$(PROJECT)-$(VERSION).bin
	@echo "Release files: $(RELEASE_DIR)/$(PROJECT)-$(VERSION).elf and $(RELEASE_DIR)/$(PROJECT)-$(VERSION).bin"

test: $(BUILD_DIR)/test_mnemonic_state
	$(BUILD_DIR)/test_mnemonic_state

$(BUILD_DIR)/test_mnemonic_state: ../tests/test_mnemonic_state.c ../common/mnemonic_state.c ../common/sha256.c ../common/bip39_lookup.c ../common/utf8.c ../common/mnemonic_state.h ../common/sha256.h ../common/bip39_lookup.h ../common/utf8.h
	@mkdir -p $(BUILD_DIR)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -I../common ../tests/test_mnemonic_state.c ../common/mnemonic_state.c ../common/sha256.c ../common/bip39_lookup.c ../common/utf8.c -o $@

clean:
	rm -rf build

.PHONY: all audit clean flash release test variant release-variant
