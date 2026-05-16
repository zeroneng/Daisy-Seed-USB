TARGET = UsbMsc

LIBDAISY_DIR = ../../libDaisy
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

CPP_SOURCES = \
UsbMscMain.cpp \
UsbMscSdBackend.cpp

C_SOURCES += \
usbd_conf.c \
usbd_desc.c \
usbd_msc_storage.c \
$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc.c \
$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc_bot.c \
$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc_data.c \
$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc_scsi.c

C_INCLUDES += \
-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/MSC/Inc

USE_FATFS = 0

include $(SYSTEM_FILES_DIR)/Makefile
