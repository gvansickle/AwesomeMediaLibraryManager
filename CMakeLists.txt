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

# Min versions of Qt5 and KF5.
set(QT_MIN_VERSION "5.9.0")
set(KF5_MIN_VERSION "5.37.0")

# KDE extra-cmake-modules, see https://api.kde.org/ecm/
find_package(ECM 5.38 REQUIRED NO_MODULE)

# Add folder where we keep support *.cmake's.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

# Prevent an in-root-of-src-tree build.
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
        message("ERROR: ${CMAKE_SOURCE_DIR} == ${CMAKE_BINARY_DIR}")
	message(FATAL_ERROR "Prevented in-root-of-tree build. Please create a build directory outside of the source code root directory and call cmake from there. Thank you.")
endif()

### KDE modules.
### @see https://api.kde.org/ecm/manual/ecm-kde-modules.7.html
include(KDEInstallDirs)
include(KDECMakeSettings)
# KDECompilerSettings
# Enables more warnings.  Transitively includes include(ECMEnableSanitizers).
# @note This disables exceptions.  We'll turn them back on with kde_enable_exceptions() below.
# @warning This also sets the compiler stds way too low (C89(!!))
# @see https://api.kde.org/ecm/kde-module/KDECompilerSettings.html
#include(KDECompilerSettings NO_POLICY_SCOPE)
### Set the required language standards.
# C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#kde_enable_exceptions()
# And undo even more of what KDECompilerSettings did to/for us.
###string(REPLACE "-Werror=undef" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
include(FeatureSummary)
message(STATUS "C compiler supported features = ${CMAKE_C_COMPILE_FEATURES}")
message(STATUS "C++ compiler supported features = ${CMAKE_CXX_COMPILE_FEATURES}")
###

# Use ccache if it's available.
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
	set_property(GLOBAL PROPERTY CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# Include some basic cmake functions.
### @todo Still needed after KDEInstallDirs?
###include(GNUInstallDirs)
include(CheckCXXCompilerFlag)
include(CMakePrintHelpers)
include(PrintTargetProperties)
include(helperfunctions)
find_package(PkgConfig REQUIRED)
# Include some basic Qt5-related cmake functions
include(QtCommon)
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE) # Without this, Release builds won't include the MSVC C runtime.
include(InstallRequiredSystemLibraries)
### @todo? find_package(ECM 1.7.0 REQUIRED NO_MODULE)
# For GoogleTest and Boost.
set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED FALSE")
include(ExternalProject)
include(DownloadProject)

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

# Generate the version info file.
configure_file(${PROJECT_SOURCE_DIR}/resources/VersionInfo.cpp.in
		${PROJECT_BINARY_DIR}/resources/VersionInfo.cpp)

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

# Sanitizers.
### @todo KDECompileFlags includes include(ECMEnableSanitizers), so we probably can get rid of this.
find_package(Sanitizers)

#
# The Qt5 packages we require.
#
find_package(Qt5 COMPONENTS Core Gui Widgets Multimedia Concurrent REQUIRED)
get_target_property(Qt5Core_location Qt5::Core LOCATION)
message(STATUS "* Found Qt version ${Qt5_VERSION} at ${Qt5Core_location}")
message(STATUS "*   Qt5_VERSION: ${Qt5_VERSION}")
message(STATUS "*   Qt5_DIR: ${Qt5_DIR}")
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

#
# KDE KF5 libs.
#
set(KF5_LINK_LIBS)
find_package(KF5 REQUIRED COMPONENTS
	CoreAddons ## USE_KF5_COREADDONS, git clone git://anongit.kde.org/kcoreaddons.git
	Config
	ConfigWidgets
	KIO ## USE_KDE_KIO, git clone git://anongit.kde.org/kio.git
	WidgetsAddons
	XmlGui
	GuiAddons
	IconThemes
	WindowSystem
)
list(APPEND KF5_LINK_LIBS KF5::CoreAddons KF5::ConfigCore KF5::ConfigGui)
list(APPEND KF5_LINK_LIBS KF5::KIOCore KF5::KIOFileWidgets KF5::KIOWidgets KF5::KIONTLM)
list(APPEND KF5_LINK_LIBS KF5::WidgetsAddons KF5::XmlGui KF5::GuiAddons KF5::IconThemes)
list(APPEND KF5_LINK_LIBS KF5::WindowSystem)


