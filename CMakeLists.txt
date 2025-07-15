#
# Copyright 2017, 2018, 2019, 2024, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
#
# This file is part of AwesomeMediaLibraryManager.
#
# AwesomeMediaLibraryManager is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# AwesomeMediaLibraryManager is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with AwesomeMediaLibraryManager.  If not, see <http://www.gnu.org/licenses/>.
#

# @file CMakeLists.txt

# References:
# Too many to list.  Some of the critical ones:
# https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1  ("Effective Modern CMake")
# https://github.com/euler0/mini-cmake-qt (QtCommon et al, Public Domain, https://github.com/euler0/mini-cmake-qt/blob/master/LICENSE)
# https://cmake.org/cmake/help/v3.0/manual/cmake-qt.7.html
# Installer refs:
# https://github.com/skypjack/qt_cpackifw
# https://cmake.org/cmake/help/v3.9/module/CPackIFW.html
# https://github.com/kracejic/cleanCppProject/blob/master/CMakeLists.txt

# @note For QtIFW installer:
# - On Linux, CPACK_IFW_ROOT (cmake var) or QTIFWDIR (env var) should point to something like ~/Qt/QtIFW-3.0.1


# cmake setup.
cmake_minimum_required(VERSION 3.27 FATAL_ERROR)
cmake_policy(SET CMP0053 NEW) # Use new var ref/esc seq eval
cmake_policy(SET CMP0010 NEW) # Silence warning on CMP0010.

message(STATUS "CMake-time ENV PATH: $ENV{PATH}")
message(STATUS "CMake-time ENV CMAKE_PREFIX_PATH: $ENV{CMAKE_PREFIX_PATH}")
message(STATUS "CMake-time ENV CMAKE_MODULE_PATH: $ENV{CMAKE_MODULE_PATH}")
include(cmake/GetVersionInfo.cmake)

# Prevent an in-root-of-src-tree build and any accidental build-induced changes to source files.
# Per @link https://stackoverflow.com/questions/1208681/with-cmake-how-would-you-disable-in-source-builds, these have to
# be prior to project().
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message("ERROR: ${CMAKE_SOURCE_DIR} == ${CMAKE_BINARY_DIR}")
	message(FATAL_ERROR "Prevented in-root-of-tree build. Please create a build directory (inside or outside) of the source code root directory and call cmake from there. Thank you.")
endif()

if(FALSE)
	### Set up the <LANG>_COMPILER_LAUNCHERs.
	### @note "This property is initialized by the value of 
	###       the CMAKE_<LANG>_COMPILER_LAUNCHER variable if it is set when a target is created."

	# Use ccache if it's available.
	find_program(CCACHE_PROGRAM ccache)
	if(CCACHE_PROGRAM)
		set_property(GLOBAL PROPERTY CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
	endif()
#	if(TRUE) # csbuild - the static analysis one.
#		set_property(GLOBAL PROPERTY CMAKE_CXX_COMPILER_LAUNCHER "csbuild;--print-current;-c")
#	endif()
endif()
if(FALSE)
	set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE, "include-what-you-use;-w;-Xiwyu;--verbose=7")
endif()

# Basic information about project
# Note that here is where CMake determines compilers and loads any toolchain.cmake file.
# (per https://cgold.readthedocs.io/en/latest/tutorials/project.html)
project(AwesomeMediaLibraryManager
        VERSION ${GVI_VERSION_TAG_QUAD}
        DESCRIPTION "The Awesome Media Library Manager"
        LANGUAGES C CXX)

# Additional project info.
set(PROJECT_AUTHOR "Gary R. Van Sickle")
set(PROJECT_AUTHOR_EMAIL "grvs@users.sourceforge.net")
set(FULL_COPYRIGHT_STRING "Copyright (c) 2017, 2018, 2019, 2024, 2025 ${PROJECT_AUTHOR} <${PROJECT_AUTHOR_EMAIL}>")

# Early include of some basic cmake stuff.
include(CMakePrintHelpers)

