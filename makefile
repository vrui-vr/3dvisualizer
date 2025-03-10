########################################################################
# Makefile for 3D Visualizer, a generic visualization program for 3D
# multivariate gridded data.
# Copyright (c) 1999-2025 Oliver Kreylos
#
# This file is part of the WhyTools Build Environment.
# 
# The WhyTools Build Environment is free software; you can redistribute
# it and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
# 
# The WhyTools Build Environment is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the WhyTools Build Environment; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
########################################################################

# Directory containing the Vrui build system. The directory below
# matches the default Vrui installation; if Vrui's installation
# directory was changed during Vrui's installation, the directory below
# must be adapted.
VRUI_MAKEDIR := /usr/local/share/Vrui-12.3/make

# Base installation directory for 3D Visualizer and its module
# plug-ins. The module plug-ins cannot be moved from this location
# after installation, or 3D Visualizer will not find them. If this is
# set to the default of $(PWD), 3D Visualizer does not have to be
# installed to be run. 3D Visualizer's executable, plug-ins, and
# resources will be installed in the bin, lib (or lib64), and share
# directories underneath the given base directory, respectively.
# Important note: Do not use ~ as an abbreviation for the user's home
# directory here; use $(HOME) instead.
INSTALLDIR := $(shell pwd)

# Flag whether to use GLSL shaders instead of fixed OpenGL functionality
# for some visualization algorithms, especially volume rendering. This
# flag should only be set to 1 on newer, dedicated 3D graphics cards
# such as Nvidia's G80 series.
USE_SHADERS = 1

# List of default visualization modules:
# MODULE_NAMES = AnalyzeFile

MODULE_NAMES = SphericalASCIIFile \
               StructuredGridASCII \
               StructuredGridVTK \
               CitcomCUCartesianRawFile \
               CitcomCUSphericalRawFile \
               CitcomSRegionalASCIIFile \
               CitcomSGlobalASCIIFile \
               CitcomtFile \
               MultiCitcomtFile \
               CitcomtVectorFile \
               StructuredHexahedralTecplotASCIIFile \
               UnstructuredHexahedralTecplotASCIIFile \
               ImageStack \
               DicomImageStack \
               MultiChannelImageStack \
               AnalyzeFile

# List of other available modules:
# Add any of these to the MODULE_NAMES list to build them
UNSUPPORTED_MODULE_NAMES = AvsUcdAsciiFile \
                           ByteVolFile \
                           SCTFile \
                           GaleFEMVectorFile \
                           GocadVoxetFile \
                           ConvectionFile \
                           ConvectionFileCartesian \
                           CSConvectionFile \
                           EarthTomographyGrid \
                           ReifSeismicTomography \
                           SeismicTomographyModel \
                           FloatGridFile \
                           FloatVolFile \
                           MultiVolFile \
                           VecVolFile \
                           Kollmann0p9File \
                           MagaliSubductionFile \
                           MargareteSubductionFile \
                           VanKekenFile \
                           UnstructuredPlot3DFile

########################################################################
# Everything below here should not have to be changed
########################################################################

# Version number for installation subdirectories. This is used to keep
# subsequent release versions of 3D Visualizer from clobbering each
# other. The value should be identical to the major.minor version
# number found in VERSION in the root package directory.
PACKAGE_VERSION = 1.19
PACKAGE_NAME = 3DVisualizer-$(PACKAGE_VERSION)

# Set up the source directory structure
PACKAGEROOT := $(shell pwd)
RESOURCEDIR = share
SHADERDIR = $(RESOURCEDIR)/Shaders

# Include definitions for the system environment and system-provided
# packages
include $(VRUI_MAKEDIR)/SystemDefinitions
include $(VRUI_MAKEDIR)/Packages.System
include $(VRUI_MAKEDIR)/Configuration.Vrui
include $(VRUI_MAKEDIR)/Packages.Vrui

# Check if the Vrui Collaboration Infrastructure is installed
-include $(VRUI_MAKEDIR)/Configuration.Collaboration
-include $(VRUI_MAKEDIR)/Packages.Collaboration
ifdef COLLABORATION_VERSION
  HAVE_COLLABORATION = 1
else
  HAVE_COLLABORATION = 0
endif

# Add base directory to include path:
EXTRACINCLUDEFLAGS += -I$(PACKAGEROOT)

# Set destination directory for libraries and plugins:
LIBDESTDIR := $(PACKAGEROOT)/$(MYLIBEXT)
MODULESDIREXT = Modules
MODULESDESTDIR := $(LIBDESTDIR)/$(MODULESDIREXT)
MODULENAME = $(MODULESDESTDIR)/lib$(1).$(PLUGINFILEEXT)

