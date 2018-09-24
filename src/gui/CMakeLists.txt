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
	actions/ActionHelpers.h
	actions/StandardActions.h
	Theme.h
	delegates/ItemDelegateLength.h
	delegates/MimeTypeDelegate.h
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
	actions/StandardActions.cpp
	Theme.cpp
	delegates/ItemDelegateLength.cpp
	delegates/MimeTypeDelegate.cpp
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
	SettingsDialog.cpp
	)
PREPEND_TO_EACH(gui_settings_SOURCE_FILES "settings/" ${gui_settings_SOURCE_FILES})

add_subdirectory(activityprogressmanager)

### @todo
#add_library(widgets "")
set(amlm_widget_SRCS "")
include(widgets/CMakeLists.txt)
#set_target_properties(widgets
#    PROPERTIES
#    AUTOMOC ON
#    AUTOUIC ON)
#target_link_libraries(widgets ${PROJECT_COMMON_LINK_LIBS})

set(amlm_gui_SRCS
	${gui_SOURCE_FILES}
	${gui_HEADER_FILES}
	${gui_settings_SOURCE_FILES}
	${HEADER_FILES_GUI_ACTIVITYPROGRESSMANAGER}
	${SOURCE_FILES_GUI_ACTIVITYPROGRESSMANAGER}
	${UI_FILES_GUI_ACTIVITYPROGRESSMANAGER}
	#${amlm_gui_widget_SRCS}
	#${HEADER_FILES_GUI_WIDGETS}
	#${SOURCE_FILES_GUI_WIDGETS}
	#${UI_FILES_GUI_WIDGETS}
	PARENT_SCOPE
	)

set(amlm_gui_widget_SRCS
 	${amlm_widget_SRCS}
 	PARENT_SCOPE
 	)

#message(STATUS "amlm_gui_SRCS: ${amlm_gui_SRCS}")


