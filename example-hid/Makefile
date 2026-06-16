TARGET = ExampleHid

LIBDAISY_DIR = ../../libDaisy
OPT = -Ofast
USB_HID_DIR = ../usb-hid

EXAMPLE_HID_TEST_KEYS ?= 1

CPP_SOURCES = ExampleHid.cpp

C_SOURCES += \
$(USB_HID_DIR)/usbd_conf.c \
$(USB_HID_DIR)/usbd_desc.c \
$(USB_HID_DIR)/usbd_hid_kbd.c

C_INCLUDES += \
-I$(USB_HID_DIR) \
-I$(USB_HID_DIR)/vendor/hid/Inc

C_DEFS += \
-DHID_FS_BINTERVAL=0x01U \
-DEXAMPLE_HID_TEST_KEYS=$(EXAMPLE_HID_TEST_KEYS)

CPP_SOURCES += \
$(USB_HID_DIR)/HID.cpp

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
