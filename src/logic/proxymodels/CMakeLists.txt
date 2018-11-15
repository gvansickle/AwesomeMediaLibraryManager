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

# @file src/logic/proxymodels/CMakeLists.txt

set(proxymodels_subdir_SOURCES
	SelectionFilterProxyModel.h
	SelectionFilterProxyModel.cpp
	ModelHelpers.h
	LibrarySortFilterProxyModel.h
	LibrarySortFilterProxyModel.cpp
	ModelChangeWatcher.cpp
	ModelChangeWatcher.h
	QPersistentModelIndexVec.h
	QPersistentModelIndexVec.cpp
	)
prepend_to_each(proxymodels_subdir_SOURCES "${CMAKE_CURRENT_LIST_DIR}/" ${proxymodels_subdir_SOURCES})



target_sources(proxymodels_subdir
	PUBLIC
		${proxymodels_subdir_SOURCES}
	)

target_link_libraries(proxymodels_subdir
	PUBLIC
	    #${PROJECT_COMMON_EXTERNAL_LINK_LIB_TARGETS} ### @todo "logic_subdir links to itself".
		${QT5_LINK_LIB_TARGETS}
		${KF5_LINK_LIB_TARGETS}
		${GTK_LINK_LIB_TARGETS}
#		${AMLM_TAGLIB_TARGET}
#		${AMLM_LIBCUE_TARGET}
)
target_include_directories(proxymodels_subdir
	PUBLIC
	    # For config.h
		${PROJECT_BINARY_DIR}/src
	)
