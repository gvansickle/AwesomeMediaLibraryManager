#
# Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

# Basic information about project
# Note that here is where CMake determines compilers and loads any toolchain.cmake file.
# (per https://cgold.readthedocs.io/en/latest/tutorials/project.html)
project(AwesomeMediaLibraryManager
        VERSION 0.0.1.0
        DESCRIPTION "The Awesome Media Library Manager"
        LANGUAGES C CXX)

# Add folder where we keep support *.cmake's.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Include some basic cmake functions.
include(CMakePrintHelpers)
include(PrintTargetProperties)
include(helperfunctions)
# Include some basic Qt5-related cmake functions
include(QtCommon)

# QtCommon/init_os_bundle: Set OS_BUNDLE to WIN32, MACOSX_BUNDLE, or empty, depending on platform.
init_os_bundle()

set(PROJECT_AUTHOR "Gary R. Van Sickle")
set(PROJECT_AUTHOR_EMAIL "grvs@users.sourceforge.net")

set(FULL_COPYRIGHT_STRING "Copyright (c) 2017 ${PROJECT_AUTHOR} <${PROJECT_AUTHOR_EMAIL}>")

message("******************************************************************")
message("* ${PROJECT_NAME} version ${PROJECT_VERSION} (${CMAKE_BUILD_TYPE})")
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

print_compilers_and_params()

# Be nice to visual studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Be nice and export compile commands by default, this is handy for clang-tidy
# and for other tools.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prevent an in-root-of-src-tree build.
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "Prevented in-tree build. Please create a build directory outside of the source code and call cmake from there. Thank you.")
endif()

# Set default install location to dist folder in build dir
# we do not want to install to /usr by default
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/dist" CACHE PATH
        "Install path prefix, prepended onto install directories." FORCE )
endif()

# C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find includes in corresponding build directories.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

### @todo do we need this?
#set(CMAKE_AUTORCC_OPTIONS "--temp")

# The Qt5 packages we require.
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Multimedia Concurrent)
get_target_property(Qt5Core_location Qt5::Core LOCATION)
message(STATUS "Found Qt ${Qt5Core_VERSION_STRING} at ${Qt5Core_location}")

# Right now we need all the verbosity we can get.
set(CMAKE_VERBOSE_MAKEFILE  YES)
set(CPACK_IFW_VERBOSE YES)

# Set PROJECT_VERSION_PATCH and PROJECT_VERSION_TWEAK to 0 if not present, needed by add_project_meta
fix_project_version()

# Set additional project information FBO QtCommon.cmake/add_project_meta().
# Resource directory FBO QtCommon/add_project_meta().
set(RESOURCE_FOLDER resources)
# The basename of the application's icon.
set(ICON_NAME "AppIcon")
# For the "CompanyName" field in the rc file.
set(RC_COMPANYNAME "${PROJECT_AUTHOR}")
set(RC_LEGALCOPYRIGHT "${FULL_COPYRIGHT_STRING}")
# Reverse of app.setOrganizationDomain()?
set(IDENTIFIER "io.github.gvansickle")

###
### The third-party packages we require.
###

set(AMLM_LIBCUE_TARGET "")
set(AMLM_TAGLIB_TARGET "")

if(NOT WIN32)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(LIBCUE REQUIRED libcue
			IMPORTED_TARGET)
	if(LIBCUE_FOUND)
		set(AMLM_LIBCUE_TARGET "PkgConfig::LIBCUE")
	endif()

    pkg_check_modules(TAGLIB REQUIRED taglib>=1.6
			IMPORTED_TARGET)
	if(TAGLIB_FOUND)
		set(AMLM_TAGLIB_TARGET "PkgConfig::TAGLIB")
	endif()
else()
    set(CMAKE_VERBOSE_MAKEFILE ON)

    # Use the in-tree copy of libcue.
    add_subdirectory("third_party"
			EXCLUDE_FROM_ALL  # Don't tie this lib directly to the ALL target.
			)

    set(AMLM_LIBCUE_TARGET "libcue")

    print_target_properties(${AMLM_LIBCUE_TARGET})

    # TagLib.
	add_subdirectory("third_party/taglib"
			EXCLUDE_FROM_ALL # Don't tie this lib directly to the ALL target.
			)
		# @note There's currently a bug in taglib's taglib/taglib/CmakeLists.txt:
		# include_directories(
		#   ${CMAKE_CURRENT_SOURCE_DIR}/toolkit
		#   [...]
		#   ${CMAKE_SOURCE_DIR}/3rdparty
		#
		# The last line point to our top-level directory, not taglib's.
		# We work around this here.
		target_include_directories(tag PRIVATE "third_party/taglib/3rdparty")


	set(AMLM_TAGLIB_TARGET "tag")

	print_target_properties(${AMLM_TAGLIB_TARGET})

	#    add_definitions(-DTAGLIB_STATIC)
