TARGET = UsbComp

LIBDAISY_DIR = ../../libDaisy
ST_USB_DEVICE_DIR = ../vendor/stm32_mw_usb_device
USB_COMP_ENABLE_CDC ?= 1
USB_COMP_ENABLE_HID ?= 1
USB_COMP_ENABLE_AUDIO ?= 1
USB_COMP_TEST_CDC ?= 1
USB_COMP_TEST_HID ?= 1
USB_COMP_TEST_AUDIO ?= 1

CPP_SOURCES = UsbComp.cpp
C_SOURCES += \
usbd_conf.c \
usbd_desc.c \
usb_comp_cdc_if.c \
usbd_composite_builder.c \
../usb-hid/usbd_hid_kbd.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_core.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_ctlreq.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_ioreq.c \
$(ST_USB_DEVICE_DIR)/Class/CDC/Src/usbd_cdc.c \

ifeq ($(USB_COMP_ENABLE_AUDIO),1)
C_SOURCES += \
usbd_audio.c \
usbd_audio_if.c
endif

C_INCLUDES += \
-I. \
-I../usb-hid/vendor/hid/Inc \
-I$(ST_USB_DEVICE_DIR)/Core/Inc \
-I$(ST_USB_DEVICE_DIR)/Class/CDC/Inc \
-I$(ST_USB_DEVICE_DIR)/Class/HID/Inc \
-I$(ST_USB_DEVICE_DIR)/Class/CompositeBuilder/Inc \
-I$(LIBDAISY_DIR)/src/usbd

C_DEFS += \
-DUSE_USBD_COMPOSITE \
-DUSBD_CMPSIT_ACTIVATE_CDC=$(USB_COMP_ENABLE_CDC) \
-DUSBD_CMPSIT_ACTIVATE_HID=$(USB_COMP_ENABLE_HID) \
-DUSBD_CMPSIT_ACTIVATE_AUDIO=$(USB_COMP_ENABLE_AUDIO) \
-DUSB_COMP_ENABLE_CDC=$(USB_COMP_ENABLE_CDC) \
-DUSB_COMP_ENABLE_HID=$(USB_COMP_ENABLE_HID) \
-DUSB_COMP_ENABLE_AUDIO=$(USB_COMP_ENABLE_AUDIO) \
-DUSB_COMP_TEST_CDC=$(USB_COMP_TEST_CDC) \
-DUSB_COMP_TEST_HID=$(USB_COMP_TEST_HID) \
-DUSB_COMP_TEST_AUDIO=$(USB_COMP_TEST_AUDIO) \
-DHID_FS_BINTERVAL=0x01U

USE_FATFS = 0

include ../daisy_standalone.mk
