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

add_library(proxymodels_subdir STATIC
	EXCLUDE_FROM_ALL
	SelectionFilterProxyModel.h
	ModelHelpers.h
	LibrarySortFilterProxyModel.h
	LibrarySortFilterProxyModel.cpp
	ModelChangeWatcher.cpp
	ModelChangeWatcher.h
	QPersistentModelIndexVec.h
	QPersistentModelIndexVec.cpp
	SelectionFilterProxyModel.cpp
	)
	
target_link_libraries(proxymodels_subdir
	PUBLIC
	    #${PROJECT_COMMON_EXTERNAL_LINK_LIB_TARGETS} ### @todo "logic_subdir links to itself".
		${QT5_LINK_LIB_TARGETS}
		${KF5_LINK_LIB_TARGETS}
		${GTK_LINK_LIB_TARGETS}
		${AMLM_TAGLIB_TARGET}
		${AMLM_LIBCUE_TARGET}
)
target_include_directories(proxymodels_subdir
	PUBLIC
	    # For config.h
		${PROJECT_BINARY_DIR}/src
	)
