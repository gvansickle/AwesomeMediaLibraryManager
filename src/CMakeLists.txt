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

# @file src/CMakeLists.txt

###
### The third-party packages we require.
###

set(HEADER_FILES_THIRD_PARTY "")
set(SOURCE_FILES_THIRD_PARTY "")

set(AMLM_LIBCUE_TARGET "")
set(AMLM_TAGLIB_TARGET "")

# Use the bundled version of libcue due to this issue: https://github.com/lipnitsk/libcue/commit/e60b7b01a40009c7c71c5e154a328881a6e9ae46
set(USE_BUNDLED_LIBCUE TRUE)


set(TAGLIB_MIN_VERSION "1.11.0")
find_package(Taglib)
set_package_properties(Taglib PROPERTIES TYPE REQUIRED PURPOSE "Required for reading media file metadata.")
if(TAGLIB_FOUND)
	add_library(taglib INTERFACE)
	set_property(TARGET taglib PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${TAGLIB_INCLUDES})
	set_property(TARGET taglib PROPERTY INTERFACE_LINK_LIBRARIES ${TAGLIB_LIBRARIES})
	set_property(TARGET taglib PROPERTY INTERFACE_CFLAGS ${TAGLIB_CFLAGS})
	### Can't do this if TARGET is INTERFACE.
	#set_property(TARGET ${AMLM_TAGLIB_TARGET} PROPERTY SKIP_AUTOGEN ON)
	get_target_property(TL_INTERFACE_INCLUDE_DIRECTORIES taglib INTERFACE_INCLUDE_DIRECTORIES)
	get_target_property(TL_INTERFACE_LINK_LIBRARIES taglib INTERFACE_LINK_LIBRARIES)
	get_target_property(TL_INTERFACE_CFLAGS taglib INTERFACE_CFLAGS)
	cmake_print_variables(TL_INTERFACE_INCLUDE_DIRECTORIES)
	cmake_print_variables(TL_INTERFACE_LINK_LIBRARIES)
	cmake_print_variables(TL_INTERFACE_CFLAGS)
	set(AMLM_TAGLIB_TARGET "taglib")
endif()


if(USE_BUNDLED_LIBCUE)
	# Use the in-tree copy of libcue.
	add_subdirectory("third_party"
		EXCLUDE_FROM_ALL  # Don't tie this lib directly to the ALL target.
		)

	set(AMLM_LIBCUE_TARGET "libcue")

#	print_target_properties(${AMLM_LIBCUE_TARGET})
else()
	# Use the system libcue.
	pkg_check_modules(LIBCUE REQUIRED libcue
		IMPORTED_TARGET)
	if(LIBCUE_FOUND)
		set(AMLM_LIBCUE_TARGET "PkgConfig::LIBCUE")
	endif()
endif()
# libcue doesn't want AUTOMOC etc. applied to it.
set_property(TARGET ${AMLM_LIBCUE_TARGET} PROPERTY SKIP_AUTOGEN ON)
get_property(AMLM_LIBCUE_TARGET_SRCS TARGET ${AMLM_LIBCUE_TARGET} PROPERTY SOURCES)
set_property(SOURCE ${AMLM_LIBCUE_TARGET_SRCS} PROPERTY SKIP_AUTOGEN ON)
#message(STATUS "AMLM_LIBCUE_TARGET:SOURCES: ${AMLM_LIBCUE_TARGET_SRCS}")

# Verdigris is just two headers.
set(USE_BUNDLED_VERDIGRIS 1)
set(INCLUDE_DIR_VERDIGRIS ${PROJECT_SOURCE_DIR}/src/third_party/verdigris/src)
add_library(verdigris_target INTERFACE)
target_include_directories(verdigris_target INTERFACE ${PROJECT_SOURCE_DIR}/src/third_party/verdigris/src)


