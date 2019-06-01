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

# @file src/logic/models/CMakeLists.txt

set(SOURCE_FILES_SRC_LOGIC_MODELS
	AbstractHeaderSection.cpp
	AbstractModelRelation.cpp
	AbstractTreeModel.cpp
	AbstractTreeModelItem.cpp
	AbstractTreeModelHeaderItem.cpp
	ScanResultsTreeModel.cpp
	ScanResultsTreeModelItem.cpp
	)
set(HEADER_FILES_SRC_LOGIC_MODELS
		AbstractHeaderSection.h
		AbstractModelRelation.h
		AbstractTreeModel.h
		AbstractTreeModelItem.h
		AbstractTreeModelHeaderItem.h
		ScanResultsTreeModel.h
		ScanResultsTreeModelItem.h
		)

target_sources_local(models
	PUBLIC
		${HEADER_FILES_SRC_LOGIC_MODELS}
	PRIVATE
		${SOURCE_FILES_SRC_LOGIC_MODELS}
	)

target_link_libraries(models
	PUBLIC
		${QT5_LINK_LIB_TARGETS}
		${KF5_LINK_LIB_TARGETS}
		${GTK_LINK_LIB_TARGETS}
)
target_include_directories(models
	PUBLIC
	    # For config.h
		${PROJECT_BINARY_DIR}/src
	PRIVATE
		# For serialization
		"${CMAKE_CURRENT_LIST_DIR}/../serialization"
		# For anything under src: utils/, logic/, etc.
		${PROJECT_SOURCE_DIR}/src
	)
