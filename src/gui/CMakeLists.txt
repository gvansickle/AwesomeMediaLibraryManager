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

# gui files
set(gui_HEADER_FILES
	CollectionDockWidget.h
	Experimental.h
	FilterWidget.h
	ItemDelegateLength.h
	MainWindow.h
	MDIArea.h
	MDILibraryView.h
	MDIPlaylistView.h
	MDITreeViewBase.h
	MetadataDockWidget.h
	NetworkAwareFileDialog.h
	PixmapLabel.h
	PlayerControls.h
	)
set(gui_SOURCE_FILES
	CollectionDockWidget.cpp
	Experimental.cpp
	FilterWidget.cpp
	ItemDelegateLength.cpp
	MainWindow.cpp
	MDIArea.cpp
	MDILibraryView.cpp
	MDIPlaylistView.cpp
	MDITreeViewBase.cpp
	MetadataDockWidget.cpp
	NetworkAwareFileDialog.cpp
	PixmapLabel.cpp
	PlayerControls.cpp
	)



# Settings dialog files (in settings/).
set(gui_settings_SOURCE_FILES
	DefaultSettingsHelper.cpp
	DefaultSettingsHelper.h
	SettingsPageGeneral.cpp
	SettingsPageGeneral.h
	SettingsPageGeneral.ui
	SettingsPageCollection.cpp
	SettingsPageCollection.h
	SettingsPageCollection.ui
	SettingsPageAppearance.cpp
	SettingsPageAppearance.h
	SettingsPageAppearance.ui
	SettingsPageLibrary.cpp
	SettingsPageLibrary.h
	SettingsPageLibrary.ui
	SettingsDialog.h
	SettingsDialogSideWidget.h
	SettingsDialog.cpp
	SettingsDialogPageBase.cpp
	SettingsDialogPageBase.h
	SDPageAppearance.cpp
	SDPageAppearance.h
	SettingsDialogSideWidget.cpp
	SDPageLibrary.cpp
	SDPageLibrary.h
	SettingsDialogBase.cpp
	SettingsDialogBase.h
	)
PREPEND_TO_EACH(gui_settings_SOURCE_FILES "settings/" ${gui_settings_SOURCE_FILES})

set(gui_SRCS
	${gui_SOURCE_FILES}
	${gui_HEADER_FILES}
	${gui_settings_SOURCE_FILES}
	)

#add_library(amlm_gui_objlib OBJECT EXCLUDE_FROM_ALL ${gui_SRCS})
#target_include_directories(amlm_gui_objlib
#	PUBLIC
#		${CMAKE_CURRENT_BINARY_DIR}/../src # For <config.h>
#		)


### @todo REFACTOR
set(nomoc_HEADERS ${HEADER_FILES_UTILS_NOMOC})
PREPEND_TO_EACH(nomoc_HEADERS "${PROJECT_SOURCE_DIR}/src/" ${nomoc_HEADERS})

add_library(amlm_gui STATIC ${gui_SRCS}
			 ${nomoc_HEADERS} ### @todo REFACTOR 
	#INTERFACE EXCLUDE_FROM_ALL
	#GLOBAL # So this is target is visible in containing directories.
	)
#target_sources(amlm_gui INTERFACE $<TARGET_OBJECTS:amlm_gui_objlib>)
target_include_directories(amlm_gui
	PUBLIC
		${CMAKE_BINARY_DIR}/src # For <config.h>
		${CMAKE_CURRENT_BINARY_DIR}/settings # For generated ui_Settings.*.h.
		${CMAKE_CURRENT_BINARY_DIR}
		${PROJECT_SOURCE_DIR}
		${Qt5Core_PRIVATE_INCLUDE_DIRS}
		${Qt5Gui_PRIVATE_INCLUDE_DIRS}
		)
target_link_libraries(amlm_gui PRIVATE ${PROJECT_COMMON_EXTERNAL_LINK_LIB_TARGETS})

