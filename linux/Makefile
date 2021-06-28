### Makefile for building and installing ReferenceFinder 4 in Unix/Linux. 
### Requires GNU Make or a compatible tool.
###   Written by Carlos A. Furuti.
###   This is part of ReferenceFinder 4 and is available under 
###     the latter's release conditions.

### Check and optionally change the next few lines in order to match 
### your platform

# Where wxWidgets is installed
WXHOME = /opt/wxGTK-2.8.0-rc1
# Binary name
PROGRAM = ReferenceFinder
# Installation directory prefix (usually /usr/local); by wxWidgets
# convention for Unix, data files will always be created in a 
# subdirectory of $(INSTALL_PREFIX)/share
INSTALL_PREFIX = /usr/local

### The remaining lines should be ok

#--- Compiling options
OPTIONS =
OPTIONS += -DAPPNAME=\"$(RF_APP_NAME_STR)\" 
OPTIONS += -DINSTALL_PREFIX=\"$(INSTALL_PREFIX)/\" 
# Temporary for experimental Linux development
OPTIONS += -DCAF

# Auxiliary wxWidgets apps
WXCONFIG = $(WXPATH)/wx-config
WXHLPCACHEDIR = $(WXPATH)/utils/hhp2cached

# Auxiliary program files
HELP = $(BUILDGENERIC)/help.zip
HELPCACHE = $(H2S)/help/reffinder.hhp.cached

BUILD ?= release

ifeq ($(BUILD),debug)
  CONDFLAGS = -DTMWX -D__RFDEBUG__ -g 
  WX = debug
else
ifeq ($(BUILD),devel)
  CONDFLAGS = -DTMWX -DRFDEBUG -g
  WX = release
else
ifeq ($(BUILD),profile)
  CONDFLAGS = -DTMWX -DRFPROFILE -O3 -g -pg
  WX = release
else
ifeq ($(BUILD),release)
  CONDFLAGS = -DTMWX -O3
  WX = release
endif
endif
endif
endif

CFLAGS = `$(WXCONFIG) --cxxflags` \
	-Wall $(CONDFLAGS) $(OPTIONS) $(INCCFLAGS)
SRCCFLAGS = $(CFLAGS) \
	-include RFPrefix.h

# Use either debug or release build directories of wxW
WXPATH = $(WXHOME)/build-$(WX)
# You may remake everything here anytime you want
BUILDGENERIC = build
# We'll leave dependency, object and binary files here
BUILDROOT = $(BUILDGENERIC)/$(BUILD)
# Relative path from here to source directory
H2S = ../source
# Extended header path
INCCFLAGS = \
	-I$(H2S)/ \
	-I$(H2S)/model \
	-I$(H2S)/gui \
	`pkg-config gtk+-2.0 --cflags`

#--- Source files (not all binaries need all of them)
MDLSRC =\
	$(H2S)/model/ReferenceFinder.cpp \
	$(H2S)/model/lexer.cpp \
	$(H2S)/model/parser.cpp
GUISRC = \
	$(H2S)/gui/RFPrefsDialog.cpp \
	$(H2S)/gui/RFStackSizer.cpp \
	$(H2S)/gui/RFHtmlHelp.cpp \
	$(H2S)/gui/RFThread.cpp \
	$(H2S)/gui/RFPrintout.cpp \
	$(H2S)/gui/RFApp.cpp \
	$(H2S)/gui/RFCanvas.cpp \
	$(H2S)/gui/RFPrefix.cpp \
	$(H2S)/gui/RFFrame.cpp
CMDSRC = \
	$(H2S)/console/ReferenceFinder_console.cpp

SRC = $(MDLSRC) $(GUISRC) $(CMDSRC)

#--- Object files
MDLOBJS = $(patsubst $(H2S)%,$(BUILDROOT)%,\
         $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(MDLSRC))))
GUIOBJS = $(patsubst $(H2S)%,$(BUILDROOT)%,\
         $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(GUISRC))))
CMDOBJS = $(patsubst $(H2S)%,$(BUILDROOT)%,\
         $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(CMDSRC))))
OBJS = $(patsubst $(H2S)%,$(BUILDROOT)%,\
         $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC))))
DEPENDS = $(patsubst $(H2S)%,$(BUILDROOT)%,\
         $(patsubst %.cpp,%.dep,$(patsubst %.c,%.dep,$(SRC))))

#--- Main target

APP = $(BUILDROOT)/$(PROGRAM)

$(APP): $(MDLOBJS) $(GUIOBJS)
	@echo Linking ReferenceFinder \($(PROGRAM)\)
	@$(CXX) -o $(APP) $(MDLOBJS) $(GUIOBJS) $(CFLAGS) \
	`$(WXCONFIG) --libs`

CMDAPP = $(BUILDROOT)/referencefinder
$(CMDAPP): $(MDLOBJS) $(CMDOBJS)
	@echo "Linking ReferenceFinder's command-line interface"  \($(CMDAPP)\)
	@$(CXX) -o $(CMDAPP) $(MDLOBJS) $(CMDOBJS) $(CFLAGS) \
	`$(WXCONFIG) --libs`

all: $(APP) $(CMDAPP)

#--- Auxiliary and optional targets

aux: $(HELP) \
	$(BUILDROOT)/getconfig

help: $(HELP)

$(HELP):  $(HELPCACHE)
	@echo Creating help file
	@(cd $(H2S)/help && zip -q - *) > $(HELP) || rm $(HELP)