#
# GTK Libs.
# See https://wiki.gnome.org/Projects/gtkmm/UsingCMake, https://cmake.org/cmake/help/v3.10/module/FindPkgConfig.html
set(GTK_LINK_LIB_TARGETS)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-3.0)
list(APPEND GTK_LINK_LIB_TARGETS PkgConfig::GTKMM)

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
message(${boost_callable_traits_SOURCE_DIR} ${boost_callable_traits_BINARY_DIR})

# Right now we need all the verbosity we can get.
set(CMAKE_VERBOSE_MAKEFILE  YES)
set(GLOBAL_DEPENDS_DEBUG_MODE TRUE)
set(CPACK_IFW_VERBOSE YES)
#set(CMAKE_DEBUG_TARGET_PROPERTIES
#		INCLUDE_DIRECTORIES
#		COMPILE_DEFINITIONS
#		POSITION_INDEPENDENT_CODE
#		CONTAINER_SIZE_REQUIRED
#		LIB_VERSION
#		)

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

###
### The third-party packages we require.
###

set(HEADER_FILES_THIRD_PARTY "")
set(SOURCE_FILES_THIRD_PARTY "")

set(AMLM_LIBCUE_TARGET "")
set(AMLM_TAGLIB_TARGET "")

# Use the bundled version of libcue due to this issue: https://github.com/lipnitsk/libcue/commit/e60b7b01a40009c7c71c5e154a328881a6e9ae46
set(USE_BUNDLED_LIBCUE TRUE)

if(NOT WIN32)
    pkg_check_modules(TAGLIB REQUIRED taglib>=1.6
			IMPORTED_TARGET)
	if(TAGLIB_FOUND)
		set(AMLM_TAGLIB_TARGET "PkgConfig::TAGLIB")
	endif()
else()
    # Use the in-tree copy of TagLib.  Requires some non-obvious cmake gyrations, since TagLib needs to be installed before you can realistically include its headers.
    # Based somewhat on this SO answer: https://stackoverflow.com/a/45079298
    add_library(taglib INTERFACE)
    ExternalProject_Add(taglib_bundled
            PREFIX "taglib_bundled"  # "Relative paths are interpreted with respect to CMAKE_CURRENT_BINARY_DIR at the point where ExternalProject_Add() is called."
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib"
            INSTALL_DIR "taglib_bundled/install"
            LOG_CONFIGURE TRUE
            LOG_BUILD TRUE
            LOG_INSTALL TRUE
            # Configure step options.
            CMAKE_ARGS "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}" "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}" "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}" "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>" "-DBUILD_BINDINGS=OFF"
            # Create a CMake target "taglib_bundled-install" which can be depended upon.
            STEP_TARGETS install
            )
    add_dependencies(taglib taglib_bundled-install)
    ExternalProject_Get_Property(taglib_bundled INSTALL_DIR)
    message(STATUS "taglib_bundled INSTALL_DIR: ${INSTALL_DIR}")
