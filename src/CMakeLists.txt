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

# This is the sum-total of all source files which go into building the main exe.
set(amlm_SRCS "")


###
### The third-party packages we require.
###

set(HEADER_FILES_THIRD_PARTY "")
set(SOURCE_FILES_THIRD_PARTY "")

set(AMLM_LIBCUE_TARGET "")
set(AMLM_TAGLIB_TARGET "")

# Use the bundled version of libcue due to this issue: https://github.com/lipnitsk/libcue/commit/e60b7b01a40009c7c71c5e154a328881a6e9ae46
set(USE_BUNDLED_LIBCUE TRUE)

if(NOT WIN32)
    pkg_check_modules(TAGLIB REQUIRED taglib>=1.6
			IMPORTED_TARGET)
	if(TAGLIB_FOUND)
		set(AMLM_TAGLIB_TARGET "PkgConfig::TAGLIB")
	endif()
else()
    # Use the in-tree copy of TagLib.  Requires some non-obvious cmake gyrations, since TagLib needs to be installed before you can realistically include its headers.
    # Based somewhat on this SO answer: https://stackoverflow.com/a/45079298
    add_library(taglib INTERFACE)
    ExternalProject_Add(taglib_bundled
            PREFIX "taglib_bundled"  # "Relative paths are interpreted with respect to CMAKE_CURRENT_BINARY_DIR at the point where ExternalProject_Add() is called."
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib"
            INSTALL_DIR "taglib_bundled/install"
            LOG_CONFIGURE TRUE
            LOG_BUILD TRUE
            LOG_INSTALL TRUE
            # Configure step options.
            CMAKE_ARGS "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}" "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}" "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}" "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>" "-DBUILD_BINDINGS=OFF"
            # Create a CMake target "taglib_bundled-install" which can be depended upon.
            STEP_TARGETS install
            )
    add_dependencies(taglib taglib_bundled-install)
    ExternalProject_Get_Property(taglib_bundled INSTALL_DIR)
    message(STATUS "taglib_bundled INSTALL_DIR: ${INSTALL_DIR}")
