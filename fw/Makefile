#
# Makefile - Makefile of the ATUSB firmware
#
# Written 2010-2011, 2013 by Werner Almesberger
# Copyright 2010-2011, 2013 by Werner Almesberger
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#

SHELL = /bin/bash

NAME = atusb
DEBUG = false

CFLAGS = -g -mmcu=$(CHIP) -DBOOT_ADDR=$(BOOT_ADDR) \
	 -Wall -Wextra -Wshadow -Werror -Wno-unused-parameter \
	 -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes

ifeq ($(DEBUG),true)
CFLAGS += -DDEBUG
endif

ifeq ($(NAME),rzusb)
CHIP=at90usb1287
CFLAGS += -DRZUSB -DAT86RF230
else ifeq ($(NAME),hulusb)
CHIP=at90usb1287
CFLAGS += -DHULUSB -DAT86RF212
else
CHIP=atmega32u2
CFLAGS += -DATUSB -DAT86RF231
endif
HOST=jlime
BOOT_ADDR=0x7000

AVR_PREFIX = $(BIN_PATH) avr-
CC = $(AVR_PREFIX)gcc
OBJCOPY = $(AVR_PREFIX)objcopy
#OBJDUMP = $(AVR_PREFIX)objdump
SIZE = $(AVR_PREFIX)size

# BCD notion is 0xJJMM with JJ being major and MM being minor. Thus 0x0020 is
# version 0.2 */
USB_BCD_VERSION = 0030
USB_VENDOR_ID = 20b7
USB_PRODUCT_ID = 1540
USB_ID = $(USB_VENDOR_ID):$(USB_PRODUCT_ID)

OBJS = atusb.o board.o board_app.o sernum.o spi.o descr.o ep0.o \
       dfu_common.o usb.o app-atu2.o mac.o
BOOT_OBJS = boot.o board.o sernum.o spi.o flash.o dfu.o \
            dfu_common.o usb.o boot-atu2.o

ifeq ($(DEBUG),true)
OBJS +=  uart.o
endif

ifeq ($(NAME),rzusb)
OBJS += board_rzusb.o
BOOT_OBJS += board_rzusb.o
else ifeq ($(NAME),hulusb)
OBJS += board_hulusb.o
BOOT_OBJS += board_hulusb.o
else
OBJS += board_atusb.o
BOOT_OBJS += board_atusb.o
endif


vpath %.c usb/

CFLAGS += -Iinclude -Iusb -I.

# ----- Verbosity control -----------------------------------------------------

CC_normal	:= $(CC)
BUILD_normal	:=
DEPEND_normal	:= $(CPP) $(CFLAGS) -MM -MG

CC_quiet	= @echo "  CC       " $@ && $(CC_normal)
BUILD_quiet	= @echo "  BUILD    " $@ && $(BUILD_normal)
DEPEND_quiet	= @$(DEPEND_normal)

ifeq ($(V),1)
    CC		= $(CC_normal)
    BUILD	= $(BUILD_normal)
    DEPEND	= $(DEPEND_normal)
else
    CC		= $(CC_quiet)
    BUILD	= $(BUILD_quiet)
    DEPEND	= $(DEPEND_quiet)
endif

# ----- Rules -----------------------------------------------------------------

.PHONY:		all clean upload prog dfu update version.c bindist
.PHONY:		prog-app prog-read on off reset

all:		$(NAME).bin boot.hex

$(NAME).elf:	$(OBJS)
		$(MAKE) version.o
		$(CC) $(CFLAGS) -o $@ $(OBJS) version.o
		$(SIZE) $@

boot.elf:	$(BOOT_OBJS)
		$(CC) $(CFLAGS) -o $@ $(BOOT_OBJS) \
		  -Wl,--section-start=.text=$(BOOT_ADDR)
		$(SIZE) $@

%.bin:		%.elf
		$(BUILD) $(OBJCOPY) -j .text -j .data -O binary $< $@
		@echo "build #`cat .version`, `ls -l $@`"

%.dfu:		%.bin
		cp $(NAME).bin $(NAME).dfu
		dfu-suffix -a $(NAME).dfu -d 0x$(USB_BCD_VERSION) \
		  -p 0x$(USB_PRODUCT_ID) -v 0x$(USB_VENDOR_ID)

