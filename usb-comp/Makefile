TARGET = UsbComp

LIBDAISY_DIR = ../../libDaisy
ST_USB_DEVICE_DIR = ../vendor/stm32_mw_usb_device

CPP_SOURCES = UsbComp.cpp
C_SOURCES += \
usbd_conf.c \
usbd_desc.c \
usb_comp_cdc_if.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_core.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_ctlreq.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_ioreq.c \
$(ST_USB_DEVICE_DIR)/Class/CDC/Src/usbd_cdc.c \
$(ST_USB_DEVICE_DIR)/Class/HID/Src/usbd_hid.c \
$(ST_USB_DEVICE_DIR)/Class/CompositeBuilder/Src/usbd_composite_builder.c

C_INCLUDES += \
-I$(ST_USB_DEVICE_DIR)/Core/Inc \
-I$(ST_USB_DEVICE_DIR)/Class/CDC/Inc \
-I$(ST_USB_DEVICE_DIR)/Class/HID/Inc \
-I$(ST_USB_DEVICE_DIR)/Class/CompositeBuilder/Inc \
-I$(LIBDAISY_DIR)/src/usbd

C_DEFS += \
-DUSE_USBD_COMPOSITE \
-DUSBD_CMPSIT_ACTIVATE_CDC=1 \
-DUSBD_CMPSIT_ACTIVATE_HID=1

USE_FATFS = 0

include ../daisy_standalone.mk
