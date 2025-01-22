#
# Copyright 2018, 2024 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

###
### Google Test
###
find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
		googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG main
		#${UPDATE_DISCONNECTED_IF_AVAILABLE} @todo
)
# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# Try to disable Google Test/Mock's own tests.
set(gmock_build_tests OFF CACHE BOOL "" FORCE)
set(gtest_build_samples OFF CACHE BOOL "" FORCE)
set(gtest_build_tests OFF CACHE BOOL "" FORCE)
# Make googletest available to our build. This adds
# the following targets: gtest, gtest_main, gmock
# and gmock_main.
FetchContent_MakeAvailable(googletest)
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
     logic/serialization/tests/XmlSerializerTest.cpp
     concurrency/tests/ExtAsyncTaskTests.cpp
     concurrency/tests/ExtAsyncTests.cpp
     concurrency/tests/ExtAsyncTestCommon.cpp
     concurrency/tests/ExtFutureTests.cpp
     concurrency/tests/AMLMJobTests.cpp
)
list(TRANSFORM AMLM_SOURCE_FILES_TEST PREPEND "../src/")

list(APPEND AMLM_HEADER_FILES_TEST
     concurrency/tests/ExtAsyncTaskTests.h
     concurrency/tests/ExtAsyncTests.h
     concurrency/tests/ExtAsyncTestCommon.h
     concurrency/tests/ExtFutureTests.h
     concurrency/tests/AMLMJobTests.h
)
list(TRANSFORM AMLM_HEADER_FILES_TEST PREPEND "../src/")

list(APPEND AMLM_SOURCE_FILES_TEST
     ${PROJECT_SOURCE_DIR}/tests/FlagsAndEnumsTests.cpp
)
list(APPEND AMLM_HEADER_FILES_TEST
     ${PROJECT_SOURCE_DIR}/tests/TestHelpers.h
     ${PROJECT_SOURCE_DIR}/tests/FlagsAndEnumsTests.h
)

add_executable(${TEST_EXE_TARGET} ${WIN32_OR_MACOS_BUNDLE} EXCLUDE_FROM_ALL)
target_sources(${TEST_EXE_TARGET}
               PRIVATE
                    alltests.cpp
                    ${AMLM_SOURCE_FILES_TEST}
                    ${AMLM_HEADER_FILES_TEST}
					${amlm_SRCS} ### @todo ${SOURCE_FILES} ${HEADER_FILES} ${META_FILES_TO_INCLUDE} ${RESOURCE_FILES}
#					${amlm_settings_SRCS}
					${PROJECT_BINARY_DIR}/resources/VersionInfo.cpp
)
target_include_directories(${TEST_EXE_TARGET}
                           PRIVATE
                           "utils/nomoc"
                           ${INCLUDE_DIR_VERDIGRIS}
                           "../src/logic"
                           "../src"
                           # For config.h
                           ${PROJECT_BINARY_DIR}/src
)
#target_compile_features(${TEST_EXE_TARGET} PRIVATE cxx_relaxed_constexpr cxx_variable_templates)
target_compile_definitions(${TEST_EXE_TARGET}
                           PUBLIC
                           # libstdc++ defs: @link https://gcc.gnu.org/onlinedocs/gcc-9.1.0/libstdc++/manual/
                           # Enable the libstdc++ debug mode.
                           #_GLIBCXX_DEBUG
                           PRIVATE
                           "USE_BUNDLED_VERDIGRIS=${USE_BUNDLED_VERDIGRIS}"
                           "TEST_FWK_IS_GTEST=1"
)
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
target_link_libraries(${TEST_EXE_TARGET}
	PUBLIC
		# Targetized C++ compile settings.
		cxx_settings
		# Qt-specific -D's.
		cxx_definitions_qt
	PRIVATE
		Threads::Threads
		gtest
		gmock
		tests
#		              kconfig_target
                      libapp

                      #                      src
#		utils
		Qt5::Test
		${PROJECT_COMMON_LINK_LIBS}
#		${QT5_LINK_LIB_TARGETS}
		${KF5_LINK_LIB_TARGETS}
)
add_test(NAME ${TEST_EXE_TARGET} COMMAND $<TARGET_FILE:${TEST_EXE_TARGET}>)
ecm_mark_as_test(${TEST_EXE_TARGET})

########################

add_library(tests STATIC EXCLUDE_FROM_ALL)
target_sources(tests
	PRIVATE
		IResultsSequenceMock.cpp
		IResultsSequenceMock.h
		TestLifecycleManager.cpp
		TestLifecycleManager.h
		TestHelpers.h
	)
target_include_directories(tests
	PRIVATE
		# For config.h
		${PROJECT_BINARY_DIR}/src
		${Qt5Gui_PRIVATE_INCLUDE_DIRS}
		${Boost_INCLUDE_DIRS}
)
target_link_libraries(tests
	PRIVATE
		gtest
		gmock
		amlm_boost_settings_target
		#${Boost_LIBRARIES}
)


