########################################################################
# Makefile for the X3P Scan Viewer.
# Copyright (c) 1999-2026 Oliver Kreylos
#
# This file is part of the X3P Scan Viewer (X3PViewer).
# 
# The X3P Scan Viewer is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
# 
# The X3P Scan Viewer is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the X3P Scan Viewer; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA
########################################################################

# Directory containing the Vrui build system. The directory below
# matches the default Vrui installation; if Vrui's installation
# directory was changed during Vrui's installation, the directory below
# must be adapted.
VRUI_MAKEDIR ?= /usr/local/share/Vrui-15.0/make

# Base installation directory for this project. If set to the default of
# $(PROJECT_ROOT), the project does not need to be installed.
# Important note: Do not use ~ as an abbreviation for the user's home
# directory here; use $(HOME) instead.
INSTALLDIR = $(PROJECT_ROOT)

########################################################################
# Specify information about this project
########################################################################

# Name of the project
PROJECT_NAME = X3PViewer

# The display name defaults to the project's name
PROJECT_DISPLAYNAME = X3P Scan Viewer

# Version number for dynamic libraries and installation subdirectories.
# This is used to keep subsequent release versions of the project from
# clobbering each other.
PROJECT_MAJOR = 1
PROJECT_MINOR = 0
PROJECT_BUILD = 0

# Include definitions for the system environment and system-provided
# packages
include $(VRUI_MAKEDIR)/SystemDefinitions
include $(VRUI_MAKEDIR)/Packages.System
include $(VRUI_MAKEDIR)/Packages.Vrui
include $(VRUI_MAKEDIR)/Configuration.Vrui

########################################################################
# Specify additional compiler and linker flags
########################################################################

CFLAGS += -Wall -pedantic

########################################################################
# List common packages used by all components of this project
# (Supported packages can be found in $(VRUI_MAKEDIR)/Packages.*)
########################################################################

PACKAGES = MYVRUI MYGLMOTIF MYIMAGES  MYGLGEOMETRY MYGLSUPPORT MYGEOMETRY MYMATH MYIO MYMISC GL

########################################################################
# Specify all final targets
# Use $(EXEDIR)/ before executable names
########################################################################

CONFIGFILES = 
EXECUTABLES = 

CONFIGFILES += Config.h

EXECUTABLES += $(EXEDIR)/X3PViewer

ALL = $(EXECUTABLES)

.PHONY: all
all: $(ALL)

########################################################################
# Pseudo-target to print configuration options and configure the package
########################################################################

.PHONY: config config-invalidate
config: config-invalidate $(DEPDIR)/config

config-invalidate:
	@mkdir -p $(DEPDIR)
	@touch $(DEPDIR)/Configure-Begin

$(DEPDIR)/Configure-Begin:
	@mkdir -p $(DEPDIR)
	@echo "---- $(PROJECT_FULLDISPLAYNAME) configuration options: ----"
	@touch $(DEPDIR)/Configure-Begin

$(DEPDIR)/Configure-Package: $(DEPDIR)/Configure-Begin
	@touch $(DEPDIR)/Configure-Package

$(DEPDIR)/Configure-Install: $(DEPDIR)/Configure-Package
	@echo "---- $(PROJECT_FULLDISPLAYNAME) installation configuration ----"
	@echo "Installation directory: $(INSTALLDIR)"
	@echo "Executable directory: $(EXECUTABLEINSTALLDIR)"
	@echo "Resource directory: $(SHAREINSTALLDIR)"
	@touch $(DEPDIR)/Configure-Install

$(DEPDIR)/Configure-End: $(DEPDIR)/Configure-Install
	@echo "---- End of $(PROJECT_FULLDISPLAYNAME) configuration options ----"
	@touch $(DEPDIR)/Configure-End

Config.h: | $(DEPDIR)/Configure-End
	@echo "Creating Config.h configuration file"
	@cp Config.h.template Config.h.temp
	@$(call CONFIG_SETSTRINGVAR,Config.h.temp,X3PVIEWER_CONFIG_SHAREDIR,$(SHAREINSTALLDIR))
	@if ! diff -qN Config.h.temp Config.h > /dev/null ; then cp Config.h.temp Config.h ; fi
	@rm Config.h.temp

$(DEPDIR)/config: $(DEPDIR)/Configure-End $(CONFIGFILES)
	@touch $(DEPDIR)/config

########################################################################
# Specify other actions to be performed on a `make clean'
########################################################################

.PHONY: extraclean
extraclean:

.PHONY: extrasqueakyclean
extrasqueakyclean:
	-rm $(CONFIGFILES)

# Include basic makefile
include $(VRUI_MAKEDIR)/BasicMakefile

########################################################################
# Specify build rules for executables
########################################################################

X3PVIEWER_SOURCES = X3PScan.cpp \
                    ScanRotateTool.cpp \
                    LightingTool.cpp \
                    LabelTool.cpp \
                    X3PViewer.cpp

$(X3PVIEWER_SOURCES:%.cpp=$(OBJDIR)/%.o): | $(DEPDIR)/config

$(EXEDIR)/X3PViewer: $(X3PVIEWER_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: X3PViewer
X3PViewer: $(EXEDIR)/X3PViewer

install: $(ALL)
	@echo Installing $(PROJECT_DISPLAYNAME) in $(INSTALLDIR)...
	@install -d $(INSTALLDIR)
	@install -d $(EXECUTABLEINSTALLDIR)
	@install $(EXECUTABLES) $(EXECUTABLEINSTALLDIR)
	@install -d $(SHAREINSTALLDIR)
	@install -d $(SHAREINSTALLDIR)/Shaders
	@install -m u=rw,go=r $(PROJECT_SHAREDIR)/Shaders/* $(SHAREINSTALLDIR)/Shaders
