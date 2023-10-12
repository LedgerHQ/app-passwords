#*******************************************************************************
#   Ledger App
#   (c) 2017 Ledger
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#*******************************************************************************

ifeq ($(BOLOS_SDK),)
    $(error Environment variable BOLOS_SDK is not set)
endif
include $(BOLOS_SDK)/Makefile.defines

all: default

APPNAME ="Passwords"
APPVERSION_M=1
APPVERSION_N=1
APPVERSION_P=1
APPVERSION=$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)

APP_LOAD_PARAMS=--appFlags 0x40 --path "5265220'" --curve secp256k1 $(COMMON_LOAD_PARAMS)

DEFINES += APPNAME=\"$(APPNAME)\"
DEFINES += MAJOR_VERSION=$(APPVERSION_M) MINOR_VERSION=$(APPVERSION_N) PATCH_VERSION=$(APPVERSION_P)
DEFINES += APPVERSION=\"$(APPVERSION)\"

ifeq ($(TARGET_NAME),TARGET_NANOS)
    ICONNAME=icons/nanos_icon_password_manager.gif
else ifeq ($(TARGET_NAME), TARGET_STAX)
    ICONNAME=icons/stax_icon_password_manager_32px.gif
else
    ICONNAME=icons/nanox_icon_password_manager.gif
endif


DEFINES += OS_IO_SEPROXYHAL
DEFINES += HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=4 IO_HID_EP_LENGTH=64 HAVE_USB_APDU
DEFINES += MAX_METADATAS=4096 MAX_METANAME=20
DEFINES += USE_CTAES
DEFINES += HAVE_WEBUSB WEBUSB_URL_SIZE_B=0 WEBUSB_URL=""
DEFINES += HAVE_SPRINTF

TESTING ?= 0
ifeq ($(TESTING), 0)
    $(info TESTING DISABLED)
    DEFINES   += HAVE_USB_HIDKBD
else
    $(info TESTING ENABLED)
    DEFINES   += TESTING
endif

ifneq ($(TARGET_NAME), TARGET_STAX)
    $(info Using BAGL)
    DEFINES += HAVE_BAGL
    DEFINES += HAVE_UX_FLOW
    ifneq ($(TARGET_NAME), TARGET_NANOS)
        DEFINES += IO_SEPROXYHAL_BUFFER_SIZE_B=300
        DEFINES += HAVE_GLO096
        DEFINES += BAGL_WIDTH=128 BAGL_HEIGHT=64
        DEFINES += HAVE_BAGL_ELLIPSIS # long label truncation feature
        DEFINES += HAVE_BAGL_FONT_OPEN_SANS_REGULAR_11PX
        DEFINES += HAVE_BAGL_FONT_OPEN_SANS_EXTRABOLD_11PX
        DEFINES += HAVE_BAGL_FONT_OPEN_SANS_LIGHT_16PX
    else
        DEFINES += IO_SEPROXYHAL_BUFFER_SIZE_B=128
    endif
else
    $(info Using NBGL)
    DEFINES += IO_SEPROXYHAL_BUFFER_SIZE_B=300
    DEFINES += NBGL_KEYBOARD
endif

POPULATE ?= 0
ifeq ($(POPULATE), 0)
    $(info POPULATE DISABLED)
else
    $(info POPULATE ENABLED)
    DEFINES   += POPULATE
endif

# Enabling debug PRINTF
DEBUG ?= 0
ifneq ($(DEBUG),0)
    $(info DEBUG ENABLED)
    DEFINES += HAVE_STACK_OVERFLOW_CHECK HAVE_PRINTF
    ifeq ($(TARGET_NAME),TARGET_NANOS)
        DEFINES   += PRINTF=screen_printf
    else
        DEFINES   += PRINTF=mcu_usb_printf
    endif
else
    $(info DEBUG DISABLED)
    DEFINES   += PRINTF\(...\)=
endif

##############
#  Compiler  #
##############
CC      := $(CLANGPATH)clang
AS      := $(GCCPATH)arm-none-eabi-gcc
LD      := $(GCCPATH)arm-none-eabi-gcc
LDLIBS  += -lm -lgcc -lc

# import rules to compile glyphs(/pone)
include $(BOLOS_SDK)/Makefile.glyphs

### computed variables
APP_SOURCE_PATH  += src
SDK_SOURCE_PATH  += lib_stusb lib_stusb_impl

ifneq ($(TARGET_NAME), TARGET_NANOS)
ifneq ($(TARGET_NAME), TARGET_STAX)
    SDK_SOURCE_PATH  += lib_ux
endif
endif

load: all
	python -m ledgerblue.loadApp $(APP_LOAD_PARAMS)

delete:
	python -m ledgerblue.deleteApp $(COMMON_DELETE_PARAMS)

# import generic rules from the sdk
include $(BOLOS_SDK)/Makefile.rules

#add dependency on custom makefile filename
dep/%.d: %.c Makefile

listvariants:
	@echo VARIANTS NONE pwmgr
