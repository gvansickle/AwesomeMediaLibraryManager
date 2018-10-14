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

# @file src/logic/CMakeLists.txt

#add_subdirectory(models EXCLUDE_FROM_ALL)
#add_subdirectory(dbmodels EXCLUDE_FROM_ALL)
#add_subdirectory(proxymodels EXCLUDE_FROM_ALL)

add_subdir_lib(models_subdir "${CMAKE_CURRENT_LIST_DIR}/models")
add_subdir_lib(dbmodels_subdir "${CMAKE_CURRENT_LIST_DIR}/dbmodels")
add_subdir_lib(proxymodels_subdir "${CMAKE_CURRENT_LIST_DIR}/proxymodels")

set(logic_subdir_SOURCES
	PerfectDeleter.cpp
	PerfectDeleter.h
	)
prepend_to_each(logic_subdir_SOURCES "${CMAKE_CURRENT_LIST_DIR}/" ${logic_subdir_SOURCES})

target_sources(logic_subdir
	PUBLIC
		${logic_subdir_SOURCES}
	)
target_link_libraries(logic_subdir
	PUBLIC
	    #${PROJECT_COMMON_EXTERNAL_LINK_LIB_TARGETS} ### @todo "logic_subdir links to itself".
		${QT5_LINK_LIB_TARGETS}
		${KF5_LINK_LIB_TARGETS}
		${GTK_LINK_LIB_TARGETS}
		${AMLM_TAGLIB_TARGET}
		${AMLM_LIBCUE_TARGET}
		dbmodels_subdir
		models_subdir
		proxymodels_subdir
)
target_include_directories(logic_subdir
	PUBLIC
	    # For config.h
		${PROJECT_BINARY_DIR}/src
	)