# Make the KConfig settings file its own library, so dependencies get handled correctly.
kconfig_add_kcfg_files(amlm_settings_SRCS AwesomeMediaLibraryManagerSettings.kcfgc)
#message(STATUS "* amlm_settings_SRCS: ${amlm_settings_SRCS}")
# Exclude the generated KConfig settings source files from AUTOMOC etc.
set_property(SOURCE ${amlm_settings_SRCS} PROPERTY SKIP_AUTOGEN ON) 
### @todo Do we need this? Target to get the deps on generated files right.
###add_custom_target(AwesomeMediaLibraryManagerSettings_h DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/AwesomeMediaLibraryManagerSettings.h)

# Collect all the project-common external link library targets into a single variable.
set(INTERNAL_LINK_LIB_TARGETS amlm_src_future)
set(PROJECT_COMMON_EXTERNAL_LINK_LIB_TARGETS
		${QT5_LINK_LIB_TARGETS}
		${KF5_LINK_LIBS}
		${GTK_LINK_LIB_TARGETS}
		${AMLM_TAGLIB_TARGET}
		${AMLM_LIBCUE_TARGET}
		${AMLM_QXTGLOBALSHORTCUT_LIB_TARGET}
		callable_traits
		verdigris_target
		)


# Main program header files.
set(HEADER_FILES
	AMLMApp.h
	logic/Library.h
	logic/LibraryEntry.h
	logic/LibraryEntryLoaderJob.h
	logic/LibraryEntryMimeData.h
	logic/LibraryModel.h
	logic/proxymodels/LibrarySortFilterProxyModel.h
	logic/Metadata.h
	logic/MetadataAbstractBase.h
	logic/MetadataFromCache.h
	logic/MetadataTaglib.h
	logic/MP2.h
	logic/ntp.h
	logic/PlaylistModel.h
	logic/PlaylistModelItem.h
	logic/PlaylistSortFilterProxyModel.h
	logic/SupportedMimeTypes.h
	logic/TrackMetadata.h
	logic/dbmodels/CollectionDatabaseModel.h
	logic/dbmodels/DBConnectionManager.h
	logic/dbmodels/EnhancedAbstractListModel.h
	logic/dbmodels/EnhancedAbstractTableModel.h
	logic/dbmodels/EntityBase.h
	logic/dbmodels/ScanResultsTableModel.h
	logic/dbmodels/SQLHelpers.h
	logic/models/ScanResultsTreeModel.h
	utils/ConnectHelpers.h
	utils/DebugBlock.h
	utils/DebugHelpers.h
	utils/Fraction.h
	utils/in.h
	utils/MapConverter.h
	utils/StringHelpers.h
	utils/WorkerThreadBase.h
	logic/LibraryRescanner.h
	utils/AsyncDirScanner.h
	utils/RegisterQtMetatypes.h
	utils/UniqueIDMixin.h
	utils/QtCastHelpers.h
	utils/TheSimplestThings.h
	logic/CueSheetParser.h
	logic/CueSheet.h
	logic/CoverArtJob.h
	logic/TagLibHelpers.h
	${PROJECT_SOURCE_DIR}/resources/VersionInfo.h    ### @todo
	logic/ModelUserRoles.h
	logic/proxymodels/ModelHelpers.h
	logic/proxymodels/SelectionFilterProxyModel.h
	logic/proxymodels/ModelChangeWatcher.h
	logic/ColumnSpec.h
	logic/LibraryRescannerMapItem.h
	logic/LibraryRescannerJob.h
	logic/proxymodels/QPersistentModelIndexVec.h
	logic/DirScanResult.h
	logic/ExtMimeType.h
	utils/Logging.h
	utils/AboutDataSetup.h
	utils/QtHelpers.h
    )