### Set the required language standards.
# C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# C++
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Min versions of Qt, KF, KF/ECM.
set(QT_MIN_VERSION "6.9.0")
set(KF_MIN_VERSION "6.11.0") ### @win temp: Only have 5.52.0
set(ECM_MIN_VERSION "6.3.0") #"5.57.0")

###
### IDE helper setup
###
source_group(TREE ${CMAKE_SOURCE_DIR})

###
### Qt setup.
### @{
set(AUTOGEN_SOURCE_GROUP autogen)
set(AUTOMOC_SOURCE_GROUP automoc)
set(AUTORCC_SOURCE_GROUP autorcc)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_AUTOGEN_PARALLEL 8)
### @}

###
### ECM Setup ====================================================================================================
###

# KDE extra-cmake-modules, see https://api.kde.org/ecm/
# @note find_package() MODULE vs. NO_MODULE (==CONFIG mode):
# "Config mode provides an elaborate interface and search procedure. Much of the interface is provided for completeness
#  and for use internally by find-modules loaded by Module mode. Most user code should simply call:
#    find_package(<package> [major[.minor]] [EXACT] [REQUIRED|QUIET])"
### @todo NO WIN:
find_package(PkgConfig REQUIRED) # @note I see PkgConfig just before ECM in a few KDE scripts.
find_package(ECM ${ECM_MIN_VERSION} REQUIRED NO_MODULE)

# Add dirs where we keep support *.cmake's, ucm, ECM, etc.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake
		${CMAKE_SOURCE_DIR}/cmake/ucm/cmake
		${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR}
)

###
### ECM Macros
### Note: The order here follows that of KDevelop and Calligra's top-level CMakeLists.txt's.  Some macros used there are omitted here.
###

# ECM KDE macros
# @note Order matters for these next two include()s.  Per the docs, InstallDirs needs to come before CMakeSettings.
# @link https://api.kde.org/ecm/kde-module/KDECMakeSettings.html#runtime-paths
include(KDEInstallDirs6)  ##< Replaces/uses GNUInstallDirs.
#set(APPLE_FORCE_X11 ON) # This applies to KDECMakeSettings.
set(ENABLE_CLAZY ON) # This applies to KDECMakeSettings.
include(KDECMakeSettings NO_POLICY_SCOPE)

# CMake macros.
include(FeatureSummary)

# ECM macros.
include(ECMOptionalAddSubdirectory)
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(ECMAddTests)
include(ECMMarkAsTest)
include(ECMQtDeclareLoggingCategory)

# Sanitizers.
# @note KF5:
# The sanitizers are not enabled by default. Instead, you must set ECM_ENABLE_SANITIZERS (either in your
# CMakeLists.txt or on the command line) to a semicolon-separated list of sanitizers you wish to enable.
# The options are:
#   address memory thread leak undefined fuzzer
# The sanitizers "address", "memory" and "thread" are mutually exclusive. You cannot enable two of them in the same build.
# "leak" requires the "address" sanitizer.
### @note KDECompileFlags includes include(ECMEnableSanitizers).
#set(ECM_ENABLE_SANITIZERS "leak;address")
#include(ECMEnableSanitizers)

# KDE macros.
# @see https://api.kde.org/ecm/manual/ecm-kde-modules.7.html
# @note Don't set the to true:
# "If KDE_INSTALL_DIRS_NO_DEPRECATED is set to TRUE, only those variables defined by the GNUInstallDirs module
# (shipped with CMake) are defined."
# That is what happens, but I'm not clear on why this var would control that; are all the KDE Install Dirs deprecated?
#set(KDE_INSTALL_DIRS_NO_DEPRECATED TRUE)


