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

function(print_varlist)
	foreach(element ${ARGN})
		#cmake_print_variables(element)
		### @todo This mostly works, except for vars like e.g. KDE_INSTALL_TARGETS_DEFAULT_ARGS,
		### which are printed with all spaces and underscores(?yeah) replaced with ";".
		### @see https://gitlab.kitware.com/cmake/community/wikis/FAQ#why-do-i-have-unwanted-semicolons-in-my-compiler-flags
		message(STATUS "${element}: \'${${element}}\'")
	endforeach()
endfunction()

function(print_compilers_and_params)
    foreach(lang C CXX)
        message("* Compiler for language ${lang}: ${CMAKE_${lang}_COMPILER}")
        foreach(build_type DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
            message("*   Flags for language ${lang} + build type ${build_type}: ${CMAKE_${lang}_FLAGS_${build_type}}")
        endforeach()
    endforeach()
endfunction()


#
# The missing CMake add_*.  Adds a subdirectory as a library target in this directory.
# Based on @link https://crascit.com/2016/01/31/enhanced-source-file-handling-with-target_sources/
# Target is EXCLUDE_FROM_ALL, so something has to depend on it for it to be built.
#
macro(add_subdir_lib add_subdir_lib_LIB_TARGET_NAME add_subdir_lib_SUBDIR)
	if(NOT ${ARGC} EQUAL 2)
		message(FATAL_ERROR "add_subdir_lib requires two arguments")
	endif()
	message(STATUS "########################## Creating library ${ARGV0} from ${ARGV1}/CMakeLists.txt with CMAKE_CURRENT_LIST_DIR: ${CMAKE_CURRENT_LIST_DIR}")
	add_library(${ARGV0} STATIC EXCLUDE_FROM_ALL "")
	include(${ARGV1}/CMakeLists.txt)
endmacro()


# @note cmake 3.12 has this functionality in a one-liner:
# list(TRANSFORM ${FILES_TO_TRANSLATE} PREPEND ${CMAKE_CURRENT_SOURCE_DIR})
# @see https://cmake.org/cmake/help/v3.12/command/list.html#transform
function(PREPEND_TO_EACH outvar prefix)
#	message(STATUS "PREPEND_TO_EACH: OUTVAR: ${outvar} PREFIX: ${prefix} ARGC: ${ARGC} ARGN: ${ARGN}")
	set(listvar "")
	foreach(element ${ARGN})
#		message(STATUS "FOREACH: ${element}")
		list(APPEND listvar "${prefix}${element}")
	endforeach()
	set(${outvar} "${listvar}" PARENT_SCOPE)
endfunction()


function(print_qtinfo)
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
ENDFUNCTION()

macro(message_cpack_summary)
    message(STATUS "CPackIFW Tools found:")
    message(STATUS "=====================")
    message(STATUS "CPACK_IFW_FRAMEWORK_VERSION: ${CPACK_IFW_FRAMEWORK_VERSION}")
    message(STATUS "CPACK_IFW_BINARYCREATOR_EXECUTABLE: ${CPACK_IFW_BINARYCREATOR_EXECUTABLE}")
    message(STATUS "CPACK_IFW_INSTALLERBASE_EXECUTABLE: ${CPACK_IFW_INSTALLERBASE_EXECUTABLE}")
    message(STATUS "Installer info:")
    message(STATUS "===============")
    message(STATUS "CPACK_PACKAGE_NAME: ${CPACK_PACKAGE_NAME}")
    message(STATUS "CPACK_PACKAGE_FILE_NAME: ${CPACK_PACKAGE_FILE_NAME}") # Name of the package file to generate.
    message(STATUS "CPACK_PACKAGE_EXECUTABLES: ${CPACK_PACKAGE_EXECUTABLES}") # List of the exe's+text label used to create Start Menu shortcuts.
    message(STATUS "CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME: ${CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME}")
    message(STATUS "CPACK_IFW_PACKAGE_NAME: ${CPACK_IFW_PACKAGE_NAME}")
    message(STATUS "CPACK_IFW_TARGET_DIRECTORY: ${CPACK_IFW_TARGET_DIRECTORY}")
    cmake_print_variables(CPACK_NSIS_INSTALL_ROOT)
    cmake_print_variables(CPACK_NSIS_DISPLAY_NAME)
    cmake_print_variables(CPACK_NSIS_PACKAGE_NAME)
    cmake_print_variables(CPACK_NSIS_coreapp_INSTALL_DIRECTORY)
    cmake_print_variables(CPACK_NSIS_EXECUTABLES_DIRECTORY)
    cmake_print_variables(CPACK_IFW_PACKAGE_NAME CPACK_IFW_FRAMEWORK_VERSION)
endmacro()