# Source file list.
set(SOURCE_FILES
	AMLMApp.cpp
	logic/Library.cpp
	logic/LibraryEntry.cpp
	logic/LibraryEntryLoaderJob.cpp
	logic/LibraryEntryMimeData.cpp
	logic/LibraryModel.cpp
	logic/proxymodels/LibrarySortFilterProxyModel.cpp
	logic/Metadata.cpp
	logic/MetadataAbstractBase.cpp
	logic/MetadataFromCache.cpp
	logic/MetadataTaglib.cpp
	logic/MP2.cpp
	logic/ntp.cpp
	logic/PlaylistModel.cpp
	logic/PlaylistModelItem.cpp
	logic/PlaylistSortFilterProxyModel.cpp
	logic/SupportedMimeTypes.cpp
	logic/TrackMetadata.cpp
	logic/dbmodels/CollectionDatabaseModel.cpp
	logic/dbmodels/DBConnectionManager.cpp
	logic/dbmodels/EnhancedAbstractListModel.cpp
	logic/dbmodels/EnhancedAbstractTableModel.cpp
	logic/dbmodels/ScanResultsTableModel.cpp
	logic/dbmodels/EntityBase.cpp
	logic/models/ScanResultsTreeModel.cpp
	utils/DebugBlock.cpp
	utils/Fraction.cpp
	utils/MapConverter.cpp
	utils/WorkerThreadBase.cpp
	logic/LibraryRescanner.cpp
	utils/AsyncDirScanner.cpp
	utils/RegisterQtMetatypes.cpp
	utils/UniqueIDMixin.h
	logic/CueSheetParser.cpp
	logic/CueSheet.cpp
	logic/CoverArtJob.cpp
	logic/ModelUserRoles.cpp
	logic/proxymodels/SelectionFilterProxyModel.cpp
	logic/proxymodels/ModelChangeWatcher.cpp
	logic/ColumnSpec.cpp
	logic/LibraryRescannerMapItem.cpp
	logic/LibraryRescannerJob.cpp
	logic/proxymodels/QPersistentModelIndexVec.cpp
	logic/DirScanResult.cpp
	logic/ExtMimeType.cpp
	utils/Logging.cpp
	utils/AboutDataSetup.cpp
	utils/QtHelpers.cpp
	)

# NoMoc files.
set(HEADER_FILES_UTILS_NOMOC
	utils/nomoc/nomocdefs.h
	utils/nomoc/nomocimpl.h)
list(APPEND HEADER_FILES ${HEADER_FILES_UTILS_NOMOC})

# Concurrency files.
set(HEADER_FILES_UTILS_CONCURRENCY
	concurrency/ReportingRunner.h
	concurrency/AsyncTaskManager.h
	concurrency/ExtendedDeferred.h
	concurrency/ExtFutureState.h
	concurrency/ExtFuture.h
	concurrency/ExtFutureWatcher.h
	concurrency/ExtAsync_traits.h
	concurrency/impl/ExtFuture_impl.hpp
	concurrency/ExtAsync.h
	concurrency/ExtAsyncTask.h
	concurrency/AMLMJob.h
	concurrency/AMLMCompositeJob.h
	concurrency/DirectoryScanJob.h
	concurrency/ThreadsafeMap.h
	concurrency/DebugSequence.h
	)
set(SOURCE_FILES_UTILS_CONCURRENCY
	concurrency/ExtAsync.cpp
	concurrency/AMLMJob.cpp
	concurrency/AMLMCompositeJob.cpp
	concurrency/ReportingRunner.cpp
	concurrency/DirectoryScanJob.cpp
	concurrency/ThreadsafeMap.cpp
	concurrency/DebugSequence.cpp
	
	# From Qt Creator
	#concurrency/runextensions.cpp
	#concurrency/runextensions.h
	)

set(SOURCE_FILES_LOGIC_MODELS
	logic/models/AbstractTreeModelItem.h
	logic/models/AbstractTreeModelItem.cpp
	logic/models/AbstractTreeModel.h
	logic/models/AbstractTreeModel.cpp
	)

add_subdirectory(gui)
#message(STATUS "amlm_gui_SRCS: ${amlm_gui_SRCS}")

#
# Append all the SOURCE_* subsets.
#

# *.ui files.
### @todo Remove? list(APPEND SOURCE_FILES ${SOURCE_FILES_UI})

# The Future
add_library(amlm_src_future EXCLUDE_FROM_ALL "")
include(future/CMakeLists.txt)

