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
	ScanResultsTreeModelXMLTags.cpp
	)
set(HEADER_FILES_SRC_LOGIC_MODELS
		AbstractHeaderSection.h
		AbstractModelRelation.h
		AbstractTreeModel.h
		AbstractTreeModelItem.h
		AbstractTreeModelHeaderItem.h
		ScanResultsTreeModel.h
		ScanResultsTreeModelItem.h
		ScanResultsTreeModelXMLTags.h
		)
prepend_to_each(SOURCE_FILES_SRC_LOGIC_MODELS "${CMAKE_CURRENT_LIST_DIR}/" ${SOURCE_FILES_SRC_LOGIC_MODELS})
prepend_to_each(HEADER_FILES_SRC_LOGIC_MODELS "${CMAKE_CURRENT_LIST_DIR}/" ${HEADER_FILES_SRC_LOGIC_MODELS})

target_sources(models_subdir
	PUBLIC
		${HEADER_FILES_SRC_LOGIC_MODELS}
	PRIVATE
		${SOURCE_FILES_SRC_LOGIC_MODELS}
	)

target_link_libraries(models_subdir
	PUBLIC
	    #${PROJECT_COMMON_EXTERNAL_LINK_LIB_TARGETS} ### @todo "logic_subdir links to itself".
		${QT5_LINK_LIB_TARGETS}
		${KF5_LINK_LIB_TARGETS}
		${GTK_LINK_LIB_TARGETS}
#		${AMLM_TAGLIB_TARGET}
#		${AMLM_LIBCUE_TARGET}
)
target_include_directories(models_subdir
	PUBLIC
	    # For config.h
		${PROJECT_BINARY_DIR}/src
	PRIVATE
		# For serialization
		"${CMAKE_CURRENT_LIST_DIR}/../serialization"
	)