%.hex:		%.elf
		$(BUILD) $(OBJCOPY) -j .text -j .data -O ihex $< $@
		@echo "Size: `$(SIZE) -A boot.hex | sed '/Total */s///p;d'` B"

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f $(NAME).bin $(NAME).elf $(NAME).dfu
		rm -f $(OBJS) $(OBJS:.o=.d)
		rm -f boot.hex boot.elf
		rm -f $(BOOT_OBJS) $(BOOT_OBJS:.o=.d)
		rm -f version.c version.d version.o

# ----- Build version ---------------------------------------------------------

version.c:
		@if [ -f .version ]; then \
		    v=`cat .version`; \
		    expr $$v + 1 >.version; \
		else \
		    echo 0 >.version; \
		fi
		@[ -s .version ] || echo 0 >.version
		@echo '/* MACHINE-GENERATED. DO NOT EDIT ! */' >version.c
		@echo '#include "version.h"' >>version.c
		@echo "const char *build_date = \"`date`\";" >>version.c
		@echo "const uint16_t build_number = `cat .version`;" \
		  >>version.c

# ----- Dependencies ----------------------------------------------------------

MKDEP =									\
	$(DEPEND) $< |							\
	  sed 								\
	    -e 's|^$(basename $(notdir $<)).o:|$@:|'			\
	    -e '/^\(.*:\)\? */{p;s///;s/ *\\\?$$/ /;s/  */:\n/g;H;}'	\
	    -e '$${g;p;}'						\
	    -e d >$(basename $@).d;					\
	  [ "$${PIPESTATUS[*]}" = "0 0" ] ||				\
	  { rm -f $(basename $@).d; exit 1; }

%.o:            %.c
		$(CC) $(CFLAGS) -Os -c $<
		$(MKDEP)

-include $(OBJS:.o=.d)

# ----- Object file variants --------------------------------------------------

app-%.o:	usb/%.c
		$(CC) $(CFLAGS) -Os -o $@ -c $<
		$(MKDEP)

boot-%.o:	usb/%.c
		$(CC) $(CFLAGS) -DBOOT_LOADER -Os -o $@ -c $<
		$(MKDEP)

# ----- Distribution ----------------------------------------------------------

BINDIST_BASE=http://downloads.qi-hardware.com/people/werner/wpan/bindist
ATUSB_BIN_NAME=atusb-`git rev-parse HEAD | cut -c 1-7`.bin

bindist:
		qippl atusb.bin wpan/bindist/$(ATUSB_BIN_NAME)
		@echo $(BINDIST_BASE)/$(ATUSB_BIN_NAME)
		@echo md5sum: `md5sum atusb.bin | sed 's/ .*//'`
		@echo atrf-id: \
		  `sed '/.*number = \(.*\);/s//#\1/p;d' version.c` \
		  `sed '/.*date = "\(.*\)";/s//\1/p;d' version.c`

# ----- Programming and device control ----------------------------------------

upload:		$(NAME).bin boot.hex
		scp $(NAME).bin boot.hex $(HOST):

# lfuse: external clock, slow start-up
# hfuse: 4 kB boot loader, reset into boot loader
# lock: allow everything but SPM to the boot loader
#       Note: when trying to program 0xef, we get back 0x2f, failing
#	      verification. So we just program 0x2f.

prog-app:
		ssh $(HOST) avrdude -F -p $(CHIP) -c nanonote_atusb -e \
		  -U flash:w:atusb.bin:r \
		  -U lfuse:w:0x60:m

prog:
		ssh $(HOST) avrdude -F -p $(CHIP) -c nanonote_atusb -e \
		  -U flash:w:boot.hex:i \
		  -U lfuse:w:0x60:m \
		  -U hfuse:w:0xd8:m \
		  -U lock:w:0x2f:m

prog-read:
		ssh $(HOST) avrdude -F -p $(CHIP) -c nanonote_atusb \
		  -U flash:r:mcu.bin:r

dfu:		$(NAME).dfu
		dfu-util -d $(USB_ID) -D $(NAME).dfu

update:		$(NAME).bin
		-atrf-reset -a
		usbwait -r -i 0.01 -t 5 $(USB_ID)
		$(MAKE) dfu

on:
		ssh $(HOST) poke 0x10010318 4

off:
		ssh $(HOST) poke 0x10010314 4

reset:
		ssh $(HOST) poke 0x10010318 2048
		ssh $(HOST) poke 0x10010314 2048