# Concurrency
list(APPEND HEADER_FILES ${HEADER_FILES_UTILS_CONCURRENCY})
list(APPEND SOURCE_FILES ${SOURCE_FILES_UTILS_CONCURRENCY})

# Models
list(APPEND SOURCE_FILES ${SOURCE_FILES_LOGIC_MODELS})

# Append the third_party files which need to be explicitly in the SOURCE_/HEADER_FILES vars.
list(APPEND HEADER_FILES ${HEADER_FILES_THIRD_PARTY})
list(APPEND SOURCE_FILES ${SOURCE_FILES_THIRD_PARTY})

# From gui/.
# @todo Make this a real target.
list(APPEND HEADER_FILES ${HEADER_FILES_GUI_ACTIVITYPROGRESSMANAGER})
list(APPEND SOURCE_FILES ${SOURCE_FILES_GUI_ACTIVITYPROGRESSMANAGER} ${UI_FILES_GUI_ACTIVITYPROGRESSMANAGER})
prepend_to_each(amlm_gui_SRCS "gui/" ${amlm_gui_SRCS}) ### @todo Make this a real target.

# Build the ultimate list of lists.
list(APPEND amlm_SRCS ${SOURCE_FILES} ${HEADER_FILES}
	${amlm_gui_SRCS} ### @todo Make this a real target.
	${amlm_gui_widget_SRCS} ### @todo
#	${amlm_src_future_SRCS}
	)

# Sets the icon on Windows and OSX.
# See https://api.kde.org/ecm/module/ECMAddAppIcon.html
file(GLOB ICONS_SRCS "${PROJECT_SOURCE_DIR}/resources/*-preferences-desktop-sound.png")
ecm_add_app_icon(amlm_SRCS ICONS ${ICONS_SRCS})


# Qt5 Binary Resource (rcc) files.
qt5_add_binary_resources(icons_oxygen "${PROJECT_SOURCE_DIR}/resources/oxygenicons.qrc")
#set(AppIcon_qrc "${CMAKE_CURRENT_SOURCE_DIR}/../resources/AppIcon.qrc")
#set(AppIcon_rcc_dir "${CMAKE_CURRENT_BINARY_DIR}/../bin")
#set(AppIcon_rcc "${AppIcon_rcc_dir}/AppIcon.rcc")
# Set up the app icon for Linux, add a dep so it gets built.
#qt5_add_binary_resources(build_AppIcon_rcc ${AppIcon_qrc} DESTINATION ${AppIcon_rcc})

# Qt5 non-binary Resource files.
qt5_add_resources(amlm_SRCS "${PROJECT_SOURCE_DIR}/resources/AppIcon.qrc")
qt5_add_resources(amlm_SRCS "${PROJECT_SOURCE_DIR}/resources/amlm.qrc")

# The link lib targets common to app and test builds.
set(PROJECT_COMMON_LINK_LIBS
		${PROJECT_COMMON_EXTERNAL_LINK_LIB_TARGETS}
		)

# WIN32_OR_MACOS_BUNDLE variable for add_executable().  Expands to WIN32 or MACOS_BUNDLE depending on OS
if (APPLE)
	set(WIN32_OR_MACOS_BUNDLE MACOSX_BUNDLE)
elseif (WIN32)
    set(WIN32_OR_MACOS_BUNDLE WIN32)
endif()


###
### The main executable.
###
### @note Including HEADER_FILES in here so they get checked for existence.
###
add_executable(${PROJECT_NAME} ${WIN32_OR_MACOS_BUNDLE}
		main.cpp
		${amlm_SRCS} #${SOURCE_FILES} ${HEADER_FILES} ${META_FILES_TO_INCLUDE} ${RESOURCE_FILES}
		${amlm_settings_SRCS}
		${PROJECT_BINARY_DIR}/resources/VersionInfo.cpp
	)
