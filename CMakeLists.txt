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
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
cmake_policy(SET CMP0053 NEW) # Use new var ref/esc seq eval
cmake_policy(SET CMP0010 NEW) # Silence warning on CMP0010.

message(STATUS "CMake-time PATH: $ENV{PATH}")
message(STATUS "CMake-time CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message(STATUS "CMake-time CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
include(cmake/GetVersionInfo.cmake)

# Prevent an in-root-of-src-tree build and any accidental build-induced changes to source files.
# Per @link https://stackoverflow.com/questions/1208681/with-cmake-how-would-you-disable-in-source-builds, these have to
# be prior to project().
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Basic information about project
# Note that here is where CMake determines compilers and loads any toolchain.cmake file.
# (per https://cgold.readthedocs.io/en/latest/tutorials/project.html)
project(AwesomeMediaLibraryManager
        VERSION ${GVI_VERSION_TAG_QUAD}
        DESCRIPTION "The Awesome Media Library Manager"
        LANGUAGES C CXX)

# Prevent an in-root-of-src-tree build.
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
        message("ERROR: ${CMAKE_SOURCE_DIR} == ${CMAKE_BINARY_DIR}")
	message(FATAL_ERROR "Prevented in-root-of-tree build. Please create a build directory outside of the source code root directory and call cmake from there. Thank you.")
endif()

# Additional project info.
set(PROJECT_AUTHOR "Gary R. Van Sickle")
set(PROJECT_AUTHOR_EMAIL "grvs@users.sourceforge.net")
set(FULL_COPYRIGHT_STRING "Copyright (c) 2017, 2018 ${PROJECT_AUTHOR} <${PROJECT_AUTHOR_EMAIL}>")

# Early include of some basic cmake stuff.
include(CMakePrintHelpers)

### Set the required language standards.
# C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Min versions of Qt5, KF5, KF5/ECM.
set(QT_MIN_VERSION "5.11.0")
set(KF5_MIN_VERSION "5.51.0")
set(ECM_MIN_VERSION "5.51.0")

###
### ECM Setup ====================================================================================================
###

# KDE extra-cmake-modules, see https://api.kde.org/ecm/
# @note find_package() MODULE vs. NO_MODULE (==CONFIG mode):
# "Config mode provides an elaborate interface and search procedure. Much of the interface is provided for completeness
#  and for use internally by find-modules loaded by Module mode. Most user code should simply call:
#    find_package(<package> [major[.minor]] [EXACT] [REQUIRED|QUIET])"
find_package(PkgConfig REQUIRED) # @note I see PkgConfig just before ECM in a few KDE scripts.
find_package(ECM ${ECM_MIN_VERSION} REQUIRED NO_MODULE)

# Add dirs where we keep support *.cmake's, ucm, cotire, ECM, etc.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake
		#${CMAKE_SOURCE_DIR}/cmake/cotire/CMake
		${CMAKE_SOURCE_DIR}/cmake/ucm/cmake
		${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR}
		${CMAKE_SOURCE_DIR}/src/third_party/continuable/cmake)

###
### ECM Macros
### Note: The order here follows that of KDevelop and Calligra's top-level CMakeLists.txt's.  Some macros used there are omitted here.
###

# ECM KDE macros
# @note Order matters for these next two include()s.  Per the docs, InstallDirs needs to come before CMakeSettings.
# @link https://api.kde.org/ecm/kde-module/KDECMakeSettings.html#runtime-paths
include(KDEInstallDirs)  ##< Replaces/uses GNUInstallDirs.
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
#   address memory thread leak undefined
# The sanitizers "address", "memory" and "thread" are mutually exclusive. You cannot enable two of them in the same build.
# "leak" requires the "address" sanitizer.
### @note KDECompileFlags includes include(ECMEnableSanitizers).
#set(ECM_ENABLE_SANITIZERS "leak;address")
include(ECMEnableSanitizers)

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
if(UNIX)
ucm_add_flags(-fmessage-length=0 -pthread -Werror=return-type -Wmaybe-uninitialized -Werror=maybe-uninitialized)
endif(UNIX)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	message(STATUS "C++ Compiler is GCC")
	ucm_add_flags(-Og -ggdb3 -fno-omit-frame-pointer -fvar-tracking -fvar-tracking-assignments -gpubnames -ggnu-pubnames)
	ucm_add_flags(-grecord-gcc-switches -gas-loc-support -gstatement-frontiers -ginline-points -femit-struct-debug-detailed=any)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	message(STATUS "C++ Compiler is Clang")
	ucm_add_flags(-Og -ggdb3 -Wthread-safety -fno-omit-frame-pointer -fdebug-macro -fdiagnostics-show-template-tree
		-fdelayed-template-parsing -frelaxed-template-template-args)