ifneq ($(HAVE_COLLABORATION),0)
  COLLABORATIONPLUGINDESTDIR := $(LIBDESTDIR)/CollaborationPlugins
  
  COLLABORATIONPLUGINNAME = $(COLLABORATIONPLUGINDESTDIR)/lib$(1).$(PLUGINFILEEXT)
  
  # Collaboration plug-ins are installed into Vrui's plug-in
  # installation directory, not 3D Visualizer's:
  COLLABORATIONPLUGININSTALLDIR := $(COLLABORATIONPLUGINS_LIBDIR)
endif

# Set up installation directory structure:
LIBINSTALLDIR = $(INSTALLDIR)/$(MYLIBEXT)
EXECUTABLEINSTALLDIR = $(INSTALLDIR)/$(EXEDIR)
MODULESINSTALLDIR_DEBUG = $(INSTALLDIR)/$(PLUGINDIR_DEBUG)/$(MODULESDIREXT)
MODULESINSTALLDIR_RELEASE = $(INSTALLDIR)/$(PLUGINDIR_RELEASE)/$(MODULESDIREXT)
ifdef DEBUG
  MODULESINSTALLDIR = $(MODULESINSTALLDIR_DEBUG)
else
  MODULESINSTALLDIR = $(MODULESINSTALLDIR_RELEASE)
endif
ifeq ($(INSTALLDIR),$(PWD))
  SHAREINSTALLDIR = $(INSTALLDIR)/$(RESOURCEDIR)
else ifneq ($(findstring $(PACKAGE_NAME),$(INSTALLDIR)),)
  SHAREINSTALLDIR = $(INSTALLDIR)/$(RESOURCEDIR)
else
	SHAREINSTALLDIR = $(INSTALLDIR)/$(RESOURCEDIR)/$(PACKAGE_NAME)
endif

########################################################################
# Specify additional compiler and linker flags
########################################################################

CFLAGS += -Wall -pedantic

########################################################################
# List packages used by this project
# (Supported packages can be found in
# $(VRUI_MAKEDIR)/BuildRoot/Packages)
########################################################################

PACKAGES = MYGEOMETRY MYMATH MYCLUSTER MYIO MYTHREADS MYMISC

########################################################################
# Specify all final targets
########################################################################

LIBRARIES = 
EXECUTABLES = 
MODULES = 
COLLABORATIONPLUGINS = 

LIBRARIES += $(call LIBRARYNAME,libVisualizer)

EXECUTABLES += $(EXEDIR)/3DVisualizer

MODULES += $(MODULE_NAMES:%=$(call MODULENAME,%))

ifneq ($(HAVE_COLLABORATION),0)
  SHAREDVISUALIZATION_VERSION = 5
  COLLABORATIONPLUGIN_NAMES = SharedVisualization.$(SHAREDVISUALIZATION_VERSION)-Server
  COLLABORATIONPLUGINS = $(COLLABORATIONPLUGIN_NAMES:%=$(call COLLABORATIONPLUGINNAME,%))
endif

ALL = $(LIBRARIES) $(EXECUTABLES) $(MODULES) $(COLLABORATIONPLUGINS)

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
	@echo "---- 3D Visualizer configuration options: ----"
ifneq ($(USE_SHADERS),0)
	@echo "Use of GLSL shaders enabled"
else
	@echo "Use of GLSL shaders disabled"
endif
ifneq ($(HAVE_COLLABORATION),0)
	@echo "Collaborative visualization enabled"
	@echo "  Run 'make plugins-install' to install collaborative visualization server plug-in"
endif
	@echo "Selected modules: $(MODULE_NAMES)"
	@touch $(DEPDIR)/Configure-Begin

$(DEPDIR)/Configure-Package: $(DEPDIR)/Configure-Begin
	@cp Config.h.template Config.h.temp
	@$(call CONFIG_SETSTRINGVAR,Config.h.temp,VISUALIZATION_CONFIG_BASEDIR,$(INSTALLDIR))
	@$(call CONFIG_SETSTRINGVAR,Config.h.temp,VISUALIZATION_CONFIG_RESOURCEDIR,$(SHAREINSTALLDIR))
	@$(call CONFIG_SETSTRINGVAR,Config.h.temp,VISUALIZATION_CONFIG_SHADERDIR,$(SHAREINSTALLDIR)/Shaders)
	@$(call CONFIG_SETSTRINGVAR,Config.h.temp,VISUALIZATION_CONFIG_MODULEDIR_DEBUG,$(PLUGININSTALLDIR_DEBUG))
	@$(call CONFIG_SETSTRINGVAR,Config.h.temp,VISUALIZATION_CONFIG_MODULEDIR_RELEASE,$(PLUGININSTALLDIR_RELEASE))
	@$(call CONFIG_SETVAR,Config.h.temp,VISUALIZATION_CONFIG_USE_SHADERS,$(USE_SHADERS))
	@$(call CONFIG_SETVAR,Config.h.temp,VISUALIZATION_CONFIG_USE_COLLABORATION,$(HAVE_COLLABORATION))
	@if ! diff -qN Config.h.temp Config.h > /dev/null ; then cp Config.h.temp Config.h ; fi
	@rm Config.h.temp
	@touch $(DEPDIR)/Configure-Package