# Compiler features FBO Verdigris (esp. on Windows/MSVC).  -D__cpp_constexpr=201304 -D__cpp_variable_templates=201304.
### @todo No longer required?
#target_compile_features(${PROJECT_NAME} PRIVATE cxx_relaxed_constexpr cxx_variable_templates)
target_compile_definitions(${PROJECT_NAME}
	PUBLIC
	"QT_SHAREDPOINTER_TRACK_POINTERS=1"
	PRIVATE
	"KXMLGUI_NO_DEPRECATED=1"
	"QT_DEPRECATED_WARNINGS=1"
	"QT_NO_KEYWORDS=1"
	"USE_BUNDLED_VERDIGRIS=${USE_BUNDLED_VERDIGRIS}"
	)
target_include_directories(${PROJECT_NAME} PRIVATE "utils/nomoc" ${INCLUDE_DIR_VERDIGRIS})
target_compile_options(${PROJECT_NAME} PRIVATE ${EXTRA_CXX_COMPILE_FLAGS})
# Only the Qt stuff needs AUTOMOC.
### This doesn't work: set_property(FILES ${SOURCE_FILES} ${HEADER_FILES} PROPERTY AUTOMOC ON)
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    AUTOMOC ON
    AUTOUIC ON)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_COMMON_LINK_LIBS})
add_dependencies(${PROJECT_NAME} icons_oxygen)

# Stop clang-tidy from scanning generated files.
file(WRITE "${PROJECT_BINARY_DIR}/.clang-tidy" "
---
Checks: '-*,llvm-twine-local'
...
")

#print_target_properties(${PROJECT_NAME})

###
### Testing
###
if(TRUE) # Enable test target.

find_package(Qt5Test REQUIRED)
include(CTest)

# @note We don't really need to do this, but will for expediency.  Per the docs:
# "CMake will generate tests only if the enable_testing() command has been invoked.
# The CTest module invokes the command automatically when the BUILD_TESTING option is ON."
# @todo Also per the docs, this has to be in the source directory root, but it seems to work here:
# "Note that ctest expects to find a test file in the build directory root.
# Therefore, this command should be in the source directory root."
enable_testing()

###
### Google Test
###
find_package(Threads REQUIRED)
download_project(PROJ                googletest
                 GIT_REPOSITORY      https://github.com/google/googletest.git
                 GIT_TAG             master
                 ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)
# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# Add googletest directly to our build. This adds
# the following targets: gtest, gtest_main, gmock
# and gmock_main.
add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR}
	EXCLUDE_FROM_ALL)
# Now we should be able to link our targets against gtest, gmock, etc. as appropriate
message(STATUS "GOOGLETEST-------------------------------")
print_varlist(googletest_SOURCE_DIR googletest_BINARY_DIR)
message(STATUS "GOOGLETEST-------------------------------")

###
### The "alltests" Google Test-based test exe.
###
set(TEST_EXE_TARGET alltests)
set(AMLM_SOURCE_FILES_TEST "")
set(AMLM_HEADER_FILES_TEST "")
list(APPEND AMLM_SOURCE_FILES_TEST
#	${PROJECT_SOURCE_DIR}/tests/TestLifecycleManager.cpp
	concurrency/tests/ExtAsyncTaskTests.cpp
	concurrency/tests/ExtAsyncTests.cpp
	concurrency/tests/ExtAsyncTestCommon.cpp
	concurrency/tests/ExtFutureTests.cpp
	concurrency/tests/AMLMJobTests.cpp)
list(APPEND AMLM_HEADER_FILES_TEST
#	${PROJECT_SOURCE_DIR}/tests/TestHelpers.h
#	${PROJECT_SOURCE_DIR}/tests/TestLifecycleManager.h
	concurrency/tests/ExtAsyncTaskTests.h
	concurrency/tests/ExtAsyncTests.h
	concurrency/tests/ExtAsyncTestCommon.h
	concurrency/tests/ExtFutureTests.h
	concurrency/tests/AMLMJobTests.h)

