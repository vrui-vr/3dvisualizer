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
VRUI_MAKEDIR = /usr/local/share/Vrui-13.0/make

# Base installation directory for 3D Visualizer and its module
# plug-ins. The module plug-ins cannot be moved from this location
# after installation, or 3D Visualizer will not find them. If this is
# set to the default of $(PROJECT_ROOT), 3D Visualizer does not have to
# be installed to be run. 3D Visualizer's executable, plug-ins, and
# resources will be installed in the bin, lib (or lib64), and share
# directories underneath the given base directory, respectively.
# Important note: Do not use ~ as an abbreviation for the user's home
# directory here; use $(HOME) instead.
INSTALLDIR = $(PROJECT_ROOT)

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
               UnstructuredHexahedralVTKXML \
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

PROJECT_NAME = 3DVisualizer
PROJECT_DISPLAYNAME = 3D Visualizer

# Version number for installation subdirectories. This is used to keep
# subsequent release versions of 3D Visualizer from clobbering each
# other. The value should be identical to the major.minor version
# number found in VERSION in the root package directory.
PROJECT_MAJOR=1
PROJECT_MINOR=21

# Include definitions for the system environment and system-provided
# packages
include $(VRUI_MAKEDIR)/SystemDefinitions
include $(VRUI_MAKEDIR)/Packages.System
include $(VRUI_MAKEDIR)/Packages.Vrui
include $(VRUI_MAKEDIR)/Configuration.Vrui

# Check if the Vrui Collaboration Infrastructure is installed
-include $(VRUI_MAKEDIR)/Configuration.Collaboration
-include $(VRUI_MAKEDIR)/Packages.Collaboration
ifdef COLLABORATION_VERSION
  HAVE_COLLABORATION = 1
else
  HAVE_COLLABORATION = 0
endif

# Set up visualization plug-ins
MODULESDIREXT = Modules
PROJECT_MODULESDIR = $(PROJECT_ROOT)/$(PLUGINDIR)/$(MODULESDIREXT)
MODULENAME = $(PROJECT_MODULESDIR)/lib$(1).$(PLUGINFILEEXT)

# Set up installation directory structure:
ifeq ($(INSTALLDIR),$(PROJECT_ROOT))
  MODULESINSTALLDIR_DEBUG = $(INSTALLDIR)/$(PLUGINDIR_DEBUG)/$(MODULESDIREXT)
  MODULESINSTALLDIR_RELEASE = $(INSTALLDIR)/$(PLUGINDIR_RELEASE)/$(MODULESDIREXT)
else ifneq ($(findstring $(PROJECT_FULLNAME),$(INSTALLDIR)),)
  MODULESINSTALLDIR_DEBUG = $(INSTALLDIR)/$(PLUGINDIR_DEBUG)/$(MODULESDIREXT)
  MODULESINSTALLDIR_RELEASE = $(INSTALLDIR)/$(PLUGINDIR_RELEASE)/$(MODULESDIREXT)
else
  MODULESINSTALLDIR_DEBUG = $(INSTALLDIR)/$(PLUGINDIR_DEBUG)/$(PROJECT_FULLNAME)/$(MODULESDIREXT)
  MODULESINSTALLDIR_RELEASE = $(INSTALLDIR)/$(PLUGINDIR_RELEASE)/$(PROJECT_FULLNAME)/$(MODULESDIREXT)
endif
ifdef DEBUG
  MODULESINSTALLDIR = $(MODULESINSTALLDIR_DEBUG)
else
  MODULESINSTALLDIR = $(MODULESINSTALLDIR_RELEASE)
endif

########################################################################
# Specify additional compiler and linker flags
########################################################################

# Add base directory to include path:
EXTRACINCLUDEFLAGS += -I$(PROJECT_ROOT)

CFLAGS += -Wall -pedantic

########################################################################
# List common packages used by all components of this project
# (Supported packages can be found in $(VRUI_MAKEDIR)/Packages.*)
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
  # Build the shared visualization server-side collaboration plug-in
  SHAREDVISUALIZATION_NAME = SharedVisualization
  SHAREDVISUALIZATION_VERSION = 5
  COLLABORATIONPLUGINS += $(call COLLABORATIONPLUGIN_SERVER_TARGET,SHAREDVISUALIZATION)
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
	@echo "---- $(PROJECT_FULLDISPLAYNAME) configuration options: ----"
ifneq ($(USE_SHADERS),0)
	@echo "Use of GLSL shaders enabled"
else
	@echo "Use of GLSL shaders disabled"
endif
ifneq ($(HAVE_COLLABORATION),0)
	@echo "Collaborative visualization enabled"
endif
	@echo "Selected modules: $(MODULE_NAMES)"
	@touch $(DEPDIR)/Configure-Begin

