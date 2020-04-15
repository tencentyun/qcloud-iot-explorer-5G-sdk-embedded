# Basic Settings
SHELL           := /bin/bash
C-SDK_DIR         ?= $(CURDIR)/qcloud-iot-explorer-sdk-embedded-c
#SUBDIRS         := directory-not-exist-actually

# Settings of input directory
SCRIPT_DIR      := $(C-SDK_DIR)/tools/build_scripts

include $(C-SDK_DIR)/make.settings
include $(SCRIPT_DIR)/parse_make_settings.mk

# Makefile echo
ifeq ($(DEBUG_MAKEFILE),n)
    Q               := @
    TOP_Q           := @
else
    Q               :=
    TOP_Q           :=
endif

# Settings of output directory
FINAL_DIR       := $(C-SDK_DIR)/output/$(BUILD_TYPE)

IOT_LIB_DIR = $(FINAL_DIR)/lib
IOT_INC_CFLAGS = -I$(FINAL_DIR)/include -I$(FINAL_DIR)/include/exports  -I$(C-SDK_DIR)/sdk_src/internal_inc

MM_LIB_DIR = $(CURDIR)/mm-lib

LDFLAGS             := -Wl,--start-group $(IOT_LIB_DIR)/libiot_sdk.a
ifeq ($(FEATURE_AUTH_WITH_NOTLS),n)
LDFLAGS             += $(IOT_LIB_DIR)/libmbedtls.a $(IOT_LIB_DIR)/libmbedx509.a $(IOT_LIB_DIR)/libmbedcrypto.a
endif
LDFLAGS             += $(IOT_LIB_DIR)/libiot_platform.a -Wl,--end-group

LDFLAGS             += $(MM_LIB_DIR)/libmodule_adaptor.a


CFLAGS += -Wall -Wno-error=sign-compare -Wno-error=format -Os -pthread -DFORCE_SSL_VERIFY
CFLAGS += ${IOT_INC_CFLAGS}
CFLAGS += -I$(MM_LIB_DIR)

ifeq ($(FEATURE_AUTH_MODE),CERT)
CFLAGS += -DAUTH_MODE_CERT
endif

EXE := module-manager

all: $(MM_LIB_DIR) $(C-SDK_DIR) $(EXE)
.PHONY: $(MM_LIB_DIR) $(C-SDK_DIR)
$(MM_LIB_DIR) $(C-SDK_DIR):
	$(MAKE) -C $@ $(MAKECMDGOALS)
$(EXE): $(addsuffix .c, $(EXE))
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean: $(MM_LIB_DIR) $(C-SDK_DIR) clean_EXE
clean_EXE:
	rm -f $(EXE)