# KDECompilerSettings
# Enables more warnings.  Transitively includes include(ECMEnableSanitizers).
# @note This disables exceptions.  We'll turn them back on with kde_enable_exceptions() below.
# @warning This also sets the compiler stds way too low (C89(!!))
# @see https://api.kde.org/ecm/kde-module/KDECompilerSettings.html
#include(KDECompilerSettings NO_POLICY_SCOPE)
#kde_enable_exceptions()
# And undo even more of what KDECompilerSettings did to/for us.
###string(REPLACE "-Werror=undef" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
# string(REGEX REPLACE "-O[^;]*" "-Og" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
# string(REGEX REPLACE "-g[^;]*" "-ggdb3" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
# string(REGEX REPLACE "-O[^;]*" "-Og" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
# string(REGEX REPLACE "-g[^;]*" "-ggdb3" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
#list(REMOVE_DUPLICATES CMAKE_CXX_FLAGS_DEBUG)

###
### END ECM Setup ====================================================================================================
###

# Set some extra debug flags early.
# This is stupid, but I can't for the life of me figure out a better way to set the compiler options in
# cmake and/or Qt Creator.
# https://github.com/onqtam/ucm#ucm_add_flags
include(ucm)
# Target-focused compile settings.
add_library(cxx_settings INTERFACE)
target_compile_features(cxx_settings INTERFACE) # cxx_std_23)
add_library(cxx_definitions_qt INTERFACE)
target_compile_definitions(cxx_definitions_qt INTERFACE)
add_library(cxx_compile_options INTERFACE)
target_compile_options(cxx_compile_options INTERFACE)
if(UNIX)
	target_compile_options(cxx_compile_options
		INTERFACE
			-Og -ggdb3 -fno-omit-frame-pointer -fmessage-length=0 -pthread -Werror=return-type
		-fno-eliminate-unused-debug-types -fno-inline -fmodules)
	ucm_add_flags(-Og -ggdb3 -fno-omit-frame-pointer -fmessage-length=0 -pthread -Werror=return-type
		-fno-eliminate-unused-debug-types -fno-inline -fmodules)
# Flags for GNU libstdc++
### @todo Detect that we're actually using GNU libstdc++
# "compiles user code using the debug mode. When defined, _GLIBCXX_ASSERTIONS is defined automatically"
### @note This crashes when linked with non-debug libstdc++.  Can't pass containers between them.
#ucm_add_flags(CXX -D_GLIBCXX_DEBUG=1)
endif(UNIX)
if(FALSE) ##CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ASAN_FLAGS}")
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	message(STATUS "C++ Compiler is GCC")
	ucm_add_flags(-fvar-tracking -fvar-tracking-assignments -gpubnames -ggnu-pubnames)
	# Max debug flags
	target_compile_options(cxx_compile_options
		INTERFACE
			-gvariable-location-views
			-gas-loc-support -gas-locview-support -gstatement-frontiers -ginline-points -femit-struct-debug-detailed=any
		)
	#ucm_add_flags(-gvariable-location-views)
	ucm_add_flags(-grecord-gcc-switches)
	ucm_add_flags(-fno-eliminate-unused-debug-types -fno-inline)
	ucm_add_flags(-Wmaybe-uninitialized -Werror=maybe-uninitialized -fdiagnostics-show-template-tree -fdiagnostics-show-option)
	ucm_add_flags(-fuse-ld=gold) # Let's try the gold linker.
	# GCC 9.x suggestions.
	add_library(cxx_suggestions INTERFACE)
	target_compile_options(cxx_suggestions INTERFACE
		-Wsuggest-attribute=const)
	target_link_libraries(cxx_settings INTERFACE cxx_suggestions)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	message(STATUS "C++ Compiler is Clang")
	target_compile_options(cxx_compile_options
		INTERFACE
			-fforce-emit-vtables
			)
	ucm_add_flags(-Wthread-safety -fdebug-macro -fdiagnostics-show-template-tree -mno-omit-leaf-frame-pointer -ftime-trace=build_time_trace.json)
	#ucm_add_flags(-fdelayed-template-parsing -frelaxed-template-template-args)