#    set(TAGLIB_PATH_PREFIX "${PROJECT_SOURCE_DIR}/../taglib-1.11.1/taglib")
#    set(TAGLIB_LIB_PATH "${TAGLIB_PATH_PREFIX}/Debug")
#    message(STATUS "${TAGLIB_LIB_PATH}")
#    # Include dir.
#    find_path(TAGLIB_INCLUDE_DIR
#            NAMES taglib/taglib.h
#            PATHS ${TAGLIB_PATH_PREFIX}-install/include)
#    # The library itself.
#    find_library(TAGLIB_LIBRARY
#            NAMES tag.lib
#            PATHS ${TAGLIB_LIB_PATH})
#    # handle the QUIETLY and REQUIRED arguments and set TAGLIB_FOUND to TRUE
#    # if all listed variables are TRUE
#    include(FindPackageHandleStandardArgs)
#    find_package_handle_standard_args(TagLib DEFAULT_MSG
#            TAGLIB_LIBRARY TAGLIB_INCLUDE_DIR)
#    set(TAGLIB_LIBRARIES ${TAGLIB_LIBRARY})
#    set(TAGLIB_INCLUDE_DIRS ${TAGLIB_INCLUDE_DIR})
#    message(STATUS "TagLib found?: ${TAGLIB_FOUND}")
#
#    if(NOT TAGLIB_LIBRARY)
#        message(FATAL_ERROR "TagLib not found.")
#    endif()
endif()

# Set up definitions and paths
add_definitions(${QT_DEFINITIONS})
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
endif()
cmake_print_variables(QT_DEFINITIONS)

link_directories(${TAGLIB_LIBRARY_DIRS})
include_directories(${TAGLIB_INCLUDE_DIRS})

set(HEADER_FILES
	gui/ActivityProgressIndicator.h
	gui/CollectionDockWidget.h
	gui/Experimental.h
	gui/FilterWidget.h
	gui/ItemDelegateLength.h
	gui/MainWindow.h
	gui/MDIArea.h
	gui/MDILibraryView.h
	gui/MDIPlaylistView.h
	gui/MDITreeViewBase.h
	gui/MetadataDockWidget.h
	gui/NetworkAwareFileDialog.h
	gui/PixmapLabel.h
	gui/PlayerControls.h
	logic/Library.h
	logic/LibraryEntry.h
	logic/LibraryEntryMimeData.h
	logic/LibraryModel.h
	logic/LibrarySortFilterProxyModel.h
	logic/Metadata.h
	logic/MetadataAbstractBase.h
	logic/MetadataFromCache.h
	logic/MetadataTaglib.h
	logic/MP2.h
	logic/ntp.h
	logic/PlaylistModel.h
	logic/PlaylistModelItem.h
	logic/PlaylistSortFilterProxyModel.h
	logic/TrackMetadata.h
	utils/ActionHelpers.h
	utils/asyncfuture.h
	utils/ConnectHelpers.h
	utils/DebugHelpers.h
	utils/Fraction.h
	utils/in.h
	utils/MapConverter.h
        utils/StringHelpers.h
	utils/Theme.h
	utils/WorkerThreadBase.h
	logic/LibraryRescanner.h
	gui/ActivityProgressWidget.h
        utils/concurrency/ReportingRunner.h
        utils/AsyncDirScanner.h
	)

# Source file list.
set(SOURCE_FILES
        gui/CollectionDockWidget.cpp
	gui/Experimental.cpp
	gui/FilterWidget.cpp
	gui/ItemDelegateLength.cpp
	gui/MainWindow.cpp
	gui/MDIArea.cpp
	gui/MDILibraryView.cpp
	gui/MDIPlaylistView.cpp
	gui/MDITreeViewBase.cpp
	gui/MetadataDockWidget.cpp
	gui/NetworkAwareFileDialog.cpp
	gui/PixmapLabel.cpp
	gui/PlayerControls.cpp
	logic/Library.cpp
	logic/LibraryEntry.cpp
	logic/LibraryEntryMimeData.cpp
	logic/LibraryModel.cpp
        logic/LibrarySortFilterProxyModel.cpp
	logic/Metadata.cpp
	logic/MetadataAbstractBase.cpp
	logic/MetadataFromCache.cpp
	logic/MetadataTaglib.cpp
	logic/MP2.cpp
	logic/ntp.cpp
	logic/PlaylistModel.cpp
	logic/PlaylistModelItem.cpp
	logic/PlaylistSortFilterProxyModel.cpp
	logic/TrackMetadata.cpp
	utils/Fraction.cpp
	utils/MapConverter.cpp
	utils/Theme.cpp
	utils/WorkerThreadBase.cpp
	main.cpp
	logic/LibraryRescanner.cpp
	gui/ActivityProgressWidget.cpp
        utils/concurrency/ReportingRunner.cpp
        utils/AsyncDirScanner.cpp
        )

