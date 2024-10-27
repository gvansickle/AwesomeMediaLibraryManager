#
# Copyright 2018, 2019, 2024 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
	ColumnSpec.cpp
	ModelIterators.cpp
    PlaceholderTreeModelItem.cpp
	ScanResultsTreeModel.cpp
	ScanResultsTreeModelItem.cpp
	SRTMItemLibEntry.cpp
	ThreadsafeTreeModel.cpp
    UndoRedoHelper.cpp
	# ETM
	treeitem.cpp
	treemodel.cpp
	)
set(HEADER_FILES_SRC_LOGIC_MODELS
		AbstractHeaderSection.h
		AbstractModelRelation.h
		AbstractTreeModel.h
		AbstractTreeModelItem.h
		AbstractTreeModelHeaderItem.h
		ColumnSpec.h
		ModelIterators.h
        PlaceholderTreeModelItem.h
		ScanResultsTreeModel.h
		ScanResultsTreeModelItem.h
		SRTMItemLibEntry.h
		ThreadsafeTreeModel.h
		UndoRedoHelper.h
		# ETM
		treeitem.h
		treemodel.h
		)

add_library(models STATIC EXCLUDE_FROM_ALL)
target_sources(models
	PRIVATE
		${HEADER_FILES_SRC_LOGIC_MODELS}
		${SOURCE_FILES_SRC_LOGIC_MODELS}
	)

target_link_libraries(models
	PUBLIC
		${QT5_LINK_LIB_TARGETS}
		${KF5_LINK_LIB_TARGETS}
#		${GTK_LINK_LIB_TARGETS}
		logic
	PRIVATE
		serialization
)
target_include_directories(models
	PRIVATE
	    # For config.h
		${PROJECT_BINARY_DIR}/src
		# For anything under src: utils/, logic/, etc.
		${PROJECT_SOURCE_DIR}/src
	)