$(DEPDIR)/Configure-Package: $(DEPDIR)/Configure-Begin
	@cp Config.h.template Config.h.temp
	@$(call CONFIG_SETSTRINGVAR,Config.h.temp,VISUALIZATION_CONFIG_BASEDIR,$(INSTALLDIR))
	@$(call CONFIG_SETSTRINGVAR,Config.h.temp,VISUALIZATION_CONFIG_RESOURCEDIR,$(SHAREINSTALLDIR))
	@$(call CONFIG_SETSTRINGVAR,Config.h.temp,VISUALIZATION_CONFIG_SHADERDIR,$(SHAREINSTALLDIR)/Shaders)
	@$(call CONFIG_SETSTRINGVAR,Config.h.temp,VISUALIZATION_CONFIG_MODULEDIR_DEBUG,$(MODULESINSTALLDIR_DEBUG))
	@$(call CONFIG_SETSTRINGVAR,Config.h.temp,VISUALIZATION_CONFIG_MODULEDIR_RELEASE,$(MODULESINSTALLDIR_RELEASE))
	@$(call CONFIG_SETVAR,Config.h.temp,VISUALIZATION_CONFIG_USE_SHADERS,$(USE_SHADERS))
	@$(call CONFIG_SETVAR,Config.h.temp,VISUALIZATION_CONFIG_USE_COLLABORATION,$(HAVE_COLLABORATION))
	@if ! diff -qN Config.h.temp Config.h > /dev/null ; then cp Config.h.temp Config.h ; fi
	@rm Config.h.temp
	@touch $(DEPDIR)/Configure-Package

$(DEPDIR)/Configure-Install: $(DEPDIR)/Configure-Package
	@echo "---- $(PROJECT_FULLDISPLAYNAME) installation configuration ----"
	@echo "Installation directory: $(INSTALLDIR)"
	@echo "Library directory: $(LIBINSTALLDIR)"
	@echo "Executable directory: $(EXECUTABLEINSTALLDIR)"
	@echo "Plug-in module directory: $(MODULESINSTALLDIR)"
	@echo "Resource directory: $(SHAREINSTALLDIR)"
	@echo "Shader directory: $(SHAREINSTALLDIR)/Shaders"
	@touch $(DEPDIR)/Configure-Install

$(DEPDIR)/Configure-End: $(DEPDIR)/Configure-Install
	@echo "---- End of $(PROJECT_FULLDISPLAYNAME) configuration options ----"
	@touch $(DEPDIR)/Configure-End

$(DEPDIR)/config: $(DEPDIR)/Configure-End
	@touch $(DEPDIR)/config

########################################################################
# Specify other actions to be performed on a `make clean'
########################################################################

.PHONY: extraclean
extraclean:
	-rm -f $(MODULE_NAMES:%=$(call MODULENAME,%))

.PHONY: extrasqueakyclean
extrasqueakyclean:
	-rm -f $(ALL)

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

LIBVISUALIZER_BASEDIR = $(PROJECT_ROOT)
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
	@mkdir -p $(PROJECT_MODULESDIR)
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

$(call PLUGINOBJNAMES,$(SHAREDVISUALIZATIONSERVER_SOURCES)): | $(DEPDIR)/config

$(call COLLABORATIONPLUGIN_SERVER_TARGET,SHAREDVISUALIZATION): $(call PLUGINOBJNAMES,$(SHAREDVISUALIZATIONSERVER_SOURCES))
.PHONY: SharedVisualizationServer
SharedVisualizationServer: $(call COLLABORATIONPLUGIN_SERVER_TARGET,SHAREDVISUALIZATION)

########################################################################
# Specify installation rules
########################################################################

#
# Rule to install 3D Visualizer in a destination directory
#

install: $(ALL)
	@echo Installing $(PROJECT_FULLDISPLAYNAME) in $(INSTALLDIR)...
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
	@echo Installing plug-ins in $(MODULESINSTALLDIR)...
	@install -d $(MODULESINSTALLDIR)
	@install $(MODULES) $(MODULESINSTALLDIR)
# Install all shared files in SHAREINSTALLDIR:
	@echo Installing shared files in $(SHAREINSTALLDIR)...
	@install -d $(SHAREINSTALLDIR)
	@install $(PROJECT_SHAREDIR)/EarthTopography.png $(PROJECT_SHAREDIR)/EarthTopography.ppm $(SHAREINSTALLDIR)
ifneq ($(USE_SHADERS),0)
	@install -d $(SHAREINSTALLDIR)/Shaders
	@install $(LIBVISUALIZER_SHADERS:%=$(PROJECT_SHAREDIR)/Shaders/%) $(SHAREINSTALLDIR)/Shaders
endif
ifneq ($(HAVE_COLLABORATION),0)
	@echo Installing $(PROJECT_FULLDISPLAYNAME) collaboration server plugin in $(COLLABORATIONPLUGININSTALLDIR)
	@install $(COLLABORATIONPLUGINS) $(COLLABORATIONPLUGININSTALLDIR)
endif
