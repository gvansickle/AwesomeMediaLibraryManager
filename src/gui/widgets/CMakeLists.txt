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

set(HEADER_FILES_GUI_WIDGETS
	CollectionStatsWidget.h
	CollectionView.h
	PixmapLabel.h
	PlayerControls.h
	)
set(SOURCE_FILES_GUI_WIDGETS
	CollectionStatsWidget.cpp
	CollectionView.cpp
	PixmapLabel.cpp
	PlayerControls.cpp
	)
set(UI_FILES_GUI_WIDGETS
	CollectionView.ui
 	)

# Prepend directory. 
prepend_to_each(HEADER_FILES_GUI_WIDGETS "${CMAKE_CURRENT_LIST_DIR}/" ${HEADER_FILES_GUI_WIDGETS})
prepend_to_each(SOURCE_FILES_GUI_WIDGETS "${CMAKE_CURRENT_LIST_DIR}/" ${SOURCE_FILES_GUI_WIDGETS})
prepend_to_each(UI_FILES_GUI_WIDGETS "${CMAKE_CURRENT_LIST_DIR}/" ${UI_FILES_GUI_WIDGETS})

#target_sources(widgets
#	PUBLIC
list(APPEND amlm_widget_SRCS
	${HEADER_FILES_GUI_WIDGETS}
	${SOURCE_FILES_GUI_WIDGETS}
	${UI_FILES_GUI_WIDGETS}
	)
