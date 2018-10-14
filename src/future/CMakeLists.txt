#
# Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

# @file src/future/CMakeLists.txt

set(HEADER_FILES_SRC_FUTURE
	deduced_type.hpp
	function_traits.hpp
	future_type_traits.hpp
	cpp14_concepts.hpp
	static_if.hpp
	Unit.hpp
	)
set(SOURCE_FILES_SRC_FUTURE
	function_traits.cpp
	)
#set(UI_FILES_SRC_FUTURE
#	PARENT_SCOPE
#	)

if(false) # Not sure this is worth the effort and confusion.
	# Prepend directory.
	prepend_to_each(HEADER_FILES_SRC_FUTURE "${CMAKE_CURRENT_LIST_DIR}/" ${HEADER_FILES_SRC_FUTURE})
	prepend_to_each(SOURCE_FILES_SRC_FUTURE "${CMAKE_CURRENT_LIST_DIR}/" ${SOURCE_FILES_SRC_FUTURE})
	# prepend_to_each(UI_FILES_GUI_ACTIVITYPROGRESSMANAGER "gui/" ${UI_FILES_GUI_ACTIVITYPROGRESSMANAGER})
else()
	add_library(amlm_src_future STATIC EXCLUDE_FROM_ALL "")
endif()

target_sources(amlm_src_future
	PUBLIC
	${HEADER_FILES_SRC_FUTURE}
	${SOURCE_FILES_SRC_FUTURE}
	)
target_link_libraries(amlm_src_future PUBLIC ${PROJECT_COMMON_EXTERNAL_LINK_LIB_TARGETS})
target_include_directories(amlm_src_future
	PUBLIC
	    "${CMAKE_CURRENT_LIST_DIR}"
		${boost_callable_traits_SOURCE_DIR}
		${boost_callable_traits_BINARY_DIR})
set_target_properties(amlm_src_future
    PROPERTIES
    AUTOMOC ON
    AUTORCC OFF
    AUTOUIC OFF)
    