# For wxW <= 2.6.3, hhp2cached doesn't correctly set exit status, so don't
# forget the "-" prefix; it also doesn't rewrite an existing cache file
$(HELPCACHE): $(WXHLPCACHEDIR)/hhp2cached \
	$(H2S)/help/*.htm $(H2S)/help/*.hhc $(H2S)/help/*.hhp \
	$(H2S)/help/*.hhk $(H2S)/help/*.css $(H2S)/help/*.png
	@echo Creating help cache
	-@rm $(HELPCACHE)
	-@$(WXHLPCACHEDIR)/hhp2cached $(H2S)/help/reffinder.hhp

#-$(WXHLPCACHEDIR)/hhp2cached $(H2S)/help/reffinder.hhp
$(WXHLPCACHEDIR)/hhp2cached:
	$(MAKE) -C $(WXHLPCACHEDIR)

$(BUILDROOT)/getconfig: getconfig.c $(H2S)/RFVersion.h
	@echo Compiling name helper
	@$(CC) $(CFLAGS) -o $(BUILDROOT)/getconfig getconfig.c

UNINSTALLER = $(BUILDROOT)/uninstall
install: all $(HELP) $(BUILDROOT)/getconfig
	@echo Installing binary program in $(INSTALL_PREFIX)/bin
	@-install -d $(INSTALL_PREFIX)/bin
	@install $(APP) $(INSTALL_PREFIX)/bin
	@install $(CMDAPP) $(INSTALL_PREFIX)/bin
	@echo Installing auxiliary files in \
          $(INSTALL_PREFIX)/share/`$(BUILDROOT)/getconfig`
	@-install -d "$(INSTALL_PREFIX)/share/`$(BUILDROOT)/getconfig`"
	@sh bundle/mkuninstall "$(UNINSTALLER)" "$(INSTALL_PREFIX)" \
	  "$(PROGRAM)" "`$(BUILDROOT)/getconfig`"
	@install $(UNINSTALLER)\
		"$(INSTALL_PREFIX)/share/`$(BUILDROOT)/getconfig`"
	@gzip -c ../LICENSE.txt > LICENSE.txt.gz
	@gzip -c ../change_log.txt > change_log.txt.gz
	@install -m 644 $(HELP) $(H2S)/about/about.htm \
		LICENSE.txt.gz change_log.txt.gz \
		$(H2S)/images/SplashScreen.png \
		resources/Icon_app_48.png \
		"$(INSTALL_PREFIX)/share/`$(BUILDROOT)/getconfig`"
	@rm LICENSE.txt.gz

dist: all $(HELP) $(BUILDROOT)/getconfig
	@sh bundle/buildbundle "`$(BUILDROOT)/getconfig -b`" \
	  $(BUILD) $(PROGRAM) $(H2S) "`$(BUILDROOT)/getconfig -p`" \
	  "`$(BUILDROOT)/getconfig -a`"

#
# Quirk: 'make clean' twice in a row causes dependencies to be rebuilt
clean: FORCE
	-@rm $(OBJS) $(DEPENDS) $(HELP) $(HELPCACHE) $(TESTS) \
	$(BUILDROOT)/getconfig 2> /dev/null

.PHONY: clean
FORCE:

buildprep: FORCE
	-@mkdir -p $(BUILDROOT)/test

#--- Rules and suffixes 

include $(DEPENDS)

.SUFFIXES: .dep .cpp

# Both object file and dependency list depend on a source file and
# all its included files (recursively)
#  (supersedes sed 's!^.*\.o[ :]*!$*.o  $@ : !g' < /tmp/mkdep.$$$$ > ...)

$(BUILDROOT)%.dep: $(H2S)%.cpp
	@echo Remaking dependencies for $<
	@-mkdir -p $(dir $(patsubst $(H2S)%,$(BUILDROOT)%,$<))
	@$(CXX) -MT $(patsubst $(H2S)%,$(BUILDROOT)%,$(<:.cpp=.o)) -MM \
           $(SRCCFLAGS) $< > /tmp/mkdep.$$$$ && \
	  sed '1i$(patsubst $(H2S)%,$(BUILDROOT)%,$(<:.cpp=.dep)) \\' \
           /tmp/mkdep.$$$$  > \
	  $(patsubst $(H2S)%,$(BUILDROOT)%,$(<:.cpp=.dep)) && \
	  rm -f /tmp/mkdep.$$$$
$(BUILDROOT)%.dep: $(H2S)%.c
	@echo Remaking dependencies for $<
	@-mkdir -p $(dir $(patsubst $(H2S)%,$(BUILDROOT)%,$<))
	@$(CXX) -MT $(patsubst $(H2S)%,$(BUILDROOT)%,$(<:.c=.o)) -MM \
           $(SRCCFLAGS) $< > /tmp/mkdep.$$$$ && \
	  sed '1i$(patsubst $(H2S)%,$(BUILDROOT)%,$(<:.c=.dep)) \\' \
           /tmp/mkdep.$$$$  > \
	  $(patsubst $(H2S)%,$(BUILDROOT)%,$(<:.c=.dep)) && \
	  rm -f /tmp/mkdep.$$$$

$(BUILDROOT)%.o: $(H2S)%.cpp
	@echo Compiling $<
	@$(CXX) -c $(SRCCFLAGS) \
          -o $(patsubst $(H2S)%,$(BUILDROOT)%,$(<:.cpp=.o)) $<
$(BUILDROOT)%.o: $(H2S)%.c
	@echo Compiling $<
	@$(CC) -c $(SRCCFLAGS) -o $(patsubst $(H2S)%,$(BUILDROOT)%,$(<:.c=.o)) $<