endif()
if(WIN32)
	#ucm_add_flags(/showIncludes)
	### NOTE: Make sure CMAKE_PREFIX_PATH has the path to the directory, so cmake can find win_flex and win_bison.
	### On Windows they're in a couple places, e.g.:
	# C:\Qt\5.12.0\Src\gnuwin32\bin\bison.exe
	# C:\CraftRoot\build\dev-utils\flexbison\work\win_bison.exe
	# C:\CraftRoot\build\dev-utils\flexbison\image-Debug-2.5.14\dev-utils\bin\win_bison.exe
	# We also need png2ico on Windows, which is here:
	# C:\CraftRoot\dev-utils\bin\png2ico.exe
	set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH};C:/CraftRoot/dev-utils/bin;C:/Qt/5.12.0/Src/gnuwin32/bin)
endif(WIN32)
target_link_libraries(cxx_settings INTERFACE cxx_compile_options)

#set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -ftime-trace")
message(STATUS "C compiler supported features = ${CMAKE_C_COMPILE_FEATURES}")
message(STATUS "C++ compiler supported features = ${CMAKE_CXX_COMPILE_FEATURES}")
message(STATUS "C++ CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
message(STATUS "C++ CMAKE_CXX_FLAGS_DEBUG = ${CMAKE_CXX_FLAGS_DEBUG}")

###



# Include some basic cmake functions.
include(CheckCXXCompilerFlag)
include(PrintTargetProperties)
include(helperfunctions)

# Include some basic Qt-related cmake functions
include(QtCommon)
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE) # Without this, Release builds won't include the MSVC C runtime.
include(InstallRequiredSystemLibraries)
# For DownloadProject, which we use for GoogleTest.
set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED FALSE")
#include(ExternalProject)
include(DownloadProject)

# Right now we need all the verbosity we can get.
set(CMAKE_VERBOSE_MAKEFILE  YES)
# "CMake automatically analyzes the global inter-target dependency graph at the beginning of native build system generation.
#  This property causes it to display details of its analysis to stderr."
# GRVS: This dumps huge amounts of info.
# set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)
set(CPACK_IFW_VERBOSE YES)
#set(CMAKE_DEBUG_TARGET_PROPERTIES
#	    INCLUDE_DIRECTORIES
#		COMPILE_DEFINITIONS
#		POSITION_INDEPENDENT_CODE
#		CONTAINER_SIZE_REQUIRED
#		LIB_VERSION
#		)



message("******************************************************************")
message("* ${PROJECT_NAME} version ${PROJECT_VERSION}-${GVI_VERSION_COMMITS}-g${GVI_VERSION_HASH}${GVI_VERSION_DIRTY_POSTFIX} (${CMAKE_BUILD_TYPE})")
message("* ${FULL_COPYRIGHT_STRING}")
message("******************************************************************")