#    print_target_properties(taglib_bundled)
	cmake_print_variables(CMAKE_CXX_COMPILER CMAKE_VERSION)

    set(taglib_LIBRARY "${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}tag${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(taglib_INCLUDE_DIR "${INSTALL_DIR}/include")
    message(STATUS "taglib_LIBRARY: ${taglib_LIBRARY}")
    message(STATUS "taglib_INCLUDE_DIR: ${taglib_INCLUDE_DIR}")

    # Set INTERFACE properties on our taglib INTERFACE library.
    # https://cmake.org/cmake/help/v3.10/command/add_library.html?highlight=add_librar#id6
    target_link_libraries(taglib INTERFACE ${taglib_LIBRARY})
    target_include_directories(taglib INTERFACE ${taglib_INCLUDE_DIR})
    target_compile_definitions(taglib INTERFACE "TAGLIB_STATIC")

    # Workaround needed to prevent cmake from complaining about INTERFACE_INCLUDE_DIRECTORIES being a "nonexistent path".
    # @todo looks like we no longer need this.
    #file(MAKE_DIRECTORY ${INSTALL_DIR}/include)

    set(AMLM_TAGLIB_TARGET "taglib")

    #print_target_properties(${AMLM_TAGLIB_TARGET})

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

# Verdigris is just two headers.
set(USE_BUNDLED_VERDIGRIS 1)
set(INCLUDE_DIR_VERDIGRIS third_party/verdigris/src)


# Add KConfig source files to the amlm_SRCS list.
kconfig_add_kcfg_files(amlm_SRCS AwesomeMediaLibraryManagerSettings.kcfgc)
# Target to get the deps on generated files right.
add_custom_target(AwesomeMediaLibraryManagerSettings_h DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/AwesomeMediaLibraryManagerSettings.h)

# Main program header files.
set(HEADER_FILES
	gui/CollectionDockWidget.h
	gui/Experimental.h
	gui/FilterWidget.h
	gui/ItemDelegateLength.h
	gui/MainWindow.h
	gui/MDIArea.h
	gui/MDILibraryView.h
	gui/MDIPlaylistView.h
	gui/MDITreeViewBase.h
	gui/MetadataDockWidget.h
	gui/NetworkAwareFileDialog.h
	gui/PixmapLabel.h
	gui/PlayerControls.h
	logic/Library.h
	logic/LibraryEntry.h
	logic/LibraryEntryMimeData.h
	logic/LibraryModel.h
	logic/LibrarySortFilterProxyModel.h
	logic/Metadata.h
	logic/MetadataAbstractBase.h
	logic/MetadataFromCache.h
	logic/MetadataTaglib.h
	logic/MP2.h
	logic/ntp.h
	logic/PlaylistModel.h
	logic/PlaylistModelItem.h
	logic/PlaylistSortFilterProxyModel.h
	logic/TrackMetadata.h
	utils/ActionHelpers.h
	utils/ConnectHelpers.h
	utils/DebugHelpers.h
	utils/Fraction.h
	utils/in.h
	utils/MapConverter.h
	utils/StringHelpers.h
	utils/Theme.h
	utils/WorkerThreadBase.h
	logic/LibraryRescanner.h
	gui/ActivityProgressWidget.h
	utils/AsyncDirScanner.h
	gui/AboutBox.h
	utils/RegisterQtMetatypes.h
	logic/CueSheetParser.h
	gui/settings/SettingsDialog.h
	gui/settings/SettingsDialogSideWidget.h
	../resources/VersionInfo.h    ### @todo
	logic/ModelUserRoles.h
	gui/DragDropTreeViewStyleProxy.h
	logic/proxymodels/ModelHelpers.h
	logic/proxymodels/SelectionFilterProxyModel.h
	logic/proxymodels/ModelChangeWatcher.h
	gui/helpers/Tips.h
	gui/menus/DropMenu.h
	gui/menus/LibraryContextMenu.h
	gui/menus/PlaylistContextMenuViewport.h
	gui/menus/PlaylistContextMenu.h
	gui/menus/ActionBundle.h
	gui/mdi/MDIModelViewPair.h
	logic/ColumnSpec.h
	logic/proxymodels/QPersistentModelIndexVec.h
	utils/Logging.h
	utils/AboutDataSetup.h
	gui/menus/HelpMenu.h
    )

# Source file list.
set(SOURCE_FILES
	gui/CollectionDockWidget.cpp
	gui/Experimental.cpp
	gui/FilterWidget.cpp
	gui/ItemDelegateLength.cpp
	gui/MainWindow.cpp
	gui/MDIArea.cpp
	gui/MDILibraryView.cpp
	gui/MDIPlaylistView.cpp
	gui/MDITreeViewBase.cpp
	gui/MetadataDockWidget.cpp
	gui/NetworkAwareFileDialog.cpp
	gui/PixmapLabel.cpp
	gui/PlayerControls.cpp
	logic/Library.cpp
	logic/LibraryEntry.cpp
	logic/LibraryEntryMimeData.cpp
	logic/LibraryModel.cpp
	logic/LibrarySortFilterProxyModel.cpp
	logic/Metadata.cpp
	logic/MetadataAbstractBase.cpp
	logic/MetadataFromCache.cpp
	logic/MetadataTaglib.cpp
	logic/MP2.cpp
	logic/ntp.cpp
	logic/PlaylistModel.cpp
	logic/PlaylistModelItem.cpp
	logic/PlaylistSortFilterProxyModel.cpp
	logic/TrackMetadata.cpp
	utils/Fraction.cpp
	utils/MapConverter.cpp
	utils/Theme.cpp
	utils/WorkerThreadBase.cpp
	logic/LibraryRescanner.cpp
	gui/ActivityProgressWidget.cpp
	concurrency/ReportingRunner.cpp
	utils/AsyncDirScanner.cpp
	gui/AboutBox.cpp
	utils/RegisterQtMetatypes.cpp
	logic/CueSheetParser.cpp
	gui/settings/SettingsDialog.cpp
	gui/settings/SettingsDialogPageBase.cpp gui/settings/SettingsDialogPageBase.h
	gui/settings/SDPageAppearance.cpp gui/settings/SDPageAppearance.h
	gui/settings/SettingsDialogSideWidget.cpp
	gui/settings/SDPageLibrary.cpp
	gui/settings/SDPageLibrary.h
	gui/MDINowPlayingView.cpp
	gui/MDINowPlayingView.h
	gui/settings/SettingsDialogBase.cpp gui/settings/SettingsDialogBase.h
	logic/ModelUserRoles.cpp
	gui/DragDropTreeViewStyleProxy.cpp
	logic/proxymodels/SelectionFilterProxyModel.cpp
	logic/proxymodels/ModelChangeWatcher.cpp
	gui/helpers/Tips.cpp
	gui/menus/DropMenu.cpp
	gui/menus/LibraryContextMenu.cpp
	gui/menus/PlaylistContextMenuViewport.cpp
	gui/menus/PlaylistContextMenu.cpp
	gui/menus/ActionBundle.cpp
	gui/mdi/MDIModelViewPair.cpp
	logic/ColumnSpec.cpp
	logic/proxymodels/QPersistentModelIndexVec.cpp
	utils/Logging.cpp
	utils/AboutDataSetup.cpp
	gui/menus/HelpMenu.cpp)

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
	concurrency/ExtFuture.h
	concurrency/ExtFutureWatcher.h
	concurrency/impl/ExtFuture_fwddecl_p.h
	concurrency/impl/ExtFuture_impl.hpp
	concurrency/ExtAsync.h
	concurrency/ExtAsyncTask.h
	concurrency/function_traits.hpp
	)
