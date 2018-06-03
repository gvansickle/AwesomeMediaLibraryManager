#
# Copyright 2017, 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

# References:
# https://github.com/euler0/mini-cmake-qt (QtCommon et al, Public Domain, https://github.com/euler0/mini-cmake-qt/blob/master/LICENSE)
# https://cmake.org/cmake/help/v3.0/manual/cmake-qt.7.html
# Installer refs:
# https://github.com/skypjack/qt_cpackifw
# https://cmake.org/cmake/help/v3.9/module/CPackIFW.html
# https://github.com/kracejic/cleanCppProject/blob/master/CMakeLists.txt

# @note For QtIFW installer:
# - On Linux, CPACK_IFW_ROOT (cmake var) or QTIFWDIR (env var) should point to something like ~/Qt/QtIFW-3.0.1

# cmake setup.
cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
cmake_policy(SET CMP0053 NEW) # Use new var ref/esc seq eval
cmake_policy(SET CMP0010 NEW) # Silence warning on CMP0010.

message(STATUS "CMake-time PATH: $ENV{PATH}")
include(cmake/GetVersionInfo.cmake)

# Basic information about project
# Note that here is where CMake determines compilers and loads any toolchain.cmake file.
# (per https://cgold.readthedocs.io/en/latest/tutorials/project.html)
project(AwesomeMediaLibraryManager
        VERSION ${GVI_VERSION_TAG_QUAD}
        DESCRIPTION "The Awesome Media Library Manager"
        LANGUAGES C CXX)

# Prevent an in-root-of-src-tree build.
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
        message("ERROR: ${CMAKE_SOURCE_DIR} == ${CMAKE_BINARY_DIR}")
	message(FATAL_ERROR "Prevented in-root-of-tree build. Please create a build directory outside of the source code root directory and call cmake from there. Thank you.")
endif()

### Set the required language standards.
# C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Min versions of Qt5, KF5, KF5/ECM.
set(QT_MIN_VERSION "5.10.0")
set(ECM_MIN_VERSION "5.44.0")
set(KF5_MIN_VERSION "5.44.0")

###
### ECM Setup ====================================================================================================
###

# KDE extra-cmake-modules, see https://api.kde.org/ecm/
# @note find_package() MODULE vs. NO_MODULE (==CONFIG mode):
# "Config mode provides an elaborate interface and search procedure. Much of the interface is provided for completeness
#  and for use internally by find-modules loaded by Module mode. Most user code should simply call:
#    find_package(<package> [major[.minor]] [EXACT] [REQUIRED|QUIET])"
find_package(ECM ${ECM_MIN_VERSION} REQUIRED NO_MODULE)

# Add dirs where we keep support *.cmake's, ECM, etc.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

# ECM Macros
include(ECMAddAppIcon)
# Sanitizers.
# @note KF5:
# The sanitizers are not enabled by default. Instead, you must set ECM_ENABLE_SANITIZERS (either in your
# CMakeLists.txt or on the command line) to a semicolon-separated list of sanitizers you wish to enable.
# The options are:
#   address memory thread leak undefined
# The sanitizers “address”, “memory” and “thread” are mutually exclusive. You cannot enable two of them in the same build.
# "leak" requires the "address" sanitizer.
### @note KDECompileFlags includes include(ECMEnableSanitizers).
include(ECMEnableSanitizers)

# KDE macros.
# @see https://api.kde.org/ecm/manual/ecm-kde-modules.7.html
set(KDE_INSTALL_DIRS_NO_DEPRECATED TRUE) ##< Don't define the deprecated KDEInstallDirs.
set(ENABLE_CLAZY ON)
set(APPLE_FORCE_X11 ON)
include(KDEInstallDirs)  ##< Replaces/uses GNUInstallDirs.
include(KDECMakeSettings)
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

# Set some extra debug flags early.
set(AMLM_MAX_DEBUG_FLAGS "-Og -ggdb3 -fno-omit-frame-pointer -fvar-tracking -fvar-tracking-assignments")
# if(TRUE)#CLANG)
# 	string(APPEND ${AMLM_MAX_DEBUG_FLAGS} -Wthread-safety)
# endif()
set(CMAKE_CXX_FLAGS_DEBUG ${AMLM_MAX_DEBUG_FLAGS})
set(CMAKE_C_FLAGS_DEBUG ${AMLM_MAX_DEBUG_FLAGS})


###
### END ECM Setup ====================================================================================================
###

# Std CMAKE includes.
include(FeatureSummary)

message(STATUS "C compiler supported features = ${CMAKE_C_COMPILE_FEATURES}")
message(STATUS "C++ compiler supported features = ${CMAKE_CXX_COMPILE_FEATURES}")
message(STATUS "C++ CMAKE_CXX_FLAGS_DEBUG = ${CMAKE_CXX_FLAGS_DEBUG}")

