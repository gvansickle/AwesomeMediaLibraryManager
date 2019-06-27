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
	DebugBlock.cpp
	DebugHelpers.cpp
	ext_iterators.cpp
	Fraction.cpp
	MapConverter.cpp
	RegisterQtMetatypes.cpp
	UniqueIDMixin.h
	Logging.cpp
	AboutDataSetup.cpp
	QtHelpers.cpp
	Stopwatch.cpp
	VectorHelpers.cpp
)

target_sources_local(utils
	PUBLIC
		${utils_HEADER_FILES}
	PRIVATE
		${utils_SOURCE_FILES}
		)

target_link_libraries(utils
	PUBLIC
		${QT5_LINK_LIB_TARGETS}
		${KF5_LINK_LIB_TARGETS}
		${GTK_LINK_LIB_TARGETS}
	PRIVATE
		future
		resources	
		)

target_include_directories(utils
	PUBLIC
		# For config.h
		${PROJECT_BINARY_DIR}/src
	PUBLIC
		# For anything in <root>/src: utils, logic, future, gui.
		${PROJECT_SOURCE_DIR}/src
		)
