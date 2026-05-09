TARGET = TinyUsbCdcHidExternal

LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

CPP_SOURCES = TinyUsbCdcHidExternal.cpp
C_SOURCES = \
usb_descriptors.c \
/home/pi/Developer/tinyusb/src/tusb.c \
/home/pi/Developer/tinyusb/src/common/tusb_fifo.c \
/home/pi/Developer/tinyusb/src/device/usbd.c \
/home/pi/Developer/tinyusb/src/class/cdc/cdc_device.c \
/home/pi/Developer/tinyusb/src/class/hid/hid_device.c \
/home/pi/Developer/tinyusb/src/portable/synopsys/dwc2/dcd_dwc2.c \
/home/pi/Developer/tinyusb/src/portable/synopsys/dwc2/dwc2_common.c

C_INCLUDES = \
-I$(LIBDAISY_DIR) \
-I$(LIBDAISY_DIR)/src/ \
-I$(LIBDAISY_DIR)/src/sys \
-I$(LIBDAISY_DIR)/src/usbd \
-I$(LIBDAISY_DIR)/src/usbh \
-I$(LIBDAISY_DIR)/Drivers/CMSIS/Include/ \
-I$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Include \
-I$(LIBDAISY_DIR)/Drivers/CMSIS/Device/ST/STM32H7xx/Include \
-I$(LIBDAISY_DIR)/Drivers/STM32H7xx_HAL_Driver/Inc/ \
-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Core/Inc \
-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Host_Library/Core/Inc \
-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc \
-I$(SYSTEM_FILES_DIR)/ \
-I$(DAISYSP_DIR)/Source \
-I$(LIBDAISY_DIR)/Middlewares/Third_Party/FatFs/src \
-I. \
-I/home/pi/Developer/tinyusb/src \
-I/home/pi/Developer/tinyusb/hw

C_DEFS = \
-DUSE_HAL_DRIVER \
-DSTM32H750xx \
-DHSE_VALUE=16000000 \
-DCORE_CM7 \
-DSTM32H750IB \
-DARM_MATH_CM7 \
-DUSE_FULL_LL_DRIVER \
-DCFG_TUSB_MCU=OPT_MCU_STM32H7 \
-DBOARD_TUD_RHPORT=0 \
-DBOARD_TUD_MAX_SPEED=OPT_MODE_FULL_SPEED

include $(SYSTEM_FILES_DIR)/Makefile
