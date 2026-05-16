TARGET = UsbComp

LIBDAISY_DIR = ../../libDaisy
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

CPP_SOURCES = UsbComp.cpp
C_SOURCES += \
usbd_conf.c \
usbd_desc.c \
usb_comp_cdc_if.c \
$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c \
$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/HID/Src/usbd_hid.c \
$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/CompositeBuilder/Src/usbd_composite_builder.c

C_INCLUDES += \
-I$(LIBDAISY_DIR)/src/usbd \
-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc \
-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/HID/Inc \
-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/CompositeBuilder/Inc

C_DEFS += \
-DUSE_USBD_COMPOSITE \
-DUSBD_CMPSIT_ACTIVATE_CDC=1 \
-DUSBD_CMPSIT_ACTIVATE_HID=1

USE_FATFS = 0

include $(SYSTEM_FILES_DIR)/Makefile
