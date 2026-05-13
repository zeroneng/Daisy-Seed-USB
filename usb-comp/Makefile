TARGET = UsbComp

LIBDAISY_DIR = ../libDaisy
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

CPP_SOURCES = UsbComp.cpp
C_SOURCES += \
usbd_conf.c \
usbd_desc.c \
usb_comp_cdc_if.c \
../libDaisy/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c \
../libDaisy/Middlewares/ST/STM32_USB_Device_Library/Class/HID/Src/usbd_hid.c \
../libDaisy/Middlewares/ST/STM32_USB_Device_Library/Class/CompositeBuilder/Src/usbd_composite_builder.c

C_INCLUDES += \
-I../libDaisy/src/usbd \
-I../libDaisy/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc \
-I../libDaisy/Middlewares/ST/STM32_USB_Device_Library/Class/HID/Inc \
-I../libDaisy/Middlewares/ST/STM32_USB_Device_Library/Class/CompositeBuilder/Inc

C_DEFS += \
-DUSE_USBD_COMPOSITE \
-DUSBD_CMPSIT_ACTIVATE_CDC=1 \
-DUSBD_CMPSIT_ACTIVATE_HID=1

USE_FATFS = 0

include $(SYSTEM_FILES_DIR)/Makefile
