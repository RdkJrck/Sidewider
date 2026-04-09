######################################
# Cesty
######################################
SDK_PATH = STM32CubeG4
PROJECT_NAME = main

# Memory definitions from linker script
FLASH_BASE = 0x08000000
FLASH_SIZE = 512K
RAM_BASE = 0x20000000
RAM_SIZE = 128K
BUILD_DIR = build

######################################
# Zdrojové soubory
######################################
C_SOURCES = \
src/main.c \
src/stm32g4xx_it.c \
$(SDK_PATH)/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c \
$(SDK_PATH)/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_cortex.c \
$(SDK_PATH)/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr.c \
$(SDK_PATH)/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr_ex.c \
$(SDK_PATH)/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc.c \
$(SDK_PATH)/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc_ex.c \
$(SDK_PATH)/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash.c \
$(SDK_PATH)/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ex.c \
$(SDK_PATH)/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_gpio.c \
$(SDK_PATH)/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pcd.c \
$(SDK_PATH)/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pcd_ex.c \
$(SDK_PATH)/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_ll_usb.c \
$(SDK_PATH)/Drivers/CMSIS/Device/ST/STM32G4xx/Source/Templates/system_stm32g4xx.c \
src/usbd_conf.c \
src/usbd_desc.c \
src/usbd_hid_gamepad.c \
src/usb_device.c \
src/sidewinder.c \
$(SDK_PATH)/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c \
$(SDK_PATH)/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c \
$(SDK_PATH)/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c 

# Startup soubor (Assembler)
AS_SOURCES = \
$(SDK_PATH)/Drivers/CMSIS/Device/ST/STM32G4xx/Source/Templates/gcc/startup_stm32g474xx.s

# Define object files with paths relative to build directory
C_OBJECTS = $(C_SOURCES:%.c=$(BUILD_DIR)/%.o)
AS_OBJECTS = $(AS_SOURCES:%.s=$(BUILD_DIR)/%.o)
OBJECTS = $(C_OBJECTS) $(AS_OBJECTS)

# VPATH tells make where to look for source files
VPATH = src:$(SDK_PATH)/Drivers/STM32G4xx_HAL_Driver/Src:$(SDK_PATH)/Drivers/CMSIS/Device/ST/STM32G4xx/Source/Templates/gcc:$(SDK_PATH)/Middlewares/ST/STM32_USB_Device_Library/Core/Src:$(SDK_PATH)/Middlewares/ST/STM32_USB_Device_Library/Class/HID/Src

######################################
# Hlavičky (Includes pro LSP)
######################################
C_INCLUDES = \
-Iinclude \
-I$(SDK_PATH)/Drivers/STM32G4xx_HAL_Driver/Inc \
-I$(SDK_PATH)/Drivers/CMSIS/Device/ST/STM32G4xx/Include \
-I$(SDK_PATH)/Drivers/CMSIS/Include \
-Icmsis_inc \
-I$(SDK_PATH)/Middlewares/ST/STM32_USB_Device_Library/Core/Inc \
-I$(SDK_PATH)/Middlewares/ST/STM32_USB_Device_Library/Class/HID/Inc \
-I$(SDK_PATH)/Projects/NUCLEO-G474RE/Templates/gcc

MEM_DEFS = 

######################################
# Kompilátor
######################################
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size

CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=hard
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# Flagy
CFLAGS = $(MCU) $(C_INCLUDES) -Inewlib_include -I$(SDK_PATH)/Drivers/CMSIS/Include -O0 -Wall -fdata-sections -ffunction-sections -g -gdwarf-2 -DSTM32G474xx 

# Linker script
LDSCRIPT = include/stm32g474xx_flash.ld
LDFLAGS = $(MCU) --specs=nano.specs --specs=nosys.specs -T$(LDSCRIPT) -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(PROJECT_NAME).map,--cref -lc -lm -lgcc

# Pravidla
all: $(BUILD_DIR)/$(PROJECT_NAME).elf

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(PROJECT_NAME).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

# Create build directory
$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean debug

debug:
	@echo "SDK_PATH = $(SDK_PATH)"
	@echo "C_SOURCES = $(C_SOURCES)"
	@echo "C_OBJECTS = $(C_OBJECTS)"
	@echo "VPATH = $(VPATH)"
	@echo "BUILD_DIR = $(BUILD_DIR)"
	@ls -la src/
