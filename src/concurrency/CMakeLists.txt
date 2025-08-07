#
# Copyright 2018, 2024, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

# @file src/concurrency/CMakeLists.txt

# Concurrency files.
set(HEADER_FILES_UTILS_CONCURRENCY
	AsyncTaskManager.h
	ExtFuture.h
	ExtFutureProgressInfo.h
	AMLMJob.h
	AMLMJobT.h
	AMLMCompositeJob.h
	ThreadsafeMap.h
	WorkerThreadBase.h
	WorkerThreadControllerBase.h
	)
set(PRIVATE_HEADER_FILES_UTILS_CONCURRENCY
	)

set(SOURCE_FILES_UTILS_CONCURRENCY
	ExtFuture.cpp
	ExtFutureProgressInfo.cpp
	AMLMJob.cpp
	AMLMJobT.cpp
	AMLMCompositeJob.cpp
	ThreadsafeMap.cpp
	WorkerThreadBase.cpp
	WorkerThreadControllerBase.cpp
	)

add_library(concurrency STATIC EXCLUDE_FROM_ALL)
target_sources(concurrency
	PRIVATE
		${HEADER_FILES_UTILS_CONCURRENCY}
		${PRIVATE_HEADER_FILES_UTILS_CONCURRENCY}
		${SOURCE_FILES_UTILS_CONCURRENCY}
		)
target_include_directories(concurrency
	PUBLIC
		# For config.h
		${PROJECT_BINARY_DIR}/src
	PRIVATE
		# For anything in <root>/src: utils, logic, future, gui.
		${PROJECT_SOURCE_DIR}/src
		${PROJECT_SOURCE_DIR}/src/logic
		${PROJECT_SOURCE_DIR}/src/logic/models
)
target_link_libraries(concurrency
	PRIVATE
		${QT_LINK_LIB_TARGETS}
		${KF_LINK_LIB_TARGETS}
#		jobs  ### @note For LibraryEntryLoaderJob.h via LibraryModel.h.
)