endif()
if(WIN32)
#ucm_add_flags(/showIncludes)
endif(WIN32)
message(STATUS "C compiler supported features = ${CMAKE_C_COMPILE_FEATURES}")
message(STATUS "C++ compiler supported features = ${CMAKE_CXX_COMPILE_FEATURES}")
message(STATUS "C++ CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
message(STATUS "C++ CMAKE_CXX_FLAGS_DEBUG = ${CMAKE_CXX_FLAGS_DEBUG}")

###

if(true)
	# Use ccache if it's available.
	find_program(CCACHE_PROGRAM ccache)
	if(CCACHE_PROGRAM)
		set_property(GLOBAL PROPERTY CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
	endif()
endif()

# Include some basic cmake functions.
include(CheckCXXCompilerFlag)
include(PrintTargetProperties)
include(helperfunctions)
# Include some basic Qt5-related cmake functions
include(QtCommon)
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE) # Without this, Release builds won't include the MSVC C runtime.
include(InstallRequiredSystemLibraries)
# For DownloadProject, which we use for GoogleTest and Boost.
set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED FALSE")
#include(ExternalProject)
include(DownloadProject)

# Right now we need all the verbosity we can get.
set(CMAKE_VERBOSE_MAKEFILE  YES)
#set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)
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

print_compilers_and_params()

# FBO at least visual studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Export compile commands.
# FBO Eclipse, clang-tidy, iwyu, etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The.  Simplest.  Things.
# Set default install location to "install" folder in build dir.
# we do not want to install to /usr by default.add_compile_options.
# "CMake sets this variable to a TRUE value when the CMAKE_INSTALL_PREFIX has just been initialized to its default value,
# typically on the first run of CMake within a new build tree. This can be used by project code to change the default 
# without overriding a user-provided value"
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
### The Qt5 packages we require.
###
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
	Core
	Gui
	Widgets
	Multimedia
	XmlPatterns
	Concurrent
	)
set(QT5_LINK_LIB_TARGETS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Multimedia Qt5::XmlPatterns Qt5::Concurrent)

# Unix-only
if(UNIX)
    find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
        X11Extras)
    list(APPEND QT5_LINK_LIB_TARGETS Qt5::X11Extras)
endif(UNIX)

print_qtinfo()

###
### KDE KF5 libs.
###
# Krita has ~this warning: "don't reorder these, this is the order they depend on each other."
set(KF5_LINK_LIB_TARGETS "")
### @note "Config" is both a find_package() keyword as well as the KF5Config components KF5::ConfigCore and KF5::ConfigGui.
###       We'll require KF5Config separately to avoid any ambiguity.
find_package(KF5Config ${KF5_MIN_VERSION} REQUIRED)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
	Archive ### @todo TRYING THIS IN CASE IconThemes needs it to work properly, otherwise REMOVE
	WidgetsAddons
	CoreAddons ## USE_KF5_COREADDONS, git clone git://anongit.kde.org/kcoreaddons.git
	GuiAddons
	ConfigWidgets ## Not in Krita from this until WindowSystem.
	KIO ## USE_KDE_KIO, git clone git://anongit.kde.org/kio.git
	ItemModels
	ItemViews
	XmlGui
	IconThemes
	WindowSystem
)
set(HAVE_KF501 ${KF5_FOUND})
list(APPEND KF5_LINK_LIB_TARGETS KF5::CoreAddons KF5::ConfigCore KF5::ConfigGui)
list(APPEND KF5_LINK_LIB_TARGETS KF5::KIOCore KF5::KIOFileWidgets KF5::KIOWidgets KF5::KIONTLM)
list(APPEND KF5_LINK_LIB_TARGETS KF5::WidgetsAddons KF5::XmlGui KF5::GuiAddons KF5::IconThemes KF5::ItemModels KF5::ItemViews)
list(APPEND KF5_LINK_LIB_TARGETS KF5::WindowSystem)

###### EXPERIMENT

#find_package(BreezeIcons REQUIRED) # Doesn't exist yet.
### @todo Need to find a better way than hardcoding the path to the rcc file here.
set(WITH_BUNDLED_BREEZE_ICONS true)
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
        message(FATAL_ERROR "Breeze-icons is missing")
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
#list(APPEND KF5_LINK_LIB_TARGETS Breeze)
print_varlist(BREEZEICONS_PATH)


###
### GTK Libs.
### See https://wiki.gnome.org/Projects/gtkmm/UsingCMake, https://cmake.org/cmake/help/v3.10/module/FindPkgConfig.html
###
set(GTK_LINK_LIB_TARGETS "")
if(UNIX)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-3.0)
list(APPEND GTK_LINK_LIB_TARGETS PkgConfig::GTKMM)
set(HAVE_GTKMM01 ${GTKMM_FOUND})
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