message("* Using CMake version ${CMAKE_VERSION}")
message("* CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message("*")
message("* From top level:")
message("*   Source directory: ${PROJECT_SOURCE_DIR} (${AwesomeMediaLibraryManager_SOURCE_DIR})")
message("*   Top level of build tree: ${CMAKE_BINARY_DIR}")
message("*   Build directory: ${PROJECT_BINARY_DIR} (${AwesomeMediaLibraryManager_BINARY_DIR})")
message("*   Version: ${PROJECT_VERSION}")
message("*   Version (alt): ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}")
message("*   Discovered tools:")
message("*        CMAKE_MAKE_PROGRAM: ${CMAKE_MAKE_PROGRAM}")
message("*        GIT_EXECUTABLE: ${GIT_EXECUTABLE}")
# Per @link https://stackoverflow.com/a/24470998
message("* Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")

print_compilers_and_params()

# FBO at least visual studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Export compile commands.
# FBO Eclipse, clang-tidy, iwyu, etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The.  Simplest.  Things.
# Set default install location to "install" folder in build dir.
# We do not want to install to /usr by default.
# "CMake sets this variable to a TRUE value when the CMAKE_INSTALL_PREFIX has just been initialized to its default value,
# typically on the first run of CMake within a new build tree. This can be used by project code to change the default 
# without overriding a user-provided value"
### @todo This is still broken: prefix.sh gets generated (first time after a wipe?) with /usr/bin paths.
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(NEW_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")
	message(STATUS "Changing CMAKE_INSTALL_PREFIX from ${CMAKE_INSTALL_PREFIX} to ${NEW_INSTALL_PREFIX}")
	set (CMAKE_INSTALL_PREFIX "${NEW_INSTALL_PREFIX}" CACHE
		PATH "Install path prefix, prepended onto install directories." FORCE )
	message(STATUS "Set CMAKE_INSTALL_PREFIX to ${CMAKE_INSTALL_PREFIX}")
else()
	message(WARNING "CMAKE_INSTALL_PREFIX set to default ${CMAKE_INSTALL_PREFIX}")
endif()
print_varlist(CMAKE_INSTALL_PREFIX KDE_INSTALL_ICONDIR)


# Compiler flag support checks.
set(EXTRA_CXX_COMPILE_FLAGS "")
check_cxx_compiler_flag("-Wuninitialized" CXX_W_UNINIT)
if(CXX_W_UNINIT)
	message(STATUS "Compile flag supported: ${CXX_W_UNINIT}")
	list(APPEND EXTRA_CXX_COMPILE_FLAGS "-Wuninitialized")
endif()
if(WIN32)
    # Make sure we're supporting Unicode on Windows.
	### @todo With KDECompileFlags we don't need this.
    add_definitions(-DUNICODE -D_UNICODE)
endif()


# Find includes in corresponding build directories.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

###
### The Qt packages we require.
### @note CONFIG|MODULE: "If no module is found the command falls back to Config mode, described below.
###       This fall back is disabled if the MODULE option is given."
###
find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
	Core
	Gui
	Widgets
	Multimedia
#	XmlPatterns
	Concurrent
	Test # For Q...ModelTester.
	)
set(QT_LINK_LIB_TARGETS
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	Qt6::Multimedia
#	Qt6::XmlPatterns
	Qt6::Concurrent
	Qt6::Test
)

# Unix-only
if(UNIX)
### Nothing here yet.  Was QT5::X11Extras, which was removed by Qt6.
endif(UNIX)
# Windows only
if(WIN32)
	list(APPEND QT_LINK_LIB_TARGETS Qt6::WinMain)
endif(WIN32)

print_qtinfo()

###
### KDE Foundation libs.
###
# Krita has ~this warning: "don't reorder these, this is the order they depend on each other."
set(KF_LINK_LIB_TARGETS "")
### @note "Config" is both a find_package() keyword as well as the KF5Config components KF5::ConfigCore and KF5::ConfigGui.
###       We'll require KF5Config separately to avoid any ambiguity.
find_package(KF6Config ${KF_MIN_VERSION} REQUIRED)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
	Config
	Archive ### @todo TRYING THIS IN CASE IconThemes needs it to work properly, otherwise REMOVE
	WidgetsAddons
	# Completion
	CoreAddons
	GuiAddons
	ConfigWidgets ## Not in Krita from this until WindowSystem.
	KIO
	ItemModels
	ItemViews
	XmlGui
	IconThemes
	WindowSystem
)
set(HAVE_KF501 ${KF5_FOUND})
set(HAVE_KF6 ${KF6_FOUND})
list(APPEND KF_LINK_LIB_TARGETS KF6::CoreAddons KF6::ConfigCore KF6::ConfigGui)
list(APPEND KF_LINK_LIB_TARGETS KF6::KIOCore KF6::KIOFileWidgets KF6::KIOWidgets)
list(APPEND KF_LINK_LIB_TARGETS KF6::WidgetsAddons KF6::XmlGui KF6::GuiAddons KF6::ItemModels KF6::ItemViews)
list(APPEND KF_LINK_LIB_TARGETS KF6::IconThemes KF6::WindowSystem)
### @todo Without this the directory is kxmlgui5, which is wrong, and I haven't found the root cause.
set(KDE_INSTALL_KXMLGUIDIR "share/kxmlgui6")  # Force KF6 path

