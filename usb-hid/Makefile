TARGET = UsbHid

LIBDAISY_DIR = ../../libDaisy

CPP_SOURCES = \
UsbHid.cpp \
HID.cpp

C_SOURCES += \
usbd_conf.c \
usbd_desc.c \
usbd_hid_kbd.c

C_INCLUDES += \
-Ivendor/hid/Inc

C_DEFS += \
-DHID_FS_BINTERVAL=0x01U

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
