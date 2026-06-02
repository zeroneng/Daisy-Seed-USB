TARGET = UsbMsc

LIBDAISY_DIR = ../../libDaisy
ST_USB_DEVICE_DIR = ../vendor/stm32_mw_usb_device

CPP_SOURCES = \
UsbMscMain.cpp \
UsbMscSdBackend.cpp

C_SOURCES += \
usbd_conf.c \
usbd_desc.c \
usbd_msc_storage.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_core.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_ctlreq.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_ioreq.c \
$(ST_USB_DEVICE_DIR)/Class/MSC/Src/usbd_msc.c \
$(ST_USB_DEVICE_DIR)/Class/MSC/Src/usbd_msc_bot.c \
$(ST_USB_DEVICE_DIR)/Class/MSC/Src/usbd_msc_data.c \
$(ST_USB_DEVICE_DIR)/Class/MSC/Src/usbd_msc_scsi.c

C_INCLUDES += \
-I$(ST_USB_DEVICE_DIR)/Core/Inc \
-I$(ST_USB_DEVICE_DIR)/Class/MSC/Inc

USE_FATFS = 0

include ../daisy_standalone.mk
