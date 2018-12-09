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

# @file concurrency/CMakeLists.txt

# Concurrency files.
set(HEADER_FILES_UTILS_CONCURRENCY
	AsyncTaskManager.h
	ExtFutureState.h
	ExtFuture.h
	ExtFutureProgressInfo.h
	ExtFuturePropagationHandler.h
	ExtFutureWatcher.h
	ExtAsync_traits.h
	IExtFutureWatcher.h
	impl/ExtAsync_impl.h
	impl/ExtFuture_impl.hpp
	impl/ExtFutureImplHelpers.h
	ExtAsync.h
	ExtAsyncExceptions.h
	AMLMJob.h
	AMLMJobT.h
	AMLMCompositeJob.h
		../logic/jobs/DirectoryScanJob.h
	ThreadsafeMap.h
	DebugSequence.h
	WorkerThreadBase.h
	)
set(SOURCE_FILES_UTILS_CONCURRENCY
	impl/ExtFutureImplHelpers.cpp
	ExtAsync.cpp
	ExtAsyncExceptions.cpp
	ExtFuture.cpp
	ExtFutureProgressInfo.cpp
	ExtFuturePropagationHandler.cpp
	IExtFutureWatcher.cpp
	AMLMJob.cpp
	AMLMJobT.cpp
	AMLMCompositeJob.cpp
		../logic/jobs/DirectoryScanJob.cpp
	ThreadsafeMap.cpp
	DebugSequence.cpp
	WorkerThreadBase.cpp
	FunctionWrapper.cpp
	FunctionWrapper.h
	WorkStealingQueue.cpp
	WorkStealingQueue.h
	ThreadPool2.cpp
	ThreadPool2.h
	)

#list(TRANSFORM SOURCE_FILES_UTILS_CONCURRENCY PREPEND "${CMAKE_CURRENT_LIST_DIR}/")
#prepend_to_each(SOURCE_FILES_UTILS_CONCURRENCY "${CMAKE_CURRENT_LIST_DIR}/" ${SOURCE_FILES_UTILS_CONCURRENCY})
#prepend_to_each(HEADER_FILES_UTILS_CONCURRENCY "${CMAKE_CURRENT_LIST_DIR}/" ${HEADER_FILES_UTILS_CONCURRENCY})

target_sources_local(concurrency
	PUBLIC
		${HEADER_FILES_UTILS_CONCURRENCY}
	PRIVATE
		${SOURCE_FILES_UTILS_CONCURRENCY}
		)
target_link_libraries(concurrency
	PUBLIC
		#${PROJECT_COMMON_EXTERNAL_LINK_LIB_TARGETS} ### @todo "logic_subdir links to itself".
		${QT5_LINK_LIB_TARGETS}
		${KF5_LINK_LIB_TARGETS}
		${GTK_LINK_LIB_TARGETS}
		)
target_include_directories(concurrency
	PUBLIC
		# For config.h
		${PROJECT_BINARY_DIR}/src
	PUBLIC
		# For anything in <root>/src: utils, logic, future, gui.
		${PROJECT_SOURCE_DIR}/src
		)