###### EXPERIMENTAL

# SQLite3
find_package(SQLite3
             REQUIRED)

#find_package(BreezeIcons REQUIRED) # Doesn't exist yet.
### @todo Need to find a better way than hardcoding the path to the rcc file here.
set(WITH_BUNDLED_BREEZE_ICONS false)
if(WIN32)
	set(BREEZEICONS_DIR
		# Craft has a couple of breeze-icons.rcc's, plus an icontheme.rcc in CraftRoot/bin/data/.
		C:/CraftRoot/bin/data/icons/breeze
	)
else()
	set(BREEZEICONS_DIR
		/usr/share/icons/breeze
		~/.icons/breeze
	)
endif()
if(WITH_BUNDLED_BREEZE_ICONS)
	### @todo SOMETHING IS WRONG HERE, FIX THIS 
	set(AMLM_INSTALL_BUNDLED_ICON_THEME_DIR "${KDE_INSTALL_FULL_DATAROOTDIR}/gvansickle/AwesomeMediaLibraryManager")
	find_file(BREEZEICONS_PATH breeze-icons.rcc
		HINTS
			${BREEZEICONS_DIR}
		PATH_SUFFIXES
			breeze
		DOC
			"Full path to the breeze-icons.rcc file"
			)
    if (NOT EXISTS ${BREEZEICONS_PATH})
        message(FATAL_ERROR "Cannot find breeze-icons.rcc, looked under paths: ${BREEZEICONS_DIR}")
    else()
    	message(STATUS "Found source default icon theme: ${BREEZEICONS_PATH}")
    	message(STATUS "Default icon theme will be installed in: ${AMLM_INSTALL_BUNDLED_ICON_THEME_DIR}")
    	# Install it as our default icon theme.
    	install(FILES ${BREEZEICONS_PATH}
			DESTINATION ${AMLM_INSTALL_BUNDLED_ICON_THEME_DIR}
			RENAME "icontheme.rcc"
			COMPONENT coreapp)
    endif()
endif()
#list(APPEND KF_LINK_LIB_TARGETS Breeze)
print_varlist(BREEZEICONS_PATH)


###
### GTK Libs.
### See https://wiki.gnome.org/Projects/gtkmm/UsingCMake, https://cmake.org/cmake/help/v3.10/module/FindPkgConfig.html
###
set(GTK_LINK_LIB_TARGETS "")
if(UNIX)
# 	pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-3.0)
# 	list(APPEND GTK_LINK_LIB_TARGETS PkgConfig::GTKMM)
# 	set(HAVE_GTKMM01 ${GTKMM_FOUND})
endif(UNIX)

###
### X11 and XCB libs.
###
if(UNIX)
find_package(X11 REQUIRED)
list(APPEND GTK_LINK_LIB_TARGETS ${X11_X11_LIB})

find_package(X11_XCB REQUIRED)
list(APPEND GTK_LINK_LIB_TARGETS ${X11_XCB_LIBRARIES})
cmake_print_variables(X11_XCB_FOUND)

find_package(XCB REQUIRED COMPONENTS
	XCB
	)
list(APPEND GTK_LINK_LIB_TARGETS ${XCB_TARGETS})
print_varlist(XCB_TARGETS GTK_LINK_LIB_TARGETS)
endif(UNIX)


# Qt logging categories.
# @link https://api.kde.org/ecm/module/ECMQtDeclareLoggingCategory.html
ecm_qt_declare_logging_category(QT_LOGGING_CATEGORY_SOURCE_FILES
                                HEADER logging_cat_ExtFuture.h
                                IDENTIFIER EXTFUTURE
                                CATEGORY_NAME amlm.concurrency.extfuture
                                DEFAULT_SEVERITY Debug ### <Debug|Info|Warning|Critical|Fatal>
                                )
