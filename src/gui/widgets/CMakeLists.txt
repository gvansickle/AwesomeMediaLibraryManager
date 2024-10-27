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

set(gui_widgets_HEADER_FILES
		ExperimentalKDEView1.h
		CollectionStatsWidget.h
		CollectionView.h
		PixmapLabel.h
		PlayerControls.h
	)
set(gui_widgets_SOURCE_FILES
		ExperimentalKDEView1.cpp
		CollectionStatsWidget.cpp
		CollectionView.cpp
		PixmapLabel.cpp
		PlayerControls.cpp
	)
set(gui_widgets_UI_FILES
		ExperimentalKDEView1.ui
		CollectionView.ui
 	)

#target_sources_local(widgets
#	PUBLIC
add_library(widgets STATIC EXCLUDE_FROM_ALL)
target_sources(widgets
	PRIVATE
		${gui_widgets_HEADER_FILES}
		${gui_widgets_SOURCE_FILES}
		${gui_widgets_UI_FILES}
	)
set_target_properties(widgets
    PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
		AUTORCC ON
		)
target_link_libraries(widgets
	PUBLIC
		${QT5_LINK_LIB_TARGETS}
		${KF5_LINK_LIB_TARGETS}
#		${GTK_LINK_LIB_TARGETS}
		jobs  ### For LibraryEntryLoaderJob.h via LibraryModel.h.
        models
		)
target_include_directories(widgets
	PUBLIC
		# For config.h
		${PROJECT_BINARY_DIR}/src
		# For nomoc.
		${PROJECT_SOURCE_DIR}/src/utils/nomoc
	PRIVATE
		# For anything in <root>/src: utils, logic, future, gui.
		${PROJECT_SOURCE_DIR}/src
		)