set(SOURCE_FILES_UTILS_CONCURRENCY
	concurrency/AsyncTaskManager.cpp
	concurrency/ExtAsync.cpp
	concurrency/function_traits.cpp
	)

#add_subdirectory(gui)

# UI files.
set(SOURCE_FILES_UI
	DefaultSettingsHelper.cpp
	DefaultSettingsHelper.h
	SettingsPageGeneral.cpp
	SettingsPageGeneral.h
	SettingsPageGeneral.ui
	SettingsPageCollection.cpp
	SettingsPageCollection.h
	SettingsPageCollection.ui
	SettingsPageAppearance.cpp
	SettingsPageAppearance.h
	SettingsPageAppearance.ui
	SettingsPageLibrary.cpp
	SettingsPageLibrary.h
	SettingsPageLibrary.ui
	)
PREPEND_TO_EACH(SOURCE_FILES_UI "gui/settings/" ${SOURCE_FILES_UI})

#
# Append all the SOURCE_* subsets.
#

# *.ui files.
list(APPEND SOURCE_FILES ${SOURCE_FILES_UI})

# Concurrency
list(APPEND HEADER_FILES ${HEADER_FILES_UTILS_CONCURRENCY})
list(APPEND SOURCE_FILES ${SOURCE_FILES_UTILS_CONCURRENCY})

# Append the third_party files which need to be explicitly in the SOURCE_/HEADER_FILES vars.
list(APPEND HEADER_FILES ${HEADER_FILES_THIRD_PARTY})
list(APPEND SOURCE_FILES ${SOURCE_FILES_THIRD_PARTY})

# Build the ultimate list of lists.
list(APPEND amlm_SRCS ${SOURCE_FILES} ${HEADER_FILES})