#
# Intel's TBB.
# CMake integration per @link https://github.com/01org/tbb/tree/tbb_2019/cmake
### This method pulls the code down from github.
# @note On my Fedora 29 box with installed binary tbb, it looks like CMake already has tbb modules at this path:
#set(PATH_TO_TBB_CMAKE_MODULES "${CMAKE_CURRENT_LIST_DIR}/cmake/tbb")
#set(TBB_ROOT "${CMAKE_CURRENT_LIST_DIR}/../tbb")
#include(${PATH_TO_TBB_CMAKE_MODULES}/TBBGet.cmake)
#include(${TBB_ROOT}/cmake/TBBBuild.cmake)
#tbb_get(TBB_ROOT tbb_root SOURCE_CODE)
#tbb_build(TBB_ROOT ${TBB_ROOT} CONFIG_DIR TBB_DIR MAKE_ARGS tbb_cpf=1)
#find_package(TBB REQUIRED tbb_preview)
#tbb_get(TBB_ROOT tbb_root CONFIG_DIR TBB_DIR)
#find_package(TBB REQUIRED tbb)

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
add_subdirectory(${boost_callable_traits_SOURCE_DIR} ${boost_callable_traits_BINARY_DIR} EXCLUDE_FROM_ALL)
print_varlist(boost_callable_traits_SOURCE_DIR boost_callable_traits_BINARY_DIR)

# Boost
# See @link https://cmake.org/cmake/help/latest/module/FindBoost.html
set(BOOST_THREAD_VERSION          4) # Latest in 1.66 as of this writing.
set(BOOST_THREAD_PROVIDES_FUTURE ON)
set(BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION ON)
set(BOOST_THREAD_FUTURE_USES_OPTIONAL ON)
set(Boost_USE_STATIC_LIBS        ON) # Only find static libs.
set(Boost_USE_STATIC_RUNTIME     ON) # Want static runtime.
set(Boost_USE_MULTITHREADED      ON) # Need multithreaded.
set(Boost_USE_DEBUG_LIBS         ON)
set(Boost_USE_RELEASE_LIBS       OFF)
#set(BOOST_ALL_DYN_LINK           OFF)
set(Boost_USE_DEBUG_RUNTIME      ON)
set(Boost_DEBUG    				OFF) # enable debug output from FindBoost.
set(Boost_DETAILED_FAILURE_MSG ON) # add detailed information to the failure message 
								   # even when the REQUIRED option is not given to the find_package call.
find_package(Boost 1.66.0 REQUIRED
	COMPONENTS
		thread
		system)
include_directories( ${Boost_INCLUDE_DIR} )
link_directories( ${Boost_LIBRARY_DIRS} )

# Qt logging categories.
# @link https://api.kde.org/ecm/module/ECMQtDeclareLoggingCategory.html
ecm_qt_declare_logging_category(QT5_LOGGING_CATEGORY_SOURCE_FILES
                                HEADER logging_cat_ExtFuture.h
                                IDENTIFIER EXTFUTURE
                                CATEGORY_NAME amlm.concurrency.extfuture
                                DEFAULT_SEVERITY Debug ### <Debug|Info|Warning|Critical|Fatal>
                                )
ecm_qt_declare_logging_category(QT5_LOGGING_CATEGORY_SOURCE_FILES
                                HEADER logging_cat_ExtAsync.h
                                IDENTIFIER EXTASYNC
                                CATEGORY_NAME amlm.concurrency.extasync
                                DEFAULT_SEVERITY Debug ### <Debug|Info|Warning|Critical|Fatal>
                                )

add_definitions(
	-DQT_DEPRECATED_WARNINGS
    #-DQT_DISABLE_DEPRECATED_BEFORE=0x051000
    -DQT_NO_SIGNALS_SLOTS_KEYWORDS
    #-DQT_NO_URL_CAST_FROM_STRING
    #-DQT_NO_CAST_FROM_ASCII
    #-DQT_NO_CAST_TO_ASCII
    #-DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_STRICT_ITERATORS
    -DQT_USE_QSTRINGBUILDER
    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)


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
	${boost_callable_traits_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/continuable/include
	${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/function2/include
	# For <enum.h> i.e. Better Enums, @link https://github.com/aantron/better-enums.
	${PROJECT_SOURCE_DIR}/src/third_party/better_enums
	### @todo REMOVE
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


###
### Add the project subdirs.
###
#set(amlm_APP_ICONS "re-set this in resources_subdir")
# This is the sum-total of all source files which go into building the main exe.
add_subdir_lib(resources_subdir "${CMAKE_CURRENT_LIST_DIR}/resources")
#message(AUTHOR_WARNING "amlm_APP_ICONS: ${amlm_APP_ICONS}")
add_subdirectory(src)
add_subdirectory(tests)


#include(resources/CMakeLists.txt)
#print_varlist(amlm_ICONS amlm_SRCS)
#ecm_add_app_icon(amlm_SRCS ICONS ${amlm_ICONS})


# This function sets up the variable META_FILES_TO_INCLUDE to the extra files
# needed for a windows build (essentially just the configured .rc file).
add_project_meta(META_FILES_TO_INCLUDE)



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
	META_FILES_TO_INCLUDE
	KDE_INSTALL_TARGETS_DEFAULT_ARGS)

#message_cpack_summary()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

message(STATUS "======================== END CMAKE SUMMARY ====================================")
