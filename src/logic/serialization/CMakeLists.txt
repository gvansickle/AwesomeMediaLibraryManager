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

# @file src/logic/serialization/CMakeLists.txt

set(serialization_subdir_SOURCES
		ExtEnum.cpp
		XmlObjects.cpp
		XmlSerializer.cpp
		XSPFSerializer.cpp
		)
set(serialization_subdir_HEADERS
		ExtEnum.h
		XmlObjects.h
		XmlSerializer.h
		ISerializable.h
		ISerializer.h
		XSPFSerializer.h
		)

prepend_to_each(serialization_subdir_SOURCES "${CMAKE_CURRENT_LIST_DIR}/" ${serialization_subdir_SOURCES})
prepend_to_each(serialization_subdir_HEADERS "${CMAKE_CURRENT_LIST_DIR}/" ${serialization_subdir_HEADERS})

target_sources(serialization_subdir
	PUBLIC
		${serialization_subdir_HEADERS}
	PRIVATE
		${serialization_subdir_SOURCES}
	)

target_link_libraries(serialization_subdir
	PUBLIC
		${QT5_LINK_LIB_TARGETS}
		${KF5_LINK_LIB_TARGETS}
		${GTK_LINK_LIB_TARGETS}

)
target_include_directories(serialization_subdir
	PUBLIC
	    # For config.h
		${PROJECT_BINARY_DIR}/src
	)
