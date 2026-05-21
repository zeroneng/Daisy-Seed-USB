TARGET = UsbCdc

LIBDAISY_DIR = ../../libDaisy
BUILD_DIR = build

CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
AS = arm-none-eabi-gcc -x assembler-with-cpp
OBJCOPY = arm-none-eabi-objcopy

MCU = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
C_DEFS = -DCORE_CM7 -DSTM32H750xx -DSTM32H750IB -DARM_MATH_CM7 -DHSE_VALUE=16000000 -DUSE_HAL_DRIVER -DUSE_FULL_LL_DRIVER -DFILEIO_ENABLE_FATFS_READER
C_INCLUDES = \
-I$(LIBDAISY_DIR) \
-I$(LIBDAISY_DIR)/src \
-I$(LIBDAISY_DIR)/src/sys \
-I$(LIBDAISY_DIR)/src/usbd \
-I$(LIBDAISY_DIR)/src/usbh \
-I$(LIBDAISY_DIR)/Drivers/CMSIS/Include \
-I$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Include \
-I$(LIBDAISY_DIR)/Drivers/CMSIS/Device/ST/STM32H7xx/Include \
-I$(LIBDAISY_DIR)/Drivers/STM32H7xx_HAL_Driver/Inc \
-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Core/Inc \
-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc \
-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Host_Library/Core/Inc \
-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc \
-I$(LIBDAISY_DIR)/Middlewares/Third_Party/FatFs/src \
-I$(LIBDAISY_DIR)/core

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) -include stm32h7xx.h -O2 -Wall -Wno-missing-attributes -Wno-stringop-overflow -fdata-sections -ffunction-sections -g -ggdb -MMD -MP -std=gnu11
CPPFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -fno-unwind-tables -fshort-enums -Wno-register -std=gnu++14
LDFLAGS = $(MCU) --specs=nano.specs --specs=nosys.specs -T$(LIBDAISY_DIR)/core/STM32H750IB_flash.lds -L$(LIBDAISY_DIR)/build -ldaisy -lc -lm -lnosys -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections -Wl,--print-memory-usage

CPP_SOURCES = UsbCdc.cpp
C_SOURCES = usbd_conf.c usbd_desc.c $(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c $(LIBDAISY_DIR)/core/startup_stm32h750xx.c
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o))) $(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SOURCES:.cpp=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.cpp $(sort $(dir $(CPP_SOURCES)))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.cpp Makefile | $(BUILD_DIR)
	$(CXX) -c $(CPPFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(OBJCOPY) -O binary -S $< $@

clean:
	rm -rf $(BUILD_DIR)

program: $(BUILD_DIR)/$(TARGET).elf
	openocd -s /usr/local/share/openocd/scripts -f interface/stlink.cfg -f target/stm32h7x.cfg -c "program ./build/$(TARGET).elf verify reset exit"

-include $(wildcard $(BUILD_DIR)/*.d)