# Sets the icon on Windows and OSX.
# See https://api.kde.org/ecm/module/ECMAddAppIcon.html
file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../resources/*-preferences-desktop-sound.png")
ecm_add_app_icon(amlm_SRCS ICONS ${ICONS_SRCS})


# Qt5 Binary Resource (rcc) files.
qt5_add_binary_resources(icons_oxygen ../resources/oxygenicons.qrc)
set(AppIcon_qrc "${CMAKE_CURRENT_SOURCE_DIR}/../resources/AppIcon.qrc")
set(AppIcon_rcc_dir "${CMAKE_CURRENT_BINARY_DIR}/../bin")
set(AppIcon_rcc "${AppIcon_rcc_dir}/AppIcon.rcc")
# Set up the app icon for Linux, add a dep so it gets built.
qt5_add_binary_resources(build_AppIcon_rcc ${AppIcon_qrc} DESTINATION ${AppIcon_rcc})

###
### The main executable.
###
### @note Including HEADER_FILES in here so they get checked for existence.
###
add_executable(${PROJECT_NAME} ${OS_BUNDLE} # Expands to WIN32 or MACOS_BUNDLE depending on OS
		main.cpp
		${amlm_SRCS} #${SOURCE_FILES} ${HEADER_FILES} ${META_FILES_TO_INCLUDE} ${RESOURCE_FILES}
		${PROJECT_BINARY_DIR}/resources/VersionInfo.cpp
	)
# Compiler features FBO Verdigris (esp. on Windows/MSVC).  -D__cpp_constexpr=201304 -D__cpp_variable_templates=201304.
target_compile_features(${PROJECT_NAME} PRIVATE cxx_relaxed_constexpr cxx_variable_templates)
target_compile_definitions(${PROJECT_NAME} PRIVATE
	"QT_SHAREDPOINTER_TRACK_POINTERS"
	"QT_DEPRECATED_WARNINGS"
	"QT_NO_KEYWORDS"
	"USE_BUNDLED_VERDIGRIS=${USE_BUNDLED_VERDIGRIS}"
	"QFUTURE_TEST")
target_include_directories(${PROJECT_NAME} PRIVATE "utils/nomoc" ${INCLUDE_DIR_VERDIGRIS})
if(NOT WIN32)
	target_compile_options(${PROJECT_NAME} PUBLIC "-Og;-ggdb3;-fno-omit-frame-pointer")
endif()
target_compile_options(${PROJECT_NAME} PRIVATE ${EXTRA_CXX_COMPILE_FLAGS})
# Only the Qt stuff needs AUTOMOC.
### This doesn't work: set_property(FILES ${SOURCE_FILES} ${HEADER_FILES} PROPERTY AUTOMOC ON)
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    AUTOMOC ON
    AUTOUIC ON)
# The libs.
set(PROJECT_COMMON_LINK_LIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Multimedia Qt5::Concurrent
		${KF5_LINK_LIBS}
		${GTK_LINK_LIB_TARGETS}
		${AMLM_TAGLIB_TARGET} ${AMLM_LIBCUE_TARGET} ${AMLM_QXTGLOBALSHORTCUT_LIB_TARGET}
		callable_traits)
target_link_libraries(${PROJECT_NAME} ${PROJECT_COMMON_LINK_LIBS})
add_dependencies(${PROJECT_NAME} icons_oxygen build_AppIcon_rcc)

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
### Google Test
find_package(Threads REQUIRED)
download_project(PROJ                googletest
                 GIT_REPOSITORY      https://github.com/google/googletest.git
                 GIT_TAG             master
                 ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)
# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR}
	EXCLUDE_FROM_ALL)

#enable_testing()
set(TEST_EXE_TARGET alltests)
set(SOURCE_FILES_TEST)
set(HEADER_FILES_TEST)
list(APPEND SOURCE_FILES_TEST
	concurrency/tests/AsyncTests.cpp)
list(APPEND
	tests/TestHelpers.h
	concurrency/tests/AsyncTests.h)

add_executable(${TEST_EXE_TARGET}
				alltests.cpp ${SOURCE_FILES_TEST} ${HEADER_FILES_TEST}
				${amlm_SRCS} ### @todo ${SOURCE_FILES} ${HEADER_FILES} ${META_FILES_TO_INCLUDE} ${RESOURCE_FILES}
				${PROJECT_BINARY_DIR}/resources/VersionInfo.cpp)
target_compile_features(${TEST_EXE_TARGET} PRIVATE cxx_relaxed_constexpr cxx_variable_templates)
target_compile_definitions(${TEST_EXE_TARGET} PRIVATE
	"QT_SHAREDPOINTER_TRACK_POINTERS"
	"QT_DEPRECATED_WARNINGS"
	"QT_NO_KEYWORDS"
	"USE_BUNDLED_VERDIGRIS=${USE_BUNDLED_VERDIGRIS}"
	"QFUTURE_TEST")
target_include_directories(${TEST_EXE_TARGET} PRIVATE "utils/nomoc" ${INCLUDE_DIR_VERDIGRIS})
target_compile_definitions(${TEST_EXE_TARGET} PRIVATE "GTEST_LANGUAGE_CXX11")
target_compile_options(${TEST_EXE_TARGET} PRIVATE ${EXTRA_CXX_COMPILE_FLAGS})
set_target_properties(${TEST_EXE_TARGET} PROPERTIES
	AUTOMOC ON
	AUTOUIC ON)
target_link_libraries(${TEST_EXE_TARGET} Threads::Threads gtest gmock_main)
target_link_libraries(${TEST_EXE_TARGET} Qt5::Test ${PROJECT_COMMON_LINK_LIBS})
add_test(NAME ${TEST_EXE_TARGET} COMMAND ${TEST_EXE_TARGET})

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
# Icons
install(FILES ${AppIcon_rcc} DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES
	    #${AppIcon_rcc}
		${CMAKE_CURRENT_BINARY_DIR}/icons_oxygen.rcc
		DESTINATION ${KDE_INSTALL_APPDIR}
		COMPONENT coreapp)
# Desktop file.
install(PROGRAMS io.github.gvansickle.awesomemedialibrarymanager.desktop DESTINATION ${KDE_INSTALL_APPDIR}
		COMPONENT coreapp)
# AppStream component metadata files.
install(FILES io.github.gvansickle.awesomemedialibrarymanager.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR}
		COMPONENT coreapp)
# KConfig file.
install(FILES AwesomeMediaLibraryManagerSettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR}
	    COMPONENT coreapp)