add_executable(${TEST_EXE_TARGET} ${WIN32_OR_MACOS_BUNDLE} "")
target_sources(${TEST_EXE_TARGET}
	PRIVATE
	    alltests.cpp ${AMLM_SOURCE_FILES_TEST} ${AMLM_HEADER_FILES_TEST}
		${amlm_SRCS} ### @todo ${SOURCE_FILES} ${HEADER_FILES} ${META_FILES_TO_INCLUDE} ${RESOURCE_FILES}
		${amlm_settings_SRCS}
		${PROJECT_BINARY_DIR}/resources/VersionInfo.cpp
	)
#target_compile_features(${TEST_EXE_TARGET} PRIVATE cxx_relaxed_constexpr cxx_variable_templates)
target_compile_definitions(${TEST_EXE_TARGET} PRIVATE
	"QT_SHAREDPOINTER_TRACK_POINTERS=1"
	"QT_DEPRECATED_WARNINGS"
	"QT_NO_KEYWORDS"
	"USE_BUNDLED_VERDIGRIS=${USE_BUNDLED_VERDIGRIS}"
	"TEST_FWK_IS_GTEST=1"
	)
target_include_directories(${TEST_EXE_TARGET} PRIVATE "utils/nomoc" ${INCLUDE_DIR_VERDIGRIS})
target_compile_definitions(${TEST_EXE_TARGET} PRIVATE "GTEST_LANGUAGE_CXX11")
target_compile_options(${TEST_EXE_TARGET} PRIVATE ${EXTRA_CXX_COMPILE_FLAGS})
set_target_properties(${TEST_EXE_TARGET} PROPERTIES
	AUTOMOC ON
	AUTOUIC ON)
# We don't want AUTOMOC etc. applied to googletest.
set_property(DIRECTORY ${googletest_SOURCE_DIR}
	PROPERTY SKIP_AUTOGEN ON
	EXCLUDE_FROM_ALL)
set_property(TARGET gtest PROPERTY SKIP_AUTOGEN ON)
set_property(TARGET gmock PROPERTY SKIP_AUTOGEN ON)
set_property(TARGET gtest_main PROPERTY SKIP_AUTOGEN ON)
set_property(TARGET gmock_main PROPERTY SKIP_AUTOGEN ON)
target_link_libraries(${TEST_EXE_TARGET} Threads::Threads gtest gmock tests_subdir)
target_link_libraries(${TEST_EXE_TARGET} Qt5::Test ${PROJECT_COMMON_LINK_LIBS})
add_test(NAME ${TEST_EXE_TARGET} COMMAND $<TARGET_FILE:${TEST_EXE_TARGET}>)
ecm_mark_as_test(${TEST_EXE_TARGET})

if(TRUE)
###
### Qt5 QTests
###
#set(test_qtestamlmjobtests_SRCS concurrency/tests/QTestAMLMJobTests.cpp)
add_executable(test_qtestamlmjobtests "") #${test_qtestamlmjobtests_SRCS} ${} ${amlm_settings_SRCS})
target_sources(test_qtestamlmjobtests
	PRIVATE
	    concurrency/tests/QTestAMLMJobTests.cpp
	    concurrency/tests/ExtAsyncTestCommon.cpp
	    concurrency/tests/ExtAsyncTestCommon.h
		#${AMLM_SOURCE_FILES_TEST} ${AMLM_HEADER_FILES_TEST}
		${amlm_SRCS} ### @todo ${SOURCE_FILES} ${HEADER_FILES} ${META_FILES_TO_INCLUDE} ${RESOURCE_FILES}
		${amlm_settings_SRCS}
		${PROJECT_BINARY_DIR}/resources/VersionInfo.cpp
	)
target_compile_definitions(test_qtestamlmjobtests PRIVATE
	"QT_SHAREDPOINTER_TRACK_POINTERS=1"
	"QT_DEPRECATED_WARNINGS"
	"QT_NO_KEYWORDS"
	"USE_BUNDLED_VERDIGRIS=${USE_BUNDLED_VERDIGRIS}"
	"TEST_FWK_IS_QTEST=1"
	)
