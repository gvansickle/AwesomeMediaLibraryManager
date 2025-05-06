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

# @file src/utils/CMakeLists.txt.

set(utils_HEADER_FILES
	ConnectHelpers.h
	DebugBlock.h
	DebugHelpers.h
	Fraction.h
	in.h
	MapConverter.h
	StringHelpers.h
	RegisterQtMetatypes.h
	UniqueIDMixin.h
	QtCastHelpers.h
	TheSimplestThings.h
	Logging.h
	AboutDataSetup.h
	QtHelpers.h
	Stopwatch.h
	VectorHelpers.h
	EnumFlagHelpers.h
	ext_iterators.h
)
set(utils_SOURCE_FILES
	ConnectHelpers.cpp
	DebugBlock.cpp
	DebugHelpers.cpp
	ext_iterators.cpp
	Fraction.cpp
	MapConverter.cpp
	StringHelpers.cpp
	RegisterQtMetatypes.cpp
	UniqueIDMixin.h
	Logging.cpp
	AboutDataSetup.cpp
	QtHelpers.cpp
	Stopwatch.cpp
	VectorHelpers.cpp
)

add_library(utils STATIC EXCLUDE_FROM_ALL)
target_sources(utils
	PRIVATE
		${utils_HEADER_FILES}
		${utils_SOURCE_FILES}
		)
target_include_directories(utils
	PRIVATE
		# For config.h
		${PROJECT_BINARY_DIR}/src
		# For anything in <root>/src: utils, logic, future, gui.
		${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(utils
	PUBLIC
		${QT_LINK_LIB_TARGETS}
		${KF_LINK_LIB_TARGETS}
		${GTK_LINK_LIB_TARGETS}
	PRIVATE
		future
		resources	
		)