# This function sets up the variable META_FILES_TO_INCLUDE to the extra files
# needed for a windows build (essentially just the configured .rc file).
add_project_meta(META_FILES_TO_INCLUDE)
cmake_print_variables(META_FILES_TO_INCLUDE)

# Qt5 Resource (qrc) files.
# RESOURCE_FILES is a list of the generated output *.cpp file names.
qt5_add_resources(RESOURCE_FILES
        resources/oxygenicons.qrc
        resources/TangoIcons.qrc
        resources/AppIcon.qrc
        )
foreach(generated_rcc_file_name ${RESOURCE_FILES})
    message(STATUS "generated_rcc_file_name: ${generated_rcc_file_name}")
    set_property(SOURCE ${generated_rcc_file_name} PROPERTY SKIP_AUTOMOC ON)
endforeach()


###
### The main executable.
###
add_executable(${PROJECT_NAME} ${OS_BUNDLE} # Expands to WIN32 or MACOS_BUNDLE depending on OS
        ${SOURCE_FILES} ${META_FILES_TO_INCLUDE} ${RESOURCE_FILES}
        )
# Only the Qt stuff needs AUTOMOC.
set_property(TARGET ${PROJECT_NAME} PROPERTY AUTOMOC ON)
# The libs.
target_link_libraries(${PROJECT_NAME}
        Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Multimedia Qt5::Concurrent
        ${AMLM_TAGLIB_TARGET} ${AMLM_LIBCUE_TARGET})

# Stop clang-tidy from scanning generated files.
file(WRITE "${PROJECT_BINARY_DIR}/.clang-tidy" "
---
Checks: '-*,llvm-twine-local'
...
")

print_target_properties(${PROJECT_NAME})

### Installation.
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION . # Default is /usr/local/${PROJECT_NAME} on UNIX and c:/Program Files/${PROJECT_NAME}/${PROJECT_NAME} on Windows.
		COMPONENT coreapp
	)

# Custom target for coordinating the packaging.
add_custom_target(packaging
    ALL
    DEPENDS ${PROJECT_NAME}
    COMMENT "Packaging ${PROJECT_NAME}...")
if(WIN32)
    # Add custom commands and install targets for Qt windeploy.
    include(package/PackageWindows.cmake)
endif()

### Packaging
# From within the build directory, just run the following commands to create the package:
# $ cmake ..
# $ make package

# Relocate this?
set(CPACK_GENERATOR NSIS)

# Generic.
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "1")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_NAME} - ${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_CONTACT "${PROJECT_AUTHOR} <${PROJECT_AUTHOR_EMAIL}>")
set(CPACK_PACKAGE_VENDOR "${COMPANY}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)

set(CPACK_PACKAGE_EXECUTABLES "${PROJECT_NAME};${PROJECT_NAME}")

#set(CPACK_PACKAGE_VERSION "1.0.0")

# Packaging: NSIS
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_EXECUTABLES_DIRECTORY .)
set(CPACK_NSIS_MENU_LINKS "https://github.com/gvansickle/AwesomeMediaLibraryManager" "AwesomeMediaLibraryManager at GitHub")

set(CPACK_NSIS_PACKAGE_ARCHITECTURE "64")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")

set(CPACK_NSIS_PACKAGE_NAME ${CPACK_PACKAGE_NAME})
# Add a "Finished, Run the app?" page to the installer.
set(CPACK_NSIS_MUI_FINISHPAGE_RUN ${PROJECT_NAME})

include(CPack)
include(CpackNSIS)
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
get_target_property(PNID ${PROJECT_NAME} INCLUDE_DIRECTORIES)
message(STATUS "PROJECT_NAME INCLUDE_DIRECTORIES: ${PNID}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")
message(STATUS "CMAKE_LIBRARY_PATH: ${CMAKE_LIBRARY_PATH}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
cmake_print_variables(CMAKE_C_COMPILER CMAKE_MAJOR_VERSION META_FILES_TO_INCLUDE)

message_cpack_summary()
message(STATUS "======================== END CMAKE SUMMARY ====================================")