target_include_directories(test_qtestamlmjobtests PRIVATE "utils/nomoc" ${INCLUDE_DIR_VERDIGRIS})
target_link_libraries(test_qtestamlmjobtests Qt5::Test ${PROJECT_COMMON_LINK_LIBS})
set_target_properties(test_qtestamlmjobtests PROPERTIES
	AUTOMOC ON
	AUTOUIC ON)
add_test(NAME test_qtestamlmjobtests COMMAND $<TARGET_FILE:test_qtestamlmjobtests>)
ecm_mark_as_test(test_qtestamlmjobtests)
###
### END Qt5 QTests
###
endif()

endif() # Enable test target.

###
### Installation.
###

### The AMLM binary.
### @note ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} "should be used when libraries or user-executable applications are installed,
###        [...] It MUST NOT be used for installing plugins, system admin executables or executables only intended for use internally by
###        other code.  Those should use KDE_INSTALL_PLUGINDIR, KDE_INSTALL_SBINDIR or KDE_INSTALL_LIBEXECDIR respectively."
### @see https://api.kde.org/ecm/kde-module/KDEInstallDirs.html?highlight=kde_install_targets_default_args#kdeinstalldirs
install(TARGETS ${PROJECT_NAME} ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
				#RUNTIME DESTINATION . # Default is /usr/local/${PROJECT_NAME} on UNIX and c:/Program Files/${PROJECT_NAME}/${PROJECT_NAME} on Windows.
				#LIBRARY DESTINATION .
				#ARCHIVE DESTINATION .
		COMPONENT coreapp)
# Install the App Icon
install(FILES ${AppIcon_rcc}
	    DESTINATION ${KDE_INSTALL_APPDIR}
		COMPONENT coreapp)
		
# Make the installed app debuggable.
# Add a .gdbinit file and support files to the install directory.
install(PROGRAMS gdb_wrap.sh
	DESTINATION
	    ${KDE_INSTALL_BINDIR}
	CONFIGURATIONS
		Debug
	COMPONENT
		coreapp
		)
install(PROGRAMS  gdbinit.in
	RENAME
		 .gdbinit
	DESTINATION
	    ${KDE_INSTALL_BINDIR}
	CONFIGURATIONS
		Debug
	COMPONENT
		coreapp
		)
###
### KF5 icon theme install for especially Windows and Mac.
###
### Per the docs @link https://api.kde.org/frameworks/kiconthemes/html/index.html, as long as icontheme.rcc
### is installed "in a directory found by QStandardPaths::AppDataLocation", the KDE QPA should find it.
### I'm not sure that actually works.  Even on Linux, it seems that "icontheme.rcc" apparently has to be in
### ${KDE_INSTALL_BINDIR}/data/ (bin/data/), and not in any of the actual QStandardPaths::AppDataLocation dirs
### (Linux: "~/.local/share/<APPNAME>", "/usr/local/share/<APPNAME>", "/usr/share/<APPNAME>") for it to be picked up.
###
### @link https://blogs.kde.org/2016/06/16/icon-theme-deployment-windows-mac-os-and-mobile-platforms
if(false)#WIN32)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/icons_oxygen.rcc
		DESTINATION ${KDE_INSTALL_ICONDIR}
		RENAME "icontheme.rcc"
		COMPONENT coreapp)
endif()
### @test TRYING EVERYTHING TO TRY TO GET THE DAMN ICONS TO WORK AGAIN.
#ecm_install_icons(ICONS ${ICONS_SRCS}
#	DESTINATION ${KDE_INSTALL_ICONDIR}
#	THEME hicolor
#	)
#ecm_install_icons(ICONS
#    DESTINATION ${KDE_INSTALL_ICONDIR}
#    THEME hicolor
#	)
# Desktop file.
install(PROGRAMS io.github.gvansickle.awesomemedialibrarymanager.desktop DESTINATION ${KDE_INSTALL_APPDIR}
		COMPONENT coreapp)
# AppStream component metadata files.
install(FILES io.github.gvansickle.awesomemedialibrarymanager.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR}
		COMPONENT coreapp)
# KConfig file.
install(FILES AwesomeMediaLibraryManagerSettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR}
	    COMPONENT coreapp)

