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

# Application icons.
### @todo Make our own icon.
set(amlm_ICONS
  48-preferences-desktop-sound.png
  64-preferences-desktop-sound.png
  128-preferences-desktop-sound.png
  scalable-preferences-desktop-sound.svgz
)

ecm_install_icons(ICONS ${amlm_ICONS} DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)

# Qt5 Binary Resource (rcc) files.
qt5_add_binary_resources(icons_oxygen oxygenicons.qrc)

# Qt5 non-binary Resource files.
qt5_add_resources(amlm_SRCS "AppIcon.qrc" PARENT_SCOPE)
qt5_add_resources(amlm_SRCS "amlm.qrc" PARENT_SCOPE)

# Default bundled icons.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/icons_oxygen.rcc
	    DESTINATION ${KDE_INSTALL_ICONDIR}
		COMPONENT coreapp)
###
### KF5 icon theme install for especially Windows and Mac.
###
### Per the docs @link https://api.kde.org/frameworks/kiconthemes/html/index.html, as long as icontheme.rcc
### is installed "in a directory found by QStandardPaths::AppDataLocation", the KDE QPA should find it.
### I'm not sure that actually works.  Even on Linux, it seems that "icontheme.rcc" apparently has to be in
### ${KDE_INSTALL_BINDIR}/data/ (bin/data/), and not in any of the actual QStandardPaths::AppDataLocation dirs
### (Linux: "~/.local/share/<APPNAME>", "/usr/local/share/<APPNAME>", "/usr/share/<APPNAME>") for it to be picked up.
###
### @link https://blogs.kde.org/2016/06/16/icon-theme-deployment-windows-mac-os-and-mobile-platforms
if(TRUE)#WIN32)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/icons_oxygen.rcc
		DESTINATION ${KDE_INSTALL_ICONDIR}
		RENAME "icontheme.rcc"
		COMPONENT coreapp)
endif()
