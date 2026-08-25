#
# Copyright (c) 2026 Yuichi Nakamura (@yunkya2)
#
# The MIT License (MIT)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

CROSS = m68k-xelf-
CC = $(CROSS)gcc $(GCC_SPECS)
AS = $(CROSS)gcc $(GCC_SPECS)
LD = $(CROSS)gcc $(GCC_SPECS)
OBJCOPY = $(CROSS)objcopy
LINK = $(CROSS)ld
TSRGEN = $(CROSS)tsrgen

GIT_REPO_VERSION=$(shell git describe --tags --always)

DEFS += -D__IOCS_INLINE__ -D__DOS_INLINE__
CFLAGS = -Os -g $(DEFS) -MMD -MP
CFLAGS += -DGIT_REPO_VERSION=\"$(GIT_REPO_VERSION)\"
ASFLAGS = $(DEFS) -MMD -MP
LDFLAGS = -s
LDFLAGS += -Wl,-Map,$(basename $@).map -specs=nano.specs

# installed mode defaults
LIBTSR = -lx68ktsr

##############################################################################
# サンプルのビルドに開発中のライブラリ、コマンドを使用するための設定
LIB_MODE ?= local

ifeq ($(LIB_MODE),local)
LOCAL_LIBDIR := $(abspath ../lib)
LOCAL_INCLUDEDIR := $(abspath ../include)
LOCAL_BINDIR := $(abspath ../bin)
export PATH := $(LOCAL_BINDIR):$(PATH)
GCC_SPECS += -specs=$(LOCAL_LIBDIR)/x68k.specs
CFLAGS += -I$(LOCAL_INCLUDEDIR)
ASFLAGS += -I$(LOCAL_INCLUDEDIR)
LDFLAGS += -L$(LOCAL_LIBDIR)
LDFLAGS += -B$(LOCAL_LIBDIR)/ -B$(LOCAL_BINDIR)/
LIBTSR = $(LOCAL_LIBDIR)/libx68ktsr.a
LIBS += $(LOCAL_LIBDIR)/libx68k.a
TSRGEN = cd $(LOCAL_LIBDIR) && $(CROSS)tsrgen
STARTFILES = -nostartfiles $(LOCAL_LIBDIR)/x68kcrt0.o
endif

##############################################################################

TSR_SAMPLES = multidrv.x localdrv.x remotedrv.x
TSR_SAMPLES += vecthook1.x vecthook2.x
TSR_SAMPLES += minidrv1.sys minidrv2.x minidrv3.x

all: $(TSR_SAMPLES)

%.x: %.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

multidrv.x: multidrv.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBTSR) $(LIBS) $(STARTFILES)

minidrv1.sys: minidrv1.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBTSR) $(LIBS) -nostartfiles

minidrv2.x: minidrv2.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBTSR) $(LIBS) $(STARTFILES)

minidrv3.x: minidrv3main.o minidrv3.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBTSR) $(LIBS) $(STARTFILES)

minidrv3.o: minidrv3tsr.o
	$(TSRGEN) -o $(abspath $@) -u minidrv3_configure $(abspath $^)

localdrv.x: localdrv.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBTSR) $(LIBS) $(STARTFILES)

remotedrv.x: remotedrv.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBTSR) $(LIBS) $(STARTFILES)

vecthook1.x: vecthook1.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBTSR) $(LIBS) $(STARTFILES)

vecthook2.x: vecthook2main.o vecthook2.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBTSR) $(LIBS) $(STARTFILES)

vecthook2.o: vecthook2tsr.o
	$(TSRGEN) -o $(abspath $@) $(abspath $^)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.o: %.S
	$(AS) $(ASFLAGS) -c $<

clean:
	-rm -f *.o *.x *.sys *.elf *.map *.d

-include *.d

.PHONY: all clean