#    print_target_properties(taglib_bundled)
	cmake_print_variables(CMAKE_CXX_COMPILER CMAKE_VERSION)

    set(taglib_LIBRARY "${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}tag${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(taglib_INCLUDE_DIR "${INSTALL_DIR}/include")
    message(STATUS "taglib_LIBRARY: ${taglib_LIBRARY}")
    message(STATUS "taglib_INCLUDE_DIR: ${taglib_INCLUDE_DIR}")

    # Set INTERFACE properties on our taglib INTERFACE library.
    # https://cmake.org/cmake/help/v3.10/command/add_library.html?highlight=add_librar#id6
    target_link_libraries(taglib INTERFACE ${taglib_LIBRARY})
    target_include_directories(taglib INTERFACE ${taglib_INCLUDE_DIR})
    target_compile_definitions(taglib INTERFACE "TAGLIB_STATIC")

    # Workaround needed to prevent cmake from complaining about INTERFACE_INCLUDE_DIRECTORIES being a "nonexistent path".
    # @todo looks like we no longer need this.
    #file(MAKE_DIRECTORY ${INSTALL_DIR}/include)

    set(AMLM_TAGLIB_TARGET "taglib")

    #print_target_properties(${AMLM_TAGLIB_TARGET})

endif()

if(USE_BUNDLED_LIBCUE)
	# Use the in-tree copy of libcue.
	add_subdirectory("third_party"
		EXCLUDE_FROM_ALL  # Don't tie this lib directly to the ALL target.
		)

	set(AMLM_LIBCUE_TARGET "libcue")

#	print_target_properties(${AMLM_LIBCUE_TARGET})
else()
	# Use the system libcue.
	pkg_check_modules(LIBCUE REQUIRED libcue
		IMPORTED_TARGET)
	if(LIBCUE_FOUND)
		set(AMLM_LIBCUE_TARGET "PkgConfig::LIBCUE")
	endif()
endif()

# Verdigris is just two headers.
set(USE_BUNDLED_VERDIGRIS 1)
set(INCLUDE_DIR_VERDIGRIS third_party/verdigris/src)

# Main program header files.
set(HEADER_FILES
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
	utils/AsyncDirScanner.h
	gui/AboutBox.h
	utils/RegisterQtMetatypes.h
	logic/CueSheetParser.h
	gui/settings/SettingsDialog.h
	gui/settings/SettingsDialogSideWidget.h
	resources/VersionInfo.h
	logic/ModelUserRoles.h
	gui/DragDropTreeViewStyleProxy.h
	logic/proxymodels/ModelHelpers.h
	logic/proxymodels/SelectionFilterProxyModel.h
	logic/proxymodels/ModelChangeWatcher.h
	gui/helpers/Tips.h
	gui/menus/DropMenu.h
	gui/menus/LibraryContextMenu.h
	gui/menus/PlaylistContextMenuViewport.h
	gui/menus/PlaylistContextMenu.h
	gui/menus/ActionBundle.h
	gui/mdi/MDIModelViewPair.h
	logic/ColumnSpec.h
	logic/proxymodels/QPersistentModelIndexVec.h
	utils/Logging.h
	utils/AboutDataSetup.h
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
	logic/LibraryRescanner.cpp
	gui/ActivityProgressWidget.cpp
	concurrency/ReportingRunner.cpp
	utils/AsyncDirScanner.cpp
	gui/AboutBox.cpp
	utils/RegisterQtMetatypes.cpp
	logic/CueSheetParser.cpp
	gui/settings/SettingsDialog.cpp
	gui/settings/SettingsDialogPageBase.cpp gui/settings/SettingsDialogPageBase.h
	gui/settings/SDPageAppearance.cpp gui/settings/SDPageAppearance.h
	gui/settings/SettingsDialogSideWidget.cpp
	gui/settings/SDPageLibrary.cpp
	gui/settings/SDPageLibrary.h
	gui/MDINowPlayingView.cpp
	gui/MDINowPlayingView.h
	gui/settings/RegisteredField.cpp gui/settings/RegisteredField.h gui/settings/SettingsDialogBase.cpp gui/settings/SettingsDialogBase.h
	logic/ModelUserRoles.cpp
	gui/DragDropTreeViewStyleProxy.cpp
	logic/proxymodels/SelectionFilterProxyModel.cpp
	logic/proxymodels/ModelChangeWatcher.cpp
	gui/helpers/Tips.cpp
	gui/menus/DropMenu.cpp
	gui/menus/LibraryContextMenu.cpp
	gui/menus/PlaylistContextMenuViewport.cpp
	gui/menus/PlaylistContextMenu.cpp
	gui/menus/ActionBundle.cpp
	gui/mdi/MDIModelViewPair.cpp
	logic/ColumnSpec.cpp
	logic/proxymodels/QPersistentModelIndexVec.cpp
	utils/Logging.cpp
	utils/AboutDataSetup.cpp)

# NoMoc files.
set(HEADER_FILES_UTILS_NOMOC
	utils/nomoc/nomocdefs.h
	utils/nomoc/nomocimpl.h)
list(APPEND HEADER_FILES ${HEADER_FILES_UTILS_NOMOC})

# Concurrency files.
set(HEADER_FILES_UTILS_CONCURRENCY
	concurrency/ReportingRunner.h
	concurrency/AsyncTaskManager.h
	concurrency/ExtendedDeferred.h
	concurrency/ExtFuture.h
	concurrency/ExtFutureWatcher.h
	concurrency/impl/ExtFuture_fwddecl_p.h
	concurrency/impl/ExtFuture_impl.hpp
	concurrency/ExtAsync.h
	concurrency/ExtAsyncTask.h
	concurrency/function_traits.hpp
	)
set(SOURCE_FILES_UTILS_CONCURRENCY
	concurrency/AsyncTaskManager.cpp
	concurrency/ExtAsync.cpp
	concurrency/function_traits.cpp
	)

# UI files.
set(SOURCE_FILES_UI
	gui/settings/SettingsPageGeneral.cpp
	gui/settings/SettingsPageGeneral.h
	gui/settings/SettingsPageGeneral.ui
	gui/settings/SettingsPageAppearance.cpp
	gui/settings/SettingsPageAppearance.h
	gui/settings/SettingsPageAppearance.ui
	gui/settings/SettingsPageLibrary.cpp
	gui/settings/SettingsPageLibrary.h
	gui/settings/SettingsPageLibrary.ui)

#
# Append all the SOURCE_* subsets.
#

# *.ui files.
list(APPEND SOURCE_FILES ${SOURCE_FILES_UI})

# Concurrency
list(APPEND HEADER_FILES ${HEADER_FILES_UTILS_CONCURRENCY})
list(APPEND SOURCE_FILES ${SOURCE_FILES_UTILS_CONCURRENCY})

# Append the third_party files which need to be explicitly in the SOURCE_/HEADER_FILES vars.
list(APPEND HEADER_FILES ${HEADER_FILES_THIRD_PARTY})
list(APPEND SOURCE_FILES ${SOURCE_FILES_THIRD_PARTY})

# This function sets up the variable META_FILES_TO_INCLUDE to the extra files
# needed for a windows build (essentially just the configured .rc file).
add_project_meta(META_FILES_TO_INCLUDE)
#cmake_print_variables(META_FILES_TO_INCLUDE)

# Qt5 Binary Resource (rcc) files.
qt5_add_binary_resources(icons_oxygen resources/oxygenicons.qrc)
qt5_add_binary_resources(icons_App resources/AppIcon.qrc)

# Add KConfig source files to the common SOURCE_FILES list and install it.
install(FILES AwesomeMediaLibraryManagerSettings.kcfg DESTINATION ${KCFG_INSTALL_DIR})
kconfig_add_kcfg_files(SOURCE_FILES AwesomeMediaLibraryManagerSettings.kcfgc)

###
### The main executable.
###
### @note Including HEADER_FILES in here so they get checked for existence.
###
add_executable(${PROJECT_NAME} ${OS_BUNDLE} # Expands to WIN32 or MACOS_BUNDLE depending on OS
		main.cpp
		${SOURCE_FILES} ${HEADER_FILES} ${META_FILES_TO_INCLUDE} ${RESOURCE_FILES}
		${PROJECT_BINARY_DIR}/resources/VersionInfo.cpp
        )
# Compiler features FBO Verdigris (esp. on Windows/MSVC).  -D__cpp_constexpr=201304 -D__cpp_variable_templates=201304.
target_compile_features(${PROJECT_NAME} PRIVATE cxx_relaxed_constexpr cxx_variable_templates)
target_compile_definitions(${PROJECT_NAME} PRIVATE
	"QT_SHAREDPOINTER_TRACK_POINTERS"
	"QT_DEPRECATED_WARNINGS"
	"QT_NO_KEYWORDS"
	"USE_BUNDLED_VERDIGRIS=${USE_BUNDLED_VERDIGRIS}"
	"QFUTURE_TEST")
target_include_directories(${PROJECT_NAME} PRIVATE "utils/nomoc" ${INCLUDE_DIR_VERDIGRIS})
if(NOT WIN32)
	target_compile_options(${PROJECT_NAME} PUBLIC "-Og;-ggdb;-fno-omit-frame-pointer")
endif()
target_compile_options(${PROJECT_NAME} PRIVATE ${EXTRA_CXX_COMPILE_FLAGS})
# Only the Qt stuff needs AUTOMOC.
### This doesn't work: set_property(FILES ${SOURCE_FILES} ${HEADER_FILES} PROPERTY AUTOMOC ON)
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    AUTOMOC ON
    AUTOUIC ON)
