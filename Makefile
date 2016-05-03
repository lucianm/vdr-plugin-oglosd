#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.

PLUGIN = oglosd

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep '\#define PSL_OGLOSD_VERSION ' $(PLUGIN).h | cut -d' ' -f3 | sed -e 's/[";]//g')

### The directory environment:

# Use package data if installed...otherwise assume we're under the VDR source directory:
PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell PKG_CONFIG_PATH="$$PKG_CONFIG_PATH:../../.." pkg-config --variable=$(1) vdr))
LIBDIR = $(call PKGCFG,libdir)
LOCDIR = $(call PKGCFG,locdir)
PLGCFG = $(call PKGCFG,plgcfg)
#
TMPDIR ?= /tmp
PREFIX ?= /usr
INCDIR ?= $(PREFIX)/include
PCDIR ?= $(PREFIX)/lib/pkgconfig

ifeq ($(OSD_DEBUG),1)
CONFIG += -DOSD_DEBUG
endif

ifeq ($(DEBUG_GL),1)
CONFIG += -DDEBUG_GL
endif

### Additional libraries to link against, put them in LIBSPSL for they will then also be exported in the pc file:
ifeq ($(GLES2),1)
CONFIG += -DUSE_GLES2
_CFLAGS += -g
LIBSPSL += -L/usr/local/lib -lGLESv2 -lEGL
else
_CFLAGS += $(shell pkg-config --cflags glew)
LIBSPSL += $(shell pkg-config --libs glew) -lglut
endif
_CFLAGS += $(shell pkg-config --cflags freetype2)
LIBSPSL   += $(shell pkg-config --libs freetype2)

LIBS += $(LIBSPSL)

### The compiler options:

export CFLAGS   = $(call PKGCFG,cflags) $(_CFLAGS)
export CXXFLAGS = $(call PKGCFG,cxxflags) $(_CFLAGS) -std=c++11

### The version number of VDR's plugin API:

APIVERSION = $(call PKGCFG,apiversion)

### Allow user defined options to overwrite defaults:

-include $(PLGCFG)

### The name of the distribution archive:

ARCHIVE = psl-$(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:

SOFILE = libpsl-$(PLUGIN)

### Includes and Defines (add further entries here):

INCLUDES +=

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"' $(CONFIG)

### The object files (add further files here):

OBJS = $(PLUGIN).o openglosd.o

### The main target:

all: $(SOFILE) i18n

### Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR     = po
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmo    = $(addsuffix .mo, $(foreach file, $(I18Npo), $(basename $(file))))
I18Nmsgs  = $(addprefix $(DESTDIR)$(LOCDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) --msgid-bugs-address='<see README>' -o $@ `ls $^`

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(DESTDIR)$(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	install -D -m644 $< $@

.PHONY: i18n
i18n: $(I18Nmo) $(I18Npot)

install-i18n: $(I18Nmsgs)

# generate pkg-config file:

pc-file: vdr-psl-$(PLUGIN).pc

.PHONY: vdr-psl-$(PLUGIN).pc
vdr-psl-$(PLUGIN).pc:
	@echo "Name: psl-$(PLUGIN)" > $@
	@echo "Description: VDR Plugin Shared Library for rendering OSD with the help of OpenGL/ES in various output plugins" >> $@
	@echo "URL: http://www.tvdr.de/" >> $@
	@echo "Version: $(VERSION)" >> $@
	@echo "Apiversion=$(APIVERSION)" >> $@
	@echo "" >> $@
	@echo "Requires:" >> $@
	@echo "Requires.private:" >> $@
	@echo "Libs: -lpsl-$(PLUGIN).$(APIVERSION)" >> $@
	@echo "Libs.private=$(LIBSPSL)" >> $@
	@echo "Cflags: $(_CFLAGS)" >> $@
	@echo "ldflags=-Wl,-L$(LIBDIR) -Wl,-rpath=$(LIBDIR)" >> $@

### Targets:

$(SOFILE): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) $(LIBS) -o $@

# install pkg-config file:
install-pc: pc-file
	if [ -n "$(PCDIR)" ] ; then\
	   mkdir -p $(DESTDIR)$(PCDIR) ;\
	   cp vdr-psl-$(PLUGIN).pc $(DESTDIR)$(PCDIR) ;\
	   fi

# install headers:
install-includes:
	@mkdir -p $(DESTDIR)$(INCDIR)/vdr/psl-$(PLUGIN)
	@cp -pLR *.h $(DESTDIR)$(INCDIR)/vdr/psl-$(PLUGIN)

install-lib: $(SOFILE)
	install -D $^ $(DESTDIR)$(LIBDIR)/$^.$(APIVERSION).so

install: install-lib install-i18n install-pc install-includes

dist: $(I18Npo) clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(OBJS) $(DEPFILE) $(SOFILE) vdr-psl-$(PLUGIN).pc *.tgz core* *~