###

if(FALSE)
	# Use ccache if it's available.
	find_program(CCACHE_PROGRAM ccache)
	if(CCACHE_PROGRAM)
		set_property(GLOBAL PROPERTY CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
	endif()
endif()

# Include some basic cmake functions.
include(CheckCXXCompilerFlag)
include(CMakePrintHelpers)
include(PrintTargetProperties)
include(helperfunctions)
find_package(PkgConfig REQUIRED)
# Include some basic Qt5-related cmake functions
include(QtCommon)
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE) # Without this, Release builds won't include the MSVC C runtime.
include(InstallRequiredSystemLibraries)
# For GoogleTest and Boost.
set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED FALSE")
include(ExternalProject)
include(DownloadProject)

# Right now we need all the verbosity we can get.
set(CMAKE_VERBOSE_MAKEFILE  YES)
#set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)
set(CPACK_IFW_VERBOSE YES)
#set(CMAKE_DEBUG_TARGET_PROPERTIES
#		INCLUDE_DIRECTORIES
#		COMPILE_DEFINITIONS
#		POSITION_INDEPENDENT_CODE
#		CONTAINER_SIZE_REQUIRED
#		LIB_VERSION
#		)

dir_summary()

# QtCommon/init_os_bundle: Set OS_BUNDLE to WIN32, MACOSX_BUNDLE, or empty, depending on platform.
init_os_bundle()

set(PROJECT_AUTHOR "Gary R. Van Sickle")
set(PROJECT_AUTHOR_EMAIL "grvs@users.sourceforge.net")

set(FULL_COPYRIGHT_STRING "Copyright (c) 2017 ${PROJECT_AUTHOR} <${PROJECT_AUTHOR_EMAIL}>")

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

print_compilers_and_params()

# FBO at least visual studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Export compile commands.
# FBO Eclipse, clang-tidy, iwyu, etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default install location to dist folder in build dir
# we do not want to install to /usr by default
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/dist" CACHE PATH
        "Install path prefix, prepended onto install directories." FORCE )
endif()



# Compiler flag support checks.
set(EXTRA_CXX_COMPILE_FLAGS "")
if(FALSE)
check_cxx_compiler_flag("-Wdocumentation" CXX_W_DOCUMENTATION)
if(CXX_W_DOCUMENTATION)
	message(STATUS "Compile flag supported: ${CXX_W_DOCUMENTATION}")
	list(APPEND EXTRA_CXX_COMPILE_FLAGS "-Wdocumentation")
endif()
endif()
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
### The Qt5 packages we require.
###
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
	Core
	Gui
	Widgets
	Multimedia
	Concurrent
	X11Extras
	)
set(QT5_LINK_LIB_TARGETS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Multimedia Qt5::Concurrent Qt5::X11Extras)
get_target_property(Qt5Core_location Qt5::Core LOCATION)
message(STATUS "* Found Qt version ${Qt5_VERSION} at ${Qt5Core_location}")
message(STATUS "*   Qt5_VERSION: ${Qt5_VERSION}")
message(STATUS "*   Qt5_DIR: ${Qt5_DIR}")
message(STATUS "*   Qt5Core_FOUND: \"${Qt5Core_FOUND}\"")
message(STATUS "*   Qt5Core_EXECUTABLE_COMPILE_FLAGS: \"${Qt5Core_EXECUTABLE_COMPILE_FLAGS}\"")
# For access to QPA headers.
message(STATUS "*   Qt5Core_PRIVATE_INCLUDE_DIRS: ${Qt5Core_PRIVATE_INCLUDE_DIRS}")
message(STATUS "*   Qt5Gui_PRIVATE_INCLUDE_DIRS: ${Qt5Gui_PRIVATE_INCLUDE_DIRS}")
if(FALSE)
	# List plugin info.
	message(STATUS "*  Qt5::Gui Plugins:")
	foreach(plugin ${Qt5Gui_PLUGINS})
	  get_target_property(_loc ${plugin} LOCATION)
	  message(STATUS "*    Plugin ${plugin} is at location ${_loc}")
	endforeach()
	message(STATUS "*  Qt5::Widgets Plugins:")
	foreach(plugin ${Qt5Widgets_PLUGINS})
	  get_target_property(_loc ${plugin} LOCATION)
	  message(STATUS "*    Plugin ${plugin} is at location ${_loc}")
	endforeach()
endif()

