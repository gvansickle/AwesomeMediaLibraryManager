#
# Copyright 2018, 2024, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

# @file src/gui/CMakeLists.txt

# gui header files
set(gui_HEADER_FILES
	AboutBox.h
	CollectionDockWidget.h
	Experimental.h
	FilterWidget.h
	MainWindow.h
	MDIArea.h
	MDILibraryView.h
	MDIPlaylistView.h
	MDITreeViewBase.h
	MetadataDockWidget.h
	NetworkAwareFileDialog.h
	helpers/Tips.h
	menus/DropMenu.h
	menus/LibraryContextMenu.h
	menus/PlaylistContextMenuViewport.h
	menus/PlaylistContextMenu.h
	menus/ActionBundle.h
	mdi/MDIModelViewPair.h
	DragDropTreeViewStyleProxy.h
	menus/HelpMenu.h
	helpers/NetAwareFileDialogGtk3.h
	helpers/Gtk3DialogHelper.h
	helpers/IfExistsAskForOverwrite.h
	actions/ActionHelpers.h
	actions/StandardActions.h
	Theme.h
	delegates/ItemDelegateLength.h
	delegates/MimeTypeDelegate.h
	delegates/BoldRowDelegate.h
	)

# gui source files
set(gui_SOURCE_FILES
	AboutBox.cpp
	CollectionDockWidget.cpp
	Experimental.cpp
	FilterWidget.cpp
	MainWindow.cpp
	MDIArea.cpp
	MDILibraryView.cpp
	MDIPlaylistView.cpp
	MDITreeViewBase.cpp
	MetadataDockWidget.cpp
	NetworkAwareFileDialog.cpp
	MDINowPlayingView.cpp
	MDINowPlayingView.h
	DragDropTreeViewStyleProxy.cpp
	helpers/Tips.cpp
	menus/DropMenu.cpp
	menus/LibraryContextMenu.cpp
	menus/PlaylistContextMenuViewport.cpp
	menus/PlaylistContextMenu.cpp
	menus/ActionBundle.cpp
	mdi/MDIModelViewPair.cpp
	menus/HelpMenu.cpp
	helpers/NetAwareFileDialogGtk3.cpp
	helpers/Gtk3DialogHelper.cpp
	helpers/IfExistsAskForOverwrite.cpp
	actions/StandardActions.cpp
	Theme.cpp
	delegates/ItemDelegateLength.cpp
	delegates/MimeTypeDelegate.cpp
	delegates/BoldRowDelegate.cpp
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
	SettingsPageDatabase.h
	SettingsPageDatabase.cpp
	SettingsPageDatabase.ui
	SettingsPageLibrary.ui
	SettingsDialog.h
	SettingsDialog.cpp
	)
PREPEND_TO_EACH(gui_settings_SOURCE_FILES "settings/" ${gui_settings_SOURCE_FILES})

add_subdirectory(activityprogressmanager)
add_subdirectory(widgets)

add_library(gui STATIC EXCLUDE_FROM_ALL)
target_sources(gui
	PRIVATE
		${gui_HEADER_FILES}
		${gui_settings_SOURCE_FILES}
		${HEADER_FILES_GUI_ACTIVITYPROGRESSMANAGER}
        ${gui_SOURCE_FILES}
		${SOURCE_FILES_GUI_ACTIVITYPROGRESSMANAGER}
		${UI_FILES_GUI_ACTIVITYPROGRESSMANAGER}
		)
target_include_directories(gui
	PRIVATE
		# For config.h
		${PROJECT_BINARY_DIR}/src
		# For anything in <root>/src: utils, logic, future, gui.
		${PROJECT_SOURCE_DIR}/src
		${QtGui_PRIVATE_INCLUDE_DIRS} ### @todo OBSOLETE THIS
)
set_target_properties(gui
    PROPERTIES
    	AUTOMOC ON
    	AUTOUIC ON
    	AUTORCC ON
    	)
target_link_libraries(gui
	PUBLIC
		kconfig_target
		${QT_LINK_LIB_TARGETS}
		${KF_LINK_LIB_TARGETS}
		${GTK_LINK_LIB_TARGETS}
		utils
	PRIVATE
		widgets
		resources
)

