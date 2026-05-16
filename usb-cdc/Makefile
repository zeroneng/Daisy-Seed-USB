TARGET = UsbCdc

LIBDAISY_DIR = ../../libDaisy
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

CPP_SOURCES = UsbCdc.cpp
C_SOURCES += \
usbd_conf.c \
usbd_desc.c \
$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c

C_INCLUDES += -I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc

USE_FATFS = 0

include $(SYSTEM_FILES_DIR)/Makefile
