SRCDIR=src
BUILDDIR=build
VPATH=$(SRCDIR)
GCC=gcc
PPDEFS=-DRTL_WS_DEBUG
PROGRAM=$(BUILDDIR)/rtl-ws-server
CSOURCEFILES=$(shell ls $(SRCDIR)/*.c)
COBJFILES=$(subst .c,.o,$(subst $(SRCDIR),$(BUILDDIR),$(CSOURCEFILES)))

define compilec
$(GCC) $(PPDEFS) -c -B $(SRCDIR) $< -o $@
endef

define link
$(GCC) $^ -lpthread -lwebsockets -lrtlsdr -lrt -lm -lfftw3 -o $@
endef

.PHONY: all build prepare clean

build: $(PROGRAM)
	
all: clean build
	
$(PROGRAM): $(COBJFILES) $(CPPOBJFILES)
	$(link) 

$(BUILDDIR)/%.o: %.c | prepare
	$(compilec)

prepare:
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

