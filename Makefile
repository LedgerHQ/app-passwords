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

APPNAME ="Passwords"
APP_LOAD_PARAMS=--appFlags 0x40 --path "" --curve secp256k1 $(COMMON_LOAD_PARAMS) 

APPVERSION_M=0
APPVERSION_N=0
APPVERSION_P=5
APPVERSION=$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)

ICONNAME=icon.gif


################
# Default rule #
################
all: default

############
# Platform #
############

DEFINES   += OS_IO_SEPROXYHAL IO_SEPROXYHAL_BUFFER_SIZE_B=300
DEFINES   += HAVE_BAGL HAVE_SPRINTF
DEFINES   += HAVE_PRINTF PRINTF=screen_printf
#DEFINES   += PRINTF\(...\)=
DEFINES   += HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=4 IO_HID_EP_LENGTH=64 HAVE_USB_APDU
#DEFINES   += HAVE_USB_HIDKBD HAVE_IO_U2F HAVE_U2F U2F_PROXY_MAGIC=\"PWD\"
DEFINES   += HAVE_USB_HIDKBD
DEFINES   += LEDGER_MAJOR_VERSION=$(APPVERSION_M) LEDGER_MINOR_VERSION=$(APPVERSION_N) LEDGER_PATCH_VERSION=$(APPVERSION_P) TCS_LOADER_PATCH_VERSION=0
DEFINES   += MAX_METADATAS=4096 MAX_METANAME=20
DEFINES   += BUI_FONT_CHOOSE BUI_FONT_INCLUDE_LUCIDA_CONSOLE_8
#DEFINES   += BKB_ANIMATE
#DEFINES   += HAVE_BUI
DEFINES   += USE_CTAES

DEFINES   += APPVERSION=\"$(APPVERSION)\"

#DEFINES   += CX_COMPLIANCE_141

##############
#  Compiler  #
##############
#GCCPATH   := $(BOLOS_ENV)/gcc-arm-none-eabi-5_3-2016q1/bin/
#CLANGPATH := $(BOLOS_ENV)/clang-arm-fropi/bin/
CC       := $(CLANGPATH)clang 

#CFLAGS   += -O0
CFLAGS   += -O3 -Os

AS     := $(GCCPATH)arm-none-eabi-gcc

LD       := $(GCCPATH)arm-none-eabi-gcc
LDFLAGS  += -O3 -Os
LDLIBS   += -lm -lgcc -lc 

# import rules to compile glyphs(/pone)
include $(BOLOS_SDK)/Makefile.glyphs

### computed variables
APP_SOURCE_PATH  += src bui
SDK_SOURCE_PATH  += lib_stusb lib_stusb_impl_kbd lib_u2f


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
