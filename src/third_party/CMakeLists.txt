#
# Copyright 2017, 2024, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(libcue)

message(STATUS "Processing third_party/CMakeLists.txt")

if(WIN32)
    ### NOTE: Make sure CMAKE_PREFIX_PATH has the path to the directory, so cmake can find win_flex and win_bison.
    ### On Windows they're in a couple places, e.g.:
    # C:\Qt\5.12.0\Src\gnuwin32\bin\bison.exe
    # C:\CraftRoot\build\dev-utils\flexbison\work\win_bison.exe
    # C:\CraftRoot\build\dev-utils\flexbison\image-Debug-2.5.14\dev-utils\bin\win_bison.exe
        # We also need png2ico on Windows, which is here:
        # C:\CraftRoot\dev-utils\bin\png2ico.exe
    set(CMAKE_VERBOSE_MAKEFILE ON)
    set(FLEX_FLAGS_EXTRA --wincompat)
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH};C:/CraftRoot/dev-utils/bin;C:/Qt/5.12.0/Src/gnuwin32/bin)
endif()

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

#include(GNUInstallDirs)

set(LIBCUE_SRC_DIR libcue)

set(HEADERS
    libcue/cd.h
    libcue/cdtext.h
    libcue/libcue.h
    libcue/rem.h
    libcue/time.h
    )

set(CUE_SOURCES ${HEADERS}
        ${LIBCUE_SRC_DIR}/cd.c
        ${LIBCUE_SRC_DIR}/cdtext.c
        ${LIBCUE_SRC_DIR}/rem.c
        ${LIBCUE_SRC_DIR}/time.c)

bison_target(CueParser
        ${LIBCUE_SRC_DIR}/cue_parser.y
        ${libcue_BINARY_DIR}/cue_parser.c
        COMPILE_FLAGS ${BISON_FLAGS_EXTRA})
flex_target(CueScanner
        ${LIBCUE_SRC_DIR}/cue_scanner.l
        ${libcue_BINARY_DIR}/cue_scanner.c
        COMPILE_FLAGS ${FLEX_FLAGS_EXTRA})
ADD_FLEX_BISON_DEPENDENCY(CueScanner CueParser)

# Generated files don't want AUTOMOC etc. applied to them.
# Actually this whole directory should be excluded.
set_property(SOURCE "${libcue_BINARY_DIR}/cue_parser.h" PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE "${libcue_BINARY_DIR}/cue_parser.c" PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE "${libcue_BINARY_DIR}/cue_scanner.c" PROPERTY SKIP_AUTOGEN ON)


# Create the "libcue" target.
add_library(libcue STATIC ${CUE_SOURCES}
        ${BISON_CueParser_OUTPUTS}
        ${FLEX_CueScanner_OUTPUTS})
target_include_directories(libcue PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libcue)
if(WIN32)
    # On Windows the including exe will need to define out the __attribute__((visibility))
    # CMake + VS makes this difficult (no function-type macros on command line).
    target_include_directories(libcue PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libcue>
            $<INSTALL_INTERFACE:.>
            $<INSTALL_INTERFACE:libcue>)
    target_compile_options(libcue PUBLIC -FIlibcue_config_win.h)
endif()
set_target_properties(libcue
	PROPERTIES C_VISIBILITY_PRESET hidden)
set_property(TARGET libcue 
			PROPERTY SKIP_AUTOGEN ON)

#install(TARGETS libcue
#        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
#install(FILES ${libcue_SOURCE_DIR}/libcue.h
#        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

## TagLib
#set(CMAKE_FIND_PACKAGE_NAME "TagLibPrefix")
#find_package(utf8cpp REQUIRED
#	PATHS ./utfcpp-4.0.6
#)
set(utf8cpp_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/utfcpp-4.0.6/source)
#set(CMAKE_FIND_PACKAGE_PREFER_CONFIG FALSE)
#find_package(TagLib REQUIRED
#	PATHS ./taglib
#)
#if(TagLib_DIR)
#	message("FOUND TAGLIB DIR: $TagLib_DIR")
#endif()
#add_subdirectory(utfcpp-4.0.6)
add_subdirectory(taglib)
#include(ExternalProject)
#ExternalProject_Add(taglib
#	SOURCE_DIR ./taglib
#)
#set_property(TARGET TagLib::TagLib PROPERTY IMPORTED_GLOBAL TRUE)

message(STATUS "Completed processing third_party/CMakeLists.txt")
