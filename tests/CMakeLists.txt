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

add_library(tests_subdir STATIC
	EXCLUDE_FROM_ALL
	# Actual tests
#	FlagsAndEnumsTests.cpp
#	FlagsAndEnumsTests.h
	# Support code
	IResultsSequenceMock.cpp
	IResultsSequenceMock.h
	TestLifecycleManager.cpp
	TestLifecycleManager.h
	TestHelpers.h
	)

target_link_libraries(tests_subdir
	PUBLIC
		gtest
		gmock
)

#target_link_libraries(tests_subdir
#	PUBLIC
#	    #${PROJECT_COMMON_EXTERNAL_LINK_LIB_TARGETS} ### @todo "logic_subdir links to itself".
#		${QT5_LINK_LIB_TARGETS}
#		${KF5_LINK_LIB_TARGETS}
#		${GTK_LINK_LIB_TARGETS}
#		${AMLM_TAGLIB_TARGET}
#		${AMLM_LIBCUE_TARGET}
#		dbmodels_subdir
#		models_subdir
#		proxymodels_subdir
#		xml_subdir
#		)
		
target_include_directories(tests_subdir
	PRIVATE
	    # For config.h
		${PROJECT_BINARY_DIR}/src
	)