# The libs.
set(PROJECT_COMMON_LINK_LIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Multimedia Qt5::Concurrent
		${KF5_LINK_LIBS}
		${GTK_LINK_LIB_TARGETS}
		${AMLM_TAGLIB_TARGET} ${AMLM_LIBCUE_TARGET} ${AMLM_QXTGLOBALSHORTCUT_LIB_TARGET}
		callable_traits)
target_link_libraries(${PROJECT_NAME}
        ${PROJECT_COMMON_LINK_LIBS}
		)
add_dependencies(${PROJECT_NAME} icons_oxygen icons_App)
# Add sanitizers.
add_sanitizers(${PROJECT_NAME})
### @note KF5:
# The sanitizers are not enabled by default. Instead, you must set ECM_ENABLE_SANITIZERS (either in your
# CMakeLists.txt or on the command line) to a semicolon-separated list of sanitizers you wish to enable.
# The options are:
#   address memory thread leak undefined
# The sanitizers “address”, “memory” and “thread” are mutually exclusive. You cannot enable two of them in the same build.
# "leak" requires the "address" sanitizer.

# Stop clang-tidy from scanning generated files.
file(WRITE "${PROJECT_BINARY_DIR}/.clang-tidy" "
---
Checks: '-*,llvm-twine-local'
...
")

#print_target_properties(${PROJECT_NAME})



###
### Testing
###
if(TRUE) # Enable test target.

find_package(Qt5Test REQUIRED)
include(CTest)
### Google Test
find_package(Threads REQUIRED)
download_project(PROJ                googletest
                 GIT_REPOSITORY      https://github.com/google/googletest.git
                 GIT_TAG             master
                 ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)
# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR}
	EXCLUDE_FROM_ALL)