$(DEPDIR)/Configure-Install: $(DEPDIR)/Configure-Package
	@echo "---- $(DISPLAYNAME) installation configuration ----"
	@echo "Installation directory: $(INSTALLDIR)"
	@echo "Library directory: $(LIBINSTALLDIR)"
	@echo "Executable directory: $(EXECUTABLEINSTALLDIR)"
	@echo "Plug-in module directory: $(PLUGININSTALLDIR)"
	@echo "Resource directory: $(SHAREINSTALLDIR)"
	@touch $(DEPDIR)/Configure-Install

$(DEPDIR)/Configure-End: $(DEPDIR)/Configure-Install
	@echo "---- End of $(DISPLAYNAME) configuration options ----"
	@touch $(DEPDIR)/Configure-End

$(DEPDIR)/config: $(DEPDIR)/Configure-End
	@touch $(DEPDIR)/config

########################################################################
# Specify other actions to be performed on a `make clean'
########################################################################

.PHONY: extraclean
extraclean:
	-rm -f $(MODULE_NAMES:%=$(call MODULENAME,%))
ifneq ($(USE_COLLABORATION),0)
	-rm -f $(COLLABORATIONPLUGIN_NAMES:%=$(call COLLABORATIONPLUGINNAME,%))
endif

.PHONY: extrasqueakyclean
extrasqueakyclean:
	-rm -f $(ALL)
	-rm -rf $(PACKAGEROOT)/$(LIBEXT)

# Include basic makefile
include $(VRUI_MAKEDIR)/BasicMakefile

########################################################################
# Specify build rules for dynamic libraries
########################################################################

#
# The 3D Visualizer core library:
#

