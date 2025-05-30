#
# Copyright 2018, 2024 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
		ISerializable.cpp
		SerializationExceptions.cpp
		SerializationHelpers.cpp
		XmlObjects.cpp
		XmlSerializer.cpp
		XSPFSerializer.cpp
		QVariantHomogenousList.cpp
		)
set(serialization_subdir_HEADERS
		ExtEnum.h
		SerializationExceptions.h
		SerializationHelpers.h
		XmlObjects.h
		XmlSerializer.h
		ISerializable.h
		ISerializer.h
		XSPFSerializer.h
		QVariantHomogenousList.h
		)

add_library(serialization STATIC EXCLUDE_FROM_ALL)
target_sources(serialization
	PRIVATE
		${serialization_subdir_HEADERS}
		${serialization_subdir_SOURCES}
	)

target_link_libraries(serialization
	PUBLIC
		${QT_LINK_LIB_TARGETS}
		${KF_LINK_LIB_TARGETS}
		${GTK_LINK_LIB_TARGETS}

)
target_include_directories(serialization
	PRIVATE
	    # For config.h
		${PROJECT_BINARY_DIR}/src
		${PROJECT_SOURCE_DIR}/src
	)
