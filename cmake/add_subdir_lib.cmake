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


#
# add_subdir_lib_internal() implementation based on the Crascit's technique here:
#   @link https://crascit.com/2016/01/31/enhanced-source-file-handling-with-target_sources/
# This is the pre-cmake-3.13 version.
# Target is EXCLUDE_FROM_ALL, so something has to depend on it for it to be built.
#
# @param add_subdir_lib_LIB_TARGET_NAME  This should be the name of the subdirectory, with "_lib" appended.  This name is what you'll use as the target link library.
# @param add_subdir_lib_SUBDIR           This should be the name of the subdirectory.
# @param add_subdir_lib_PLACEHOLDER_EXCLUDE_FROM_ALL  Pass "EXCLUDE_FROM_ALL" or "".
#
macro(add_subdir_lib_internal add_subdir_lib_LIB_TARGET_NAME add_subdir_lib_SUBDIR add_subdir_lib_PLACEHOLDER_EXCLUDE_FROM_ALL)
	if(NOT ${ARGC} EQUAL 3)
		message(FATAL_ERROR "add_subdir_lib requires 3 arguments")
	endif()
	message(STATUS "Creating library ${ARGV0} from ${ARGV1}/CMakeLists.txt with CMAKE_CURRENT_LIST_DIR: ${CMAKE_CURRENT_LIST_DIR}")
	add_library(${ARGV0} STATIC ${PLACEHOLDER_EXCLUDE_FROM_ALL} "")
	include(${ARGV1}/CMakeLists.txt)
endmacro()

#
# add_subdir_lib(): The missing CMake add_*().  Adds a subdirectory as a library target in this directory.
#
macro(add_subdir_lib add_subdir_lib_LIB_TARGET_NAME)
	# Option flags we understand.
	set(options CREATE_STATIC_LIB EXCLUDE_FROM_ALL)
	# Arguments taking 1 value.
	#set(oneValueArgs DESTINATION RENAME)
	# Arguments taking multiple values.
	#set(multiValueArgs TARGETS CONFIGURATIONS)
	cmake_parse_arguments(ADD_SUBDIR_LIB
		"${options}"
		"${oneValueArgs}"
		"${multiValueArgs}"
		${ARGN})
	message(STATUS "ADD_SUBDIR_LIB_UNPARSED_ARGUMENTS: ${ADD_SUBDIR_LIB_UNPARSED_ARGUMENTS}")
	if((${ARGC} LESS "1") OR (${ARGC} GREATER "3"))
		message(FATAL_ERROR "add_subdir_lib() requires 1-3 arguments, ${ARGC} arguments provided.")
	endif()
	# @todo WIP
	message(STATUS "=========================================================================")
	set(save_LIB_TARGET_NAME "${ARGV0}")
	message(STATUS "save_LIB_TARGET_NAME: ${save_LIB_TARGET_NAME}")
	# Make a unique var, since we're in the calling scope.
	# Make a string for the directory.
	#set(varname add_subdir_lib_${add_subdir_lib_LIB_TARGET_NAME}_SUBDIRECTORY)
	set(varname "${save_LIB_TARGET_NAME}")
	message(STATUS "varname: ${varname}")
#	string(REGEX REPLACE "^(.*)_subdir$" "\\1" varname "${varname}")
#	message(STATUS "varname: ${varname}")
	# Create an absolute path to the subdir.
	set(add_subdir_lib_SUBDIR "${CMAKE_CURRENT_LIST_DIR}/${varname}")

	if(ADD_SUBDIR_LIB_PLACEHOLDER_EXCLUDE_FROM_ALL)
		set(EFA "EXCLUDE_FROM_ALL")
	else()
		set(EFA "")
	endif()

	if(ADD_SUBDIR_LIB_CREATE_STATIC_LIB)
		set(CSL "EXCLUDE_FROM_ALL")
	else()
		set(CSL "") # @todo
	endif()

	message(STATUS "add_subdir_lib_internal( '${add_subdir_lib_LIB_TARGET_NAME}' '${add_subdir_lib_SUBDIR}' '${EFA}')")

	# Create the subdir library
	add_subdir_lib_internal(${add_subdir_lib_LIB_TARGET_NAME} ${add_subdir_lib_SUBDIR} "${EFA}")
	message(STATUS "=========================================================================")
endmacro()



# From Crascit: @link https://crascit.com/2016/01/31/enhanced-source-file-handling-with-target_sources/
# This is a compatibility helper function for cmake pre-3.13 and the target_sources() subdir handling idiom
# in his article at the link above.
# NOTE: This helper function assumes no generator expressions are used for the source files
function(target_sources_local target)
  if(POLICY CMP0076)
	# New behavior is available, so just forward to it by ensuring
	# that we have the policy set to request the new behavior, but
	# don't change the policy setting for the calling scope
	cmake_policy(PUSH)
	cmake_policy(SET CMP0076 NEW)
	target_sources(${target} ${ARGN})
	cmake_policy(POP)
	return()
  endif()

  # Must be using CMake 3.12 or earlier, so simulate the new behavior
  unset(_srcList)
  get_target_property(_targetSourceDir ${target} SOURCE_DIR)

  foreach(src ${ARGN})
	if(NOT src STREQUAL "PRIVATE" AND
			NOT src STREQUAL "PUBLIC" AND
	   NOT src STREQUAL "INTERFACE" AND
	   NOT IS_ABSOLUTE "${src}")
      # Relative path to source, prepend relative to where target was defined
	  file(RELATIVE_PATH src "${_targetSourceDir}" "${CMAKE_CURRENT_LIST_DIR}/${src}")
    endif()
	list(APPEND _srcList ${src})
  endforeach()
  target_sources(${target} ${_srcList})
endfunction()
