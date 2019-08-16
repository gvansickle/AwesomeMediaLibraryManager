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

# @file src/logic/jobs/CMakeLists.txt

set(jobs_SOURCE_FILES
		CoverArtJob.cpp
		DatabaseScanJob.cpp
		DirectoryScanJob.cpp
		LibraryEntryLoaderJob.cpp
		LibraryRescannerJob.cpp
	)
set(jobs_HEADER_FILES
		CoverArtJob.h
		DatabaseScanJob.h
		DirectoryScanJob.h
		LibraryEntryLoaderJob.h
		LibraryRescannerJob.h
	)

target_sources_local(jobs
	PUBLIC
		${jobs_HEADER_FILES}
	PRIVATE
		${jobs_SOURCE_FILES}
	)

target_link_libraries(jobs
	PUBLIC
	    #${PROJECT_COMMON_EXTERNAL_LINK_LIB_TARGETS}
		${QT5_LINK_LIB_TARGETS}
		${KF5_LINK_LIB_TARGETS}
    PRIVATE
		${AMLM_TAGLIB_TARGET}
		concurrency
		)
target_include_directories(jobs
	PUBLIC
	    # For config.h
		${PROJECT_BINARY_DIR}/src
	PRIVATE
		# For anything under src: utils/, logic/, etc.
		${PROJECT_SOURCE_DIR}/src
	)
