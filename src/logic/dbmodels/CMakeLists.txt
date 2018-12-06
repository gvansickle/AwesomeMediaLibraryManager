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

# @file src/logic/dbmodels/CMakeLists.txt

set(dbmodels_subdir_SOURCES
	CollectionDatabaseModel.cpp
	CollectionDatabaseModel.h
	CollectionModel.cpp
	CollectionModel.h
	DBConnectionManager.cpp
	DBConnectionManager.h
	EnhancedAbstractListModel.cpp
	EnhancedAbstractListModel.h
	EnhancedAbstractTableModel.cpp
	EnhancedAbstractTableModel.h
	EntityBase.cpp
	EntityBase.h
	ScanResultsTableModel.cpp
	ScanResultsTableModel.h
		)
prepend_to_each(dbmodels_subdir_SOURCES "${CMAKE_CURRENT_LIST_DIR}/" ${dbmodels_subdir_SOURCES})

target_sources(dbmodels
	PUBLIC
		${dbmodels_subdir_SOURCES}
	)
target_link_libraries(dbmodels PUBLIC ${PROJECT_COMMON_EXTERNAL_LINK_LIB_TARGETS})
target_include_directories(dbmodels
	PUBLIC
#		${CMAKE_CURRENT_SOURCE_DIR}/.. # To allow <future/whatever.h>
		${CMAKE_CURRENT_LIST_DIR}
#	    ${CMAKE_CURRENT_LIST_DIR}/.. # To allow <future/whatever.h>
		${boost_callable_traits_SOURCE_DIR}
		${boost_callable_traits_BINARY_DIR}
		# For config.h
		${PROJECT_BINARY_DIR}/src
		)

		
set_target_properties(dbmodels
    PROPERTIES
    AUTOMOC ON
    AUTORCC OFF
    AUTOUIC OFF)