ecm_qt_declare_logging_category(QT_LOGGING_CATEGORY_SOURCE_FILES
                                HEADER logging_cat_ExtAsync.h
                                IDENTIFIER EXTASYNC
                                CATEGORY_NAME amlm.concurrency.extasync
                                DEFAULT_SEVERITY Debug ### <Debug|Info|Warning|Critical|Fatal>
                                )

# Qt-specific compiler definitions.                               
target_compile_definitions(cxx_definitions_qt
	INTERFACE
		-DQT_DEPRECATED_WARNINGS
		-DQT_SHAREDPOINTER_TRACK_POINTERS=1
    	-DQT_NO_KEYWORDS
    	-DQT_NO_SIGNALS_SLOTS_KEYWORDS
	    #-DQT_NO_URL_CAST_FROM_STRING
	    #-DQT_NO_CAST_FROM_ASCII
	    #-DQT_NO_CAST_TO_ASCII
	    #-DQT_NO_CAST_FROM_BYTEARRAY
	    -DQT_STRICT_ITERATORS
	    -DQT_USE_QSTRINGBUILDER
	    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
	   )


# Set PROJECT_VERSION_PATCH and PROJECT_VERSION_TWEAK to 0 if not present, needed by add_project_meta
fix_project_version()

# Set additional project information FBO QtCommon.cmake/add_project_meta().
# Resource directory FBO QtCommon/add_project_meta().
set(RESOURCE_FOLDER resources)
# The basename of the application's icon.
set(ICON_NAME "AMLMAppIcon")
# For the "CompanyName" field in the rc file.
set(RC_COMPANYNAME "${PROJECT_AUTHOR}")
set(RC_LEGALCOPYRIGHT "${FULL_COPYRIGHT_STRING}")
# Reverse of app.setOrganizationDomain()?
set(IDENTIFIER "io.github.gvansickle")

### Generate configured headers.
# @note The version info file is generated in the "resources" dir/target.
# Generate the config.h file.
configure_file(${PROJECT_SOURCE_DIR}/src/config.h.in
	${PROJECT_BINARY_DIR}/src/config.h
	ESCAPE_QUOTES)

# Create a graphviz dep graph.
# add_custom_target(graphviz ALL
# 	${CMAKE_COMMAND} .. "--graphviz=graphviz.dot" .
# 	WORKING_DIRECTORY .)
find_package(Doxygen REQUIRED)
if(DOXYGEN_FOUND)
	# Set directories.
	set(DOXYGEN_INPUT "${CMAKE_SOURCE_DIR}/src")
	set(DOXYGEN_OUTPUT "${CMAKE_BINARY_DIR}/docs")
	# Configure Doxyfile.
	configure_file(
		${CMAKE_SOURCE_DIR}/Doxyfile.in
		${CMAKE_BINARY_DIR}/Doxyfile
		@ONLY
	)
	# Custom target to generate doxygen docs.
	add_custom_target(docs
		COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMENT "Generating internals documentation with Doxygen"
		VERBATIM
	)
else()
	message(WARNING "Doxygen not found.  Documentation will not be generated.")
endif()



###
### Add the project subdirs.
###
#set(amlm_APP_ICONS "re-set this in resources_subdir")
add_subdirectory(resources EXCLUDE_FROM_ALL)
message(AUTHOR_WARNING "amlm_APP_ICONS: ${amlm_APP_ICONS}")
add_subdirectory(src)

###
### Testing
###
include(CTest)
### This var comes from CTest, and is True by default.
if(BUILD_TESTING)
	add_subdirectory(tests)
endif()

#include(resources/CMakeLists.txt)
#print_varlist(amlm_ICONS amlm_SRCS)
#ecm_add_app_icon(amlm_SRCS ICONS ${amlm_ICONS})


# This function sets up the variable META_FILES_TO_INCLUDE to the extra files
# needed for a windows build (essentially just the configured .rc file).
add_project_meta(META_FILES_TO_INCLUDE)

###
### Install
###
# In subdirs.


