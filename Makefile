SRCDIR=src
BUILDDIR=build
VPATH=$(SRCDIR)
GPP=g++ -std=c++0x
GCC=gcc
PROGRAM=$(BUILDDIR)/rtl-ws-server
CSOURCEFILES=$(shell ls $(SRCDIR)/*.c)
COBJFILES=$(subst .c,.o,$(subst $(SRCDIR),$(BUILDDIR),$(CSOURCEFILES)))
CPPSOURCEFILES=$(shell ls $(SRCDIR)/*.cpp)
CPPOBJFILES=$(subst .cpp,.o,$(subst $(SRCDIR),$(BUILDDIR),$(CPPSOURCEFILES)))

define compilecpp
$(GPP) -c -B $(SRCDIR) $< -o $@
endef

define compilec
$(GCC) -c -B $(SRCDIR) $< -o $@
endef

define link
$(GCC) $^ -lpthread -lwebsockets -lrtlsdr -lstdc++ -o $@
endef

.PHONY: all build prepare clean

build: $(PROGRAM)
	
all: clean build
	
$(PROGRAM): $(COBJFILES) $(CPPOBJFILES)
	$(link) 

$(BUILDDIR)/%.o: %.c | prepare
	$(compilec)

$(BUILDDIR)/%.o: %.cpp | prepare
	$(compilecpp)

prepare:
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

