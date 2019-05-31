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

# @file src/logic/CMakeLists.txt

add_subdir_lib(dbmodels EXCLUDE_FROM_ALL)
add_subdir_lib(jobs EXCLUDE_FROM_ALL)
add_subdir_lib(models EXCLUDE_FROM_ALL)
add_subdir_lib(proxymodels EXCLUDE_FROM_ALL)
add_subdir_lib(serialization EXCLUDE_FROM_ALL)

set(logic_subdir_SOURCES
	AMLMTagMap.cpp
	AudioFileType.cpp
	Library.cpp
	LibraryEntry.cpp
	LibraryEntryMimeData.cpp
	LibraryModel.cpp
	Metadata.cpp
	MetadataAbstractBase.cpp
	MetadataFromCache.cpp
	MetadataTaglib.cpp
	MP2.cpp
	ntp.cpp
	PlaylistModel.cpp
	PlaylistModelItem.cpp
	PlaylistSortFilterProxyModel.cpp
	SupportedMimeTypes.cpp
	TrackMetadata.cpp
	CueSheetParser.cpp
	CueSheet.cpp
	ModelUserRoles.cpp
	ColumnSpec.cpp
	LibraryRescannerMapItem.cpp
	DirScanResult.cpp
	ExtMimeType.cpp
	LibraryRescanner.cpp
	ExtUrl.cpp
	PerfectDeleter.cpp
	Frames.cpp
	)

set(logic_subdir_HEADERS
	AMLMTagMap.h
	AudioFileType.h
	Library.h
	LibraryEntry.h
	LibraryEntryMimeData.h
	LibraryModel.h
	Metadata.h
	MetadataAbstractBase.h
	MetadataFromCache.h
	MetadataTaglib.h
	MP2.h
	ntp.h
	PlaylistModel.h
	PlaylistModelItem.h
	PlaylistSortFilterProxyModel.h
	SupportedMimeTypes.h
	TrackMetadata.h
	LibraryRescanner.h
	CueSheetParser.h
	CueSheet.h
	TagLibHelpers.h
	ModelUserRoles.h
	ColumnSpec.h
	LibraryRescannerMapItem.h
	DirScanResult.h
	ExtMimeType.h
	ExtUrl.h
	PerfectDeleter.h
	Frames.h
	)

target_sources_local(logic
	PUBLIC
		${logic_subdir_HEADERS}
	PRIVATE
		${logic_subdir_SOURCES}
	)
target_link_libraries(logic
	PUBLIC
	    #${PROJECT_COMMON_EXTERNAL_LINK_LIB_TARGETS} ### @todo "logic_subdir links to itself".
		${QT5_LINK_LIB_TARGETS}
		${KF5_LINK_LIB_TARGETS}
		${GTK_LINK_LIB_TARGETS}
		${AMLM_TAGLIB_TARGET}
		${AMLM_LIBCUE_TARGET}
		dbmodels
		jobs
		models
		proxymodels
		serialization
	)
target_include_directories(logic
	PUBLIC
	    # For config.h
		${PROJECT_BINARY_DIR}/src
	PRIVATE
		${PROJECT_SOURCE_DIR}/src
	)



