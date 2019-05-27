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

# @file tests/CMakeLists.txt

### @todo This needs work.  Not sure atm how best to partition this.

# add_library(amlm_tests STATIC
# 	EXCLUDE_FROM_ALL
# 	)
target_sources_local(tests
	PRIVATE
		IResultsSequenceMock.cpp
		IResultsSequenceMock.h
		TestLifecycleManager.cpp
		TestLifecycleManager.h
		TestHelpers.h
	)
target_link_libraries(tests
	PRIVATE
		gtest
		gmock
	PRIVATE
		amlm_boost_settings_target
		#${Boost_LIBRARIES}
)

target_include_directories(tests
	PRIVATE
	    # For config.h
		${PROJECT_BINARY_DIR}/src
		${Qt5Gui_PRIVATE_INCLUDE_DIRS}
		${Boost_INCLUDE_DIRS}
	)
