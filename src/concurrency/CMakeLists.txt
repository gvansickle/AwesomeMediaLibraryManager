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
	ExtFutureState.h
	ExtFuture.h
	ExtFutureProgressInfo.h
	ExtFuturePropagationHandler.h
	ExtFutureWatcher.h
	ExtFuture_traits.h
	IExtFutureWatcher.h
	ExtAsync.h
	ExtAsyncExceptions.h
	AMLMJob.h
	AMLMJobT.h
	AMLMCompositeJob.h
	ThreadsafeMap.h
	DebugSequence.h
	WorkerThreadBase.h
	WorkerThreadControllerBase.h
	)
set(PRIVATE_HEADER_FILES_UTILS_CONCURRENCY
	impl/IExecutor.h
	impl/ExtAsync_impl.h
	impl/ExtFuture_impl.hpp
	impl/ExtFuture_make_xxx_future.h
	impl/ExtFuture_when_all.h
	impl/ExtFuture_when_any.h
	impl/ExtFutureImplHelpers.h
	impl/ExtAsync_RunInThread.h
	impl/ManagedExtFutureWatcher_impl.h
	)

set(SOURCE_FILES_UTILS_CONCURRENCY
	impl/ExtFuture_when_all.cpp
	impl/ExtFuture_when_any.cpp
	impl/ExtFutureImplHelpers.cpp
	impl/ManagedExtFutureWatcher_impl.cpp
	ExtAsync.cpp
	ExtAsyncExceptions.cpp
	ExtFuture.cpp
	ExtFutureProgressInfo.cpp
	ExtFuturePropagationHandler.cpp
	IExtFutureWatcher.cpp
	AMLMJob.cpp
	AMLMJobT.cpp
	AMLMCompositeJob.cpp
	ThreadsafeMap.cpp
	DebugSequence.cpp
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

