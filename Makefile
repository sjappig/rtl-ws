SRCDIR=src
BUILDDIR=build
VPATH=$(SRCDIR)
GCC=gcc
NEONSUPPORT=-mfloat-abi=hard -mfpu=neon
GCCWITHNEON=$(GCC) $(NEONSUPPORT)
PPDEFS=-DRTL_WS_DEBUG
PROGRAM=$(BUILDDIR)/rtl-ws-server
CSOURCEFILES=$(shell ls $(SRCDIR)/*.c)
COBJFILES=$(subst .c,.o,$(subst $(SRCDIR),$(BUILDDIR),$(CSOURCEFILES)))

define compilecwithneon
$(GCCWITHNEON) $(PPDEFS) -c -B $(SRCDIR) $< -o $@
endef

define compilec
$(GCC) $(PPDEFS) -c -B $(SRCDIR) $< -o $@
endef

define link
$(GCCWITHNEON) $^ -lpthread -lwebsockets -lrtlsdr -lrt -lm -lfftw3 -o $@
endef

.PHONY: all build prepare clean

build: $(PROGRAM)
	
all: clean build
	
$(PROGRAM): $(COBJFILES) $(CPPOBJFILES)
	$(link) 

$(BUILDDIR)/resample.o: resample.c | prepare
	$(compilecwithneon)

$(BUILDDIR)/%.o: %.c | prepare
	$(compilec)

prepare:
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

