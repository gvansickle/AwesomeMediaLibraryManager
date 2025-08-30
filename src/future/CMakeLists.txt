#
# Copyright 2018, 2024 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
	enable_shared_from_this_virtual.h
	function_traits.hpp
	future_type_traits.hpp
	future_algorithms.h
	cpp14_concepts.hpp
	cloneable.h
	grvs_functional.hpp
	guideline_helpers.h
	InsertionOrderedMap.h
	overloaded.h
	preproc.h
	string_ops.h
	Unit.hpp
	)
set(SOURCE_FILES_SRC_FUTURE
	function_traits.cpp
	guideline_helpers.cpp
	InsertionOrderedMap.cpp
	string_ops.cpp
	)

add_library(future STATIC EXCLUDE_FROM_ALL)
target_sources(future
	PRIVATE
		${HEADER_FILES_SRC_FUTURE}
		${SOURCE_FILES_SRC_FUTURE}
)
target_include_directories(future
	PUBLIC
		${CMAKE_CURRENT_LIST_DIR}
		${CMAKE_CURRENT_LIST_DIR}/.. # To allow <future/whatever.h>
	PRIVATE
		# For config.h
		${PROJECT_BINARY_DIR}/src
)

target_link_libraries(future
	PUBLIC
		${PROJECT_COMMON_EXTERNAL_LINK_LIB_TARGETS}
)


