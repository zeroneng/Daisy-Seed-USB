TARGET = ExampleComp

# Boot Type
# APP_TYPE = BOOT_QSPI

LIBDAISY_DIR = ../../libDaisy
USB_COMP_DIR = ../usb-comp
ST_USB_DEVICE_DIR = $(USB_COMP_DIR)/vendor/stm32_mw_usb_device

CPP_SOURCES = ExampleComp.cpp

C_SOURCES += \
$(USB_COMP_DIR)/usbd_conf.c \
$(USB_COMP_DIR)/usbd_desc.c \
$(USB_COMP_DIR)/usb_comp_cdc_if.c \
$(USB_COMP_DIR)/usbd_audio.c \
$(USB_COMP_DIR)/usbd_audio_if.c \
$(USB_COMP_DIR)/usbd_midi.c \
$(USB_COMP_DIR)/usbd_hid_kbd.c \
$(USB_COMP_DIR)/usbd_composite_builder.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_core.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_ctlreq.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_ioreq.c \
$(ST_USB_DEVICE_DIR)/Class/CDC/Src/usbd_cdc.c

C_INCLUDES += \
-I$(USB_COMP_DIR) \
-I$(USB_COMP_DIR)/vendor/hid/Inc \
-I$(ST_USB_DEVICE_DIR)/Core/Inc \
-I$(ST_USB_DEVICE_DIR)/Class/CDC/Inc \
-I$(ST_USB_DEVICE_DIR)/Class/CompositeBuilder/Inc

C_DEFS += \
-DUSE_USBD_COMPOSITE \
-DUSBD_CMPSIT_ACTIVATE_CDC=1 \
-DUSBD_CMPSIT_ACTIVATE_HID=1 \
-DUSBD_CMPSIT_ACTIVATE_AUDIO=1 \
-DUSBD_CMPSIT_ACTIVATE_MIDI=1 \
-DUSBD_CMPSIT_ACTIVATE_MSC=0 \
-DUSB_COMP_TEST_CDC=1 \
-DUSB_COMP_TEST_HID=0 \
-DUSB_COMP_TEST_AUDIO=1 \
-DUSB_COMP_TEST_MIDI=1 \
-DUSB_COMP_TEST_MSC=0 \
-DUSB_COMP_AUDIO_START_ON_BOOT=1 \
-DUSB_COMP_AUDIO_CAPTURE_RING_SIZE=4096 \
-DUSB_COMP_AUDIO_USE_SDRAM=1 \
-DHID_FS_BINTERVAL=0x01U

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
