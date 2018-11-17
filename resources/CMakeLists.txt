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
prepend_to_each(amlm_ICONS "${CMAKE_CURRENT_LIST_DIR}/" ${amlm_ICONS})

set(amlm_XQUERIES
	xquery_files.qrc
	filelist.xq
)
prepend_to_each(amlm_XQUERIES "${CMAKE_CURRENT_LIST_DIR}/" ${amlm_XQUERIES})

target_sources(resources_subdir 
	PUBLIC
		${amlm_ICONS}
		${amlm_XQUERIES}
)

# Qt5 Binary Resource (rcc) files.
#qt5_add_binary_resources(icons_oxygen oxygenicons.qrc)

# Qt5 non-binary Resource files.
#qt5_add_resources(amlm_SRCS "AppIcon.qrc")
#qt5_add_resources(amlm_SRCS "amlm.qrc")

#target_sources(amlm_pseudolib
#	PUBLIC
#	${CMAKE_CURRENT_LIST_DIR}/AppIcon.qrc
#	${CMAKE_CURRENT_LIST_DIR}/amlm.qrc
#	)

# Default bundled icons.
#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/icons_oxygen.rcc
#	    DESTINATION ${KDE_INSTALL_ICONDIR}
#		COMPONENT coreapp)

