TARGET = UsbHid

LIBDAISY_DIR = ../../libDaisy
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

CPP_SOURCES = UsbHid.cpp
C_SOURCES += \
usbd_conf.c \
usbd_desc.c \
usbd_hid_kbd.c

C_INCLUDES += -I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/HID/Inc
APP_DEFS += -DHID_FS_BINTERVAL=0x01U

USE_FATFS = 0

include $(SYSTEM_FILES_DIR)/Makefile
