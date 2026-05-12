TARGET = UsbMidi

LIBDAISY_DIR = ../libDaisy
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

CPP_SOURCES = UsbMidi.cpp
C_SOURCES += \
usbd_conf.c \
usbd_desc.c

USE_FATFS = 0

include $(SYSTEM_FILES_DIR)/Makefile