###
### KDE KF5 libs.
###
set(KF5_LINK_LIBS)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
	CoreAddons ## USE_KF5_COREADDONS, git clone git://anongit.kde.org/kcoreaddons.git
	Config
	ConfigWidgets
	KIO ## USE_KDE_KIO, git clone git://anongit.kde.org/kio.git
	WidgetsAddons
	XmlGui
	GuiAddons
	IconThemes
	WindowSystem
	ThreadWeaver
)
set(HAVE_KF501 ${KF5_FOUND})
list(APPEND KF5_LINK_LIBS KF5::CoreAddons KF5::ConfigCore KF5::ConfigGui)
list(APPEND KF5_LINK_LIBS KF5::KIOCore KF5::KIOFileWidgets KF5::KIOWidgets KF5::KIONTLM)
list(APPEND KF5_LINK_LIBS KF5::WidgetsAddons KF5::XmlGui KF5::GuiAddons KF5::IconThemes)
list(APPEND KF5_LINK_LIBS KF5::WindowSystem KF5::ThreadWeaver)


###
### GTK Libs.
### See https://wiki.gnome.org/Projects/gtkmm/UsingCMake, https://cmake.org/cmake/help/v3.10/module/FindPkgConfig.html
###
set(GTK_LINK_LIB_TARGETS)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-3.0)
list(APPEND GTK_LINK_LIB_TARGETS PkgConfig::GTKMM)
set(HAVE_GTKMM01 ${GTKMM_FOUND})

###
### X11 and XCB libs.
###
if(TRUE)

find_package(X11 REQUIRED)
list(APPEND GTK_LINK_LIB_TARGETS ${X11_X11_LIB})

find_package(X11_XCB REQUIRED)
list(APPEND GTK_LINK_LIB_TARGETS ${X11_XCB_LIBRARIES})
cmake_print_variables(X11_XCB_FOUND)

find_package(XCB REQUIRED COMPONENTS
	XCB
	)
list(APPEND GTK_LINK_LIB_TARGETS ${XCB_TARGETS})
cmake_print_variables(XCB_TARGETS)

cmake_print_variables(GTK_LINK_LIB_TARGETS)

endif()

#
# And Boost.  Sorry.  But just CallableTraits, has no other deps.
#
# See https://cmake.org/cmake/help/v3.10/module/FindBoost.html
### CallableTraits only in 1.66+.
#set(Boost_USE_MULTITHREADED      ON)
#find_package(Boost 1.66.0 REQUIRED
#  COMPONENTS CallableTraits
#  )
download_project(PROJ                boost_callable_traits
	GIT_REPOSITORY https://github.com/boostorg/callable_traits.git
	GIT_TAG        master
	${UPDATE_DISCONNECTED_IF_AVAILABLE}
	)
add_subdirectory(${boost_callable_traits_SOURCE_DIR} ${boost_callable_traits_BINARY_DIR})
#message(STATUS ${boost_callable_traits_SOURCE_DIR} ${boost_callable_traits_BINARY_DIR})

# Use include-what-you-use.
if(FALSE)
	if(NOT WIN32)
		set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "iwyu;-Xiwyu;--mapping_file=/usr/share/include-what-you-use/qt5_4.imp;-Xiwyu;--mapping_file=/usr/share/include-what-you-use/gcc.stl.headers.imp;-Xiwyu;--mapping_file=${PROJECT_SOURCE_DIR}/utils/nomoc/nomoc.imp")
	endif()
endif()

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

# So the source in src can find the "resources/" dir.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${Qt5Core_PRIVATE_INCLUDE_DIRS}
	${Qt5Gui_PRIVATE_INCLUDE_DIRS})

### Generate configured headers.
# Generate the version info file.
configure_file(${PROJECT_SOURCE_DIR}/resources/VersionInfo.cpp.in
		${PROJECT_BINARY_DIR}/resources/VersionInfo.cpp)
# Generate the amlm-configure.h file.
configure_file(${PROJECT_SOURCE_DIR}/src/config.h.in
	${PROJECT_BINARY_DIR}/src/config.h
	ESCAPE_QUOTES)

add_subdirectory(src)


# This function sets up the variable META_FILES_TO_INCLUDE to the extra files
# needed for a windows build (essentially just the configured .rc file).
add_project_meta(META_FILES_TO_INCLUDE)
#cmake_print_variables(META_FILES_TO_INCLUDE)


###
### Packaging
###
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

### END INSTALLER STUFF


###
### Print summary.
###
message(STATUS "======================== START CMAKE SUMMARY ====================================")
message(STATUS "PROJECT_NAME: ${PROJECT_NAME}")
message(STATUS "PROJECT_NAME INCLUDE_DIRECTORIES: ${PNID}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")
message(STATUS "CMAKE_LIBRARY_PATH: ${CMAKE_LIBRARY_PATH}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
cmake_print_variables(CMAKE_CXX_COMPILER)
cmake_print_variables(META_FILES_TO_INCLUDE)

message_cpack_summary()
message(STATUS "======================== END CMAKE SUMMARY ====================================")

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