#enable_testing()
set(TEST_EXE_TARGET alltests)
set(SOURCE_FILES_TEST)
set(HEADER_FILES_TEST)
list(APPEND SOURCE_FILES_TEST
	concurrency/tests/AsyncTests.cpp)
list(APPEND
	tests/TestHelpers.h
	concurrency/tests/AsyncTests.h)

add_executable(${TEST_EXE_TARGET}
				alltests.cpp ${SOURCE_FILES_TEST} ${HEADER_FILES_TEST}
				${SOURCE_FILES} ${HEADER_FILES} ${META_FILES_TO_INCLUDE} ${RESOURCE_FILES}
				${PROJECT_BINARY_DIR}/resources/VersionInfo.cpp)
# Add sanitizers.
add_sanitizers(${TEST_EXE_TARGET})
target_compile_features(${TEST_EXE_TARGET} PRIVATE cxx_relaxed_constexpr cxx_variable_templates)
target_compile_definitions(${TEST_EXE_TARGET} PRIVATE
	"QT_SHAREDPOINTER_TRACK_POINTERS"
	"QT_DEPRECATED_WARNINGS"
	"QT_NO_KEYWORDS"
	"USE_BUNDLED_VERDIGRIS=${USE_BUNDLED_VERDIGRIS}"
	"QFUTURE_TEST")
target_include_directories(${TEST_EXE_TARGET} PRIVATE "utils/nomoc" ${INCLUDE_DIR_VERDIGRIS})
target_compile_definitions(${TEST_EXE_TARGET} PRIVATE "GTEST_LANGUAGE_CXX11")
target_compile_options(${TEST_EXE_TARGET} PRIVATE ${EXTRA_CXX_COMPILE_FLAGS})
set_target_properties(${TEST_EXE_TARGET} PROPERTIES
	AUTOMOC ON
	AUTOUIC ON)
target_link_libraries(${TEST_EXE_TARGET} Threads::Threads gtest gmock_main)
target_link_libraries(${TEST_EXE_TARGET} Qt5::Test ${PROJECT_COMMON_LINK_LIBS})
add_test(NAME ${TEST_EXE_TARGET} COMMAND ${TEST_EXE_TARGET})

endif() # Enable test target.

###
### Installation.
###
install(TARGETS ${PROJECT_NAME}
                RUNTIME DESTINATION . # Default is /usr/local/${PROJECT_NAME} on UNIX and c:/Program Files/${PROJECT_NAME}/${PROJECT_NAME} on Windows.
                LIBRARY DESTINATION .
                ARCHIVE DESTINATION .
		COMPONENT coreapp
	)
install(FILES
    ### @todo I think these are installing on top of themselves.
    #icons_oxygen.rcc icons_Tango.rcc icons_App.rcc
    ${CMAKE_CURRENT_BINARY_DIR}/icons_oxygen.rcc
    #${CMAKE_CURRENT_BINARY_DIR}/icons_Tango.rcc
    ${CMAKE_CURRENT_BINARY_DIR}/icons_App.rcc
    DESTINATION .
    COMPONENT coreapp)

# Custom target for coordinating the packaging.
add_custom_target(packaging
    ALL
	DEPENDS ${PROJECT_NAME} icons_oxygen icons_App
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
cmake_print_variables(CMAKE_CXX_COMPILER META_FILES_TO_INCLUDE)

message_cpack_summary()
message(STATUS "======================== END CMAKE SUMMARY ====================================")

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