ABSTRACT_SOURCES = $(wildcard Abstract/*.cpp)

TEMPLATIZED_SOURCES = $(wildcard Templatized/*.cpp)

WRAPPERS_SOURCES = $(wildcard Wrappers/*.cpp)

CONCRETE_SOURCES = Concrete/SphericalCoordinateTransformer.cpp \
                   Concrete/EarthRenderer.cpp \
                   Concrete/PointSet.cpp

LIBVISUALIZER_SOURCES = $(ABSTRACT_SOURCES) \
                        $(TEMPLATIZED_SOURCES) \
                        $(WRAPPERS_SOURCES) \
                        $(CONCRETE_SOURCES) \
                        GLRenderState.cpp
ifneq ($(USE_SHADERS),0)
  LIBVISUALIZER_SOURCES += TwoSidedSurfaceShader.cpp \
                           TwoSided1DTexturedSurfaceShader.cpp \
                           Polyhedron.cpp \
                           Raycaster.cpp \
                           SingleChannelRaycaster.cpp \
                           TripleChannelRaycaster.cpp
else
  LIBVISUALIZER_SOURCES += VolumeRenderer.cpp \
                           PaletteRenderer.cpp
endif
LIBVISUALIZER_SOURCES += ColorBar.cpp \
                         ColorMap.cpp \
                         PaletteEditor.cpp

# List of required shaders:
LIBVISUALIZER_SHADERS = SingleChannelRaycaster.vs \
                        SingleChannelRaycaster.fs \
                        TripleChannelRaycaster.vs \
                        TripleChannelRaycaster.fs

$(call LIBOBJNAMES,$(LIBVISUALIZER_SOURCES)): | $(DEPDIR)/config

LIBVISUALIZER_PACKAGES = MYVRUI MYSCENEGRAPH MYGLMOTIF MYIMAGES MYGLGEOMETRY MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYCLUSTER MYIO MYMISC GL MATH
ifneq ($(HAVE_COLLABORATION),0)
  LIBVISUALIZER_PACKAGES += MYCOLLABORATION2CLIENT
endif
$(call LIBRARYNAME,libVisualizer): PACKAGES = $(LIBVISUALIZER_PACKAGES)
$(call LIBRARYNAME,libVisualizer): $(call LIBOBJNAMES,$(LIBVISUALIZER_SOURCES))
.PHONY: libVisualizer
libVisualizer: $(call LIBRARYNAME,libVisualizer)

LIBVISUALIZER_BASEDIR = $(PACKAGEROOT)
LIBVISUALIZER_DEPENDS = MYVRUI MYSCENEGRAPH MYGLMOTIF MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYIO MYPLUGINS MYTHREADS MYMISC
LIBVISUALIZER_DEPENDS_INLINE = MYVRUI MYSCENEGRAPH MYGLMOTIF MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYMISC
ifneq ($(HAVE_COLLABORATION),0)
  LIBVISUALIZER_DEPENDS += MYCOLLABORATION2CLIENT
  LIBVISUALIZER_DEPENDS_INLINE += MYCOLLABORATION2CLIENT
endif

LIBVISUALIZER_INCLUDE = -I.
LIBVISUALIZER_LIBDIR  = -L$(LIBVISUALIZER_BASEDIR)/$(LIBEXT)
LIBVISUALIZER_LIBS    = -lVisualizer$(LDEXT)
LIBVISUALIZER_RPATH   = $(LIBINSTALLDIR)

########################################################################
# Specify build rules for executables
########################################################################

VISUALIZER_SOURCES = BaseLocator.cpp \
                     CuttingPlaneLocator.cpp \
                     EvaluationLocator.cpp \
                     ScalarEvaluationLocator.cpp \
                     VectorEvaluationLocator.cpp \
                     Extractor.cpp \
                     ExtractorLocator.cpp \
                     ElementList.cpp

ifneq ($(HAVE_COLLABORATION),0)
  VISUALIZER_SOURCES += SharedVisualizationProtocol.cpp \
                        SharedVisualizationClient.cpp
endif

VISUALIZER_SOURCES += Visualizer.cpp

$(VISUALIZER_SOURCES:%.cpp=$(OBJDIR)/%.o): | $(DEPDIR)/config

$(EXEDIR)/3DVisualizer: PACKAGES += LIBVISUALIZER MYVRUI MYSCENEGRAPH MYGLMOTIF MYGLGEOMETRY MYGLSUPPORT MYGLWRAPPERS MYCLUSTER MYIO MYTHREADS MYREALTIME GL
ifneq ($(HAVE_COLLABORATION),0)
  $(EXEDIR)/3DVisualizer: PACKAGES += MYCOLLABORATION2CLIENT
endif
$(EXEDIR)/3DVisualizer: LINKFLAGS += $(PLUGINHOSTLINKFLAGS)
$(EXEDIR)/3DVisualizer: $(VISUALIZER_SOURCES:%.cpp=$(OBJDIR)/%.o) | $(call LIBRARYNAME,libVisualizer)
.PHONY: 3DVisualizer
3DVisualizer: $(EXEDIR)/3DVisualizer

########################################################################
# Specify build rules for plug-ins
########################################################################

# Function to generate module plug-in object file names:
MODULEOBJNAMES = $(1:%.cpp=$(OBJDIR)/pic/Concrete/%.o)

# Implicit rule for creating visualization module plug-ins:
$(call MODULENAME,%): PACKAGES += LIBVISUALIZER MYPLUGINS MYIO
ifneq ($(USE_COLLABORATION),0)
$(call MODULENAME,%): PACKAGES += MYCOLLABORATION2CLIENT
endif
$(call MODULENAME,%): $(call MODULEOBJNAMES,%.cpp) | $(call LIBRARYNAME,libVisualizer)
	@mkdir -p $(PLUGINDESTDIR)
ifdef SHOWCOMMAND
	$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINLFLAGS)
else
	@echo Linking $@...
	@$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINLFLAGS)
endif

# List of all object files potentially used for modules:
MODULE_OBJS = $(patsubst %.cpp,$(OBJDIR)/pic/%.o,$(wildcard Concrete/*.cpp))

$(MODULE_OBJS): | $(DEPDIR)/config

# Dependencies for visualization modules:
$(call MODULENAME,CitcomSRegionalASCIIFile): $(call MODULEOBJNAMES,CitcomSCfgFileParser.cpp \
                                                                   CitcomSRegionalASCIIFile.cpp)

$(call MODULENAME,CitcomSGlobalASCIIFile): $(call MODULEOBJNAMES, CitcomSCfgFileParser.cpp \
                                                                  CitcomSGlobalASCIIFile.cpp)

$(call MODULENAME,StructuredHexahedralTecplotASCIIFile): $(call MODULEOBJNAMES,TecplotASCIIFileHeaderParser.cpp \
                                                                               StructuredHexahedralTecplotASCIIFile.cpp)

$(call MODULENAME,UnstructuredHexahedralTecplotASCIIFile): $(call MODULEOBJNAMES,TecplotASCIIFileHeaderParser.cpp \
                                                                                 UnstructuredHexahedralTecplotASCIIFile.cpp)

$(call MODULENAME,ImageStack): PACKAGES += MYIMAGES
$(call MODULENAME,MultiChannelImageStack): PACKAGES += MYIMAGES

$(call MODULENAME,DicomImageStack): $(call MODULEOBJNAMES,HuffmanTable.cpp \
                                                          JPEGDecompressor.cpp \
                                                          DicomFile.cpp \
                                                          DicomImageStack.cpp)

$(call MODULENAME,UnstructuredHexahedralXdmf): PACKAGES += HDF5
$(call MODULENAME,UnstructuredHexahedralXdmf): $(call MODULEOBJNAMES,HDF5Support.cpp \
                                                                     UnstructuredHexahedralXdmf.cpp)

$(call MODULENAME,UnstructuredHexahedralVTKXML): PACKAGES += MYIO MYTHREADS
$(call MODULENAME,UnstructuredHexahedralVTKXML): $(call MODULEOBJNAMES,VTKCDataParser.cpp \
                                                                       VTKFileReader.cpp \
                                                                       VTKFile.cpp \
                                                                       UnstructuredHexahedralVTKXML.cpp)

# Keep module object files around after building:
.SECONDARY: $(MODULE_OBJS)

# Rule for shared visualization plug-in:
SHAREDVISUALIZATIONSERVER_SOURCES = SharedVisualizationProtocol.cpp \
                                    SharedVisualizationServer.cpp

$(call COLLABORATIONPLUGINNAME,SharedVisualization.$(SHAREDVISUALIZATION_VERSION)-Server): PACKAGES = MYCOLLABORATION2SERVER MYMISC
$(call COLLABORATIONPLUGINNAME,SharedVisualization.$(SHAREDVISUALIZATION_VERSION)-Server): $(call PLUGINOBJNAMES,$(SHAREDVISUALIZATIONSERVER_SOURCES))
	@mkdir -p $(COLLABORATIONPLUGINDESTDIR)
ifdef SHOWCOMMAND
	$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINLFLAGS)
else
	@echo Linking $@...
	@$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINLFLAGS)
endif
.PHONY: SharedVisualizationServer
SharedVisualizationServer: $(call COLLABORATIONPLUGINNAME,SharedVisualization.$(SHAREDVISUALIZATION_VERSION)-Server)

########################################################################
# Specify installation rules
########################################################################

#
# Rule to install 3D Visualizer in a destination directory
#

install: $(ALL)
	@echo Installing $(DISPLAYNAME) in $(INSTALLDIR)...
	@install -d $(INSTALLDIR)
# Install all library files in LIBINSTALLDIR:
	@echo Installing libraries in $(LIBINSTALLDIR)...
	@install -d $(LIBINSTALLDIR)
	@install $(LIBRARIES) $(LIBINSTALLDIR)
	@echo Configuring run-time linker...
	$(call CREATE_SYMLINK,libVisualizer)
# Install all executables in EXECUTABLEINSTALLDIR:
	@echo Installing executables in $(EXECUTABLEINSTALLDIR)...
	@install -d $(EXECUTABLEINSTALLDIR)
	@install $(EXECUTABLES) $(EXECUTABLEINSTALLDIR)
# Install all plug-ins in PLUGININSTALLDIR:
	@echo Installing plug-ins in $(PLUGININSTALLDIR)...
	@install -d $(PLUGININSTALLDIR)
	@install $(MODULES) $(PLUGININSTALLDIR)
# Install all shared files in SHAREINSTALLDIR:
	@echo Installing shared files in $(SHAREINSTALLDIR)...
	@install -d $(SHAREINSTALLDIR)
	@install $(RESOURCEDIR)/EarthTopography.png $(RESOURCEDIR)/EarthTopography.ppm $(SHAREINSTALLDIR)
ifneq ($(USE_SHADERS),0)
	@install -d $(SHAREINSTALLDIR)/Shaders
	@install $(LIBVISUALIZER_SHADERS:%=$(RESOURCEDIR)/Shaders/%) $(SHAREINSTALLDIR)/Shaders
endif
ifneq ($(HAVE_COLLABORATION),0)
	@echo Installing 3D Visualizer collaboration server plugin in $(COLLABORATIONPLUGININSTALLDIR)
	@install $(COLLABORATIONPLUGINS) $(COLLABORATIONPLUGININSTALLDIR)
endif
