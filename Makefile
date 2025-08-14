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
include $(BOLOS_SDK)/Makefile.target

########################################
#        Mandatory configuration       #
########################################
# Application name
APPNAME ="Passwords"

# Application version
APPVERSION_M=1
APPVERSION_N=3
APPVERSION_P=0
APPVERSION=$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)

# Application source files
APP_SOURCE_PATH += src

ICON_NANOSP = icons/nanox_icon_password_manager.gif
ICON_NANOX = icons/nanox_icon_password_manager.gif
ICON_STAX = icons/stax_icon_password_manager_32px.gif

ifeq ($(TARGET_NAME),$(filter $(TARGET_NAME),TARGET_NANOX TARGET_NANOS2))
    # Nano Home Screen icon
    ICON_HOME_NANO = glyphs/home_passwords_14px.gif
endif

# Application allowed derivation curves.
CURVE_APP_LOAD_PARAMS = secp256k1

# Application allowed derivation paths.
PATH_APP_LOAD_PARAMS = "5265220'"

# Setting to allow building variant applications
# - <VARIANT_PARAM> is the name of the parameter which should be set
#   to specify the variant that should be build.
# - <VARIANT_VALUES> a list of variant that can be build using this app code.
#   * It must at least contains one value.
#   * Values can be the app ticker or anything else but should be unique.
VARIANT_PARAM = NONE
VARIANT_VALUES = pwmgr

# Enabling DEBUG flag will enable PRINTF and disable optimizations
#DEBUG = 1

########################################
#     Application custom permissions   #
########################################
# See SDK `include/appflags.h` for the purpose of each permission
#HAVE_APPLICATION_FLAG_DERIVE_MASTER = 1
HAVE_APPLICATION_FLAG_GLOBAL_PIN = 1
#HAVE_APPLICATION_FLAG_BOLOS_SETTINGS = 1
#HAVE_APPLICATION_FLAG_LIBRARY = 1

########################################
# Application communication interfaces #
########################################
# ENABLE_BLUETOOTH = 1
#ENABLE_NFC = 1
ENABLE_NBGL_FOR_NANO_DEVICES = 1

########################################
#         NBGL custom features         #
########################################
# ENABLE_NBGL_QRCODE = 1
ENABLE_NBGL_KEYBOARD = 1
#ENABLE_NBGL_KEYPAD = 1

########################################
#          Features disablers          #
########################################
# These advanced settings allow to disable some feature that are by
# default enabled in the SDK `Makefile.standard_app`.
#DISABLE_STANDARD_APP_FILES = 1
#DISABLE_DEFAULT_IO_SEPROXY_BUFFER_SIZE = 1 # To allow custom size declaration
#DISABLE_STANDARD_APP_DEFINES = 1 # Will set all the following disablers
#DISABLE_STANDARD_SNPRINTF = 1
#DISABLE_STANDARD_USB = 1
#DISABLE_STANDARD_WEBUSB = 1
#DISABLE_DEBUG_LEDGER_ASSERT = 1
#DISABLE_DEBUG_THROW = 1
DISABLE_OS_IO_STACK_USE=1
DISABLE_STANDARD_U2F=1

########################################
#        Main app configuration        #
########################################

DEFINES += USE_CTAES

TESTING ?= 0
ifeq ($(TESTING), 0)
    DEFINES += HAVE_USB_HIDKBD
else
    $(info TESTING ENABLED)
    DEFINES += TESTING
endif

POPULATE ?= 0
ifneq ($(POPULATE), 0)
    $(info POPULATE ENABLED)
    DEFINES += POPULATE
endif

include $(BOLOS_SDK)/Makefile.standard_app