###
### Packaging
###
if(false) # This has to be broken now.
# From within the build directory, just run the following commands to create the package:
# $ cmake ..
# $ make package

# Custom target for coordinating the packaging.
add_custom_target(packaging
	ALL
	DEPENDS ${PROJECT_NAME} icons_oxygen ${AppIcon_rcc}
	COMMENT "Packaging ${PROJECT_NAME}...")
if(WIN32)
	# Add custom commands and install targets for Qt windeploy.
	include(package/PackageWindows.cmake)
endif()

# Relocate this?
set(CPACK_GENERATOR NSIS)

# Generic.
# https://cmake.org/cmake/help/v3.10/module/CPack.html
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION ${GVI_GIT_DESCRIBE_OUTPUT}-${CMAKE_BUILD_TYPE})
# Doesn't look like we need to set these separately.
#set(CPACK_PACKAGE_VERSION_MAJOR ???)
#set(CPACK_PACKAGE_VERSION_MINOR ???)
#set(CPACK_PACKAGE_VERSION_PATCH ???)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_NAME} - ${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_CONTACT "${PROJECT_AUTHOR} <${PROJECT_AUTHOR_EMAIL}>")
set(CPACK_PACKAGE_VENDOR "${COMPANY}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)

set(CPACK_PACKAGE_EXECUTABLES "${PROJECT_NAME};${PROJECT_NAME}")

# Packaging: NSIS
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_EXECUTABLES_DIRECTORY .)

set(CPACK_NSIS_PACKAGE_ARCHITECTURE "64")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")

set(CPACK_NSIS_PACKAGE_NAME ${CPACK_PACKAGE_NAME})
# Add a "Finished, Run the app?" page to the installer.
set(CPACK_NSIS_MUI_FINISHPAGE_RUN ${PROJECT_NAME})

set(CPACK_NSIS_MENU_LINKS "https://github.com/gvansickle/AwesomeMediaLibraryManager" "AwesomeMediaLibraryManager at GitHub")

include(CPack)
if(WIN32)
	include(CpackNSIS)
endif()
include(CPackIFW REQUIRED)

# Add components to be packaged by CPack.
cpack_add_component(
		coreapp
		DISPLAY_NAME "${PROJECT_NAME} application"
		DESCRIPTION "Install ${PROJECT_NAME} application files"
		REQUIRED)

#include(CPackHelper)

#### Show package configuration files
#add_custom_target(package_files
#    SOURCES package/qtifw/create_start_menu_shortcuts.qs
#    COMMENT "Dummy target to show package configuration files in Qt Creator interface")

endif()### END INSTALLER STUFF

# Generate a dependency graph.
# @see CMakeGraphVizOptions.cmake
if(FALSE)
add_custom_target(graphviz ALL
	COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/graphviz"
	COMMAND "${CMAKE_COMMAND}" "--graphviz=graphviz/amlm.dot" .
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()





###
### Print summary.
###
message(STATUS "======================== START CMAKE SUMMARY ====================================")
message(STATUS "Variables:")
print_varlist(
	PROJECT_NAME
	CMAKE_BINARY_DIR
	PROJECT_BINARY_DIR
	CMAKE_LIBRARY_PATH
	CMAKE_PREFIX_PATH
	CMAKE_CXX_COMPILER
	CMAKE_CXX_FLAGS
	CMAKE_CXX_FLAGS_DEBUG
	CMAKE_EXPORT_COMPILE_COMMANDS
	CMAKE_INSTALL_PREFIX
	KDE_INSTALL_BINDIR
	KDE_INSTALL_DATADIR
	KDE_INSTALL_ICONDIR
	KDE_INSTALL_TARGETS_DEFAULT_ARGS
	KDE_INSTALL_KXMLGUIDIR
	KDE_INSTALL_KCFGDIR
	META_FILES_TO_INCLUDE
)

#message_cpack_summary()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

message(STATUS "======================== END CMAKE SUMMARY ====================================")
