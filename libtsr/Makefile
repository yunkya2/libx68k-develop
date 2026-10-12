CROSS = m68k-xelf-
CC = $(CROSS)gcc
AS = $(CROSS)gcc
LD = $(CROSS)gcc
AR = $(CROSS)ar
RANLIB = $(CROSS)ranlib

LIBDIR = ../lib
INCLUDEDIR = ../include
BINDIR = ../bin

CFLAGS = -Os -g $(INC) $(DEFS) -MMD -MP
ASFLAGS = $(INC) $(DEFS) -MMD -MP
BUILDDIR = build

INC += -Iinclude -I.

OBJS = tsr_finddev.o tsr_keepdev.o tsr_getresidentend.o \
       tsr_setdevdata.o tsr_getdevdata.o tsr_freedev.o \
       tsr_findproc.o tsr_keepproc.o tsr_freeproc.o \
       tsr_setprocdata.o tsr_getprocdata.o \
       tsr_bpb2dpb.o tsr_attachdrive.o tsr_getlogicaldrive.o \
       tsr_detachdrive.o tsr_formatdrive.o \
       tsr_parseinitparam.o tsr_heapfree.o \
       tsr_setvectors.o tsr_restorevectors.o tsr_getoldvector.o
OBJS := $(addprefix $(BUILDDIR)/,$(OBJS))

all: install

$(BUILDDIR)/libx68ktsr.a: $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

install: $(LIBDIR)/libx68ktsr.a $(INCLUDEDIR)/x68k/tsr.h \
         $(INCLUDEDIR)/x68k/dos_internal.h $(BINDIR)/m68k-xelf-tsrgen

$(LIBDIR)/libx68ktsr.a: | $(BUILDDIR)/libx68ktsr.a $(LIBDIR)
	cp -p $(BUILDDIR)/libx68ktsr.a $@

$(INCLUDEDIR)/x68k/tsr.h: | include/x68k/tsr.h $(INCLUDEDIR)/x68k
	cp -p include/x68k/tsr.h $@

$(INCLUDEDIR)/x68k/dos_internal.h: | include/x68k/dos_internal.h $(INCLUDEDIR)/x68k
	cp -p include/x68k/dos_internal.h $@

$(BINDIR)/m68k-xelf-tsrgen: | m68k-xelf-tsrgen $(BINDIR)
	cp -p m68k-xelf-tsrgen $@

$(BUILDDIR) $(LIBDIR) $(INCLUDEDIR)/x68k $(BINDIR):
	mkdir -p $@

$(BUILDDIR)/%.o: %.c include/x68k/tsr.h include/x68k/dos_internal.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/%.o: %.S | $(BUILDDIR)
	$(AS) $(ASFLAGS) -c -o $@ $<

clean:
	-rm -rf $(BUILDDIR) $(LIBDIR) $(INCLUDEDIR) $(BINDIR)

-include $(BUILDDIR)/*.d

.PHONY: all install clean
