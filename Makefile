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
APPVERSION_P=0
APPVERSION=$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)

APP_LOAD_PARAMS=--appFlags 0x40 --path "44'/1" --curve secp256k1 $(COMMON_LOAD_PARAMS)

DEFINES += APPNAME=\"$(APPNAME)\"
DEFINES += MAJOR_VERSION=$(APPVERSION_M) MINOR_VERSION=$(APPVERSION_N) PATCH_VERSION=$(APPVERSION_P)
DEFINES += APPVERSION=\"$(APPVERSION)\"

ifeq ($(TARGET_NAME),TARGET_NANOS)
    ICONNAME=icons/nanos_icon_password_manager.gif
else ifeq ($(TARGET_NAME), TARGET_FATSTACKS)
    ICONNAME=icons/nanox_icon_password_manager.gif
else
    ICONNAME=icons/nanox_icon_password_manager.gif
endif


DEFINES += OS_IO_SEPROXYHAL IO_SEPROXYHAL_BUFFER_SIZE_B=300
DEFINES += HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=4 IO_HID_EP_LENGTH=64 HAVE_USB_APDU
DEFINES += MAX_METADATAS=4096 MAX_METANAME=20
DEFINES += USE_CTAES
DEFINES += HAVE_WEBUSB WEBUSB_URL_SIZE_B=0 WEBUSB_URL=""
DEFINES += UNUSED\(x\)=\(void\)x
DEFINES += HAVE_UX_FLOW
DEFINES += HAVE_SPRINTF

TESTING:=0
ifeq ($(TESTING),0)
    $(info TESTING DISABLED)
    DEFINES   += HAVE_USB_HIDKBD
else
    $(info TESTING ENABLED)
    DEFINES   += TESTING
endif

ifneq ($(TARGET_NAME), TARGET_FATSTACKS)
    $(info Using BAGL)
    DEFINES += HAVE_BAGL
    DEFINES += HAVE_UX_FLOW
    ifneq ($(TARGET_NAME), TARGET_NANOS)
        DEFINES += HAVE_GLO096
        DEFINES += BAGL_WIDTH=128 BAGL_HEIGHT=64
        DEFINES += HAVE_BAGL_ELLIPSIS # long label truncation feature
        DEFINES += HAVE_BAGL_FONT_OPEN_SANS_REGULAR_11PX
        DEFINES += HAVE_BAGL_FONT_OPEN_SANS_EXTRABOLD_11PX
        DEFINES += HAVE_BAGL_FONT_OPEN_SANS_LIGHT_16PX
    endif
else
    $(info Using NBGL)
    DEFINES += NBGL_KEYBOARD
endif

# Enabling debug PRINTF
DEBUG:=0
ifneq ($(DEBUG),0)
    $(info DEBUG enabled)
    DEFINES += HAVE_STACK_OVERFLOW_CHECK HAVE_PRINTF
    ifeq ($(TARGET_NAME),TARGET_NANOS)
        DEFINES   += PRINTF=screen_printf
    else
        DEFINES   += PRINTF=mcu_usb_printf
    endif
else
    DEFINES   += PRINTF\(...\)=
endif

##############
#  Compiler  #
##############
ifneq ($(BOLOS_ENV),)
    $(info BOLOS_ENV=$(BOLOS_ENV))
    CLANGPATH := $(BOLOS_ENV)/clang-arm-fropi/bin/
    GCCPATH := $(BOLOS_ENV)/gcc-arm-none-eabi-5_3-2016q1/bin/
else
    $(info BOLOS_ENV is not set: falling back to CLANGPATH and GCCPATH)
endif
ifeq ($(CLANGPATH),)
    $(info CLANGPATH is not set: clang will be used from PATH)
endif
ifeq ($(GCCPATH),)
    $(info GCCPATH is not set: arm-none-eabi-* will be used from PATH)
endif

CC      := $(CLANGPATH)clang
CFLAGS  += -O3 -Os
AS      := $(GCCPATH)arm-none-eabi-gcc
LD      := $(GCCPATH)arm-none-eabi-gcc
LDFLAGS += -O3 -Os
LDLIBS  += -lm -lgcc -lc

# import rules to compile glyphs(/pone)
include $(BOLOS_SDK)/Makefile.glyphs

### computed variables
APP_SOURCE_PATH  += src
SDK_SOURCE_PATH  += lib_stusb lib_stusb_impl

ifeq ($(TARGET_NAME), TARGET_FATSTACKS)
    SDK_SOURCE_PATH += lib_nbgl/src
    SDK_SOURCE_PATH += lib_ux_fatstacks
else ifneq ($(TARGET_NAME), TARGET_NANOS)
    SDK_SOURCE_PATH  += lib_ux
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
