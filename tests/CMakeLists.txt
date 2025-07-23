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
     concurrency/tests/ExtAsyncTests.cpp
     concurrency/tests/ExtAsyncTestCommon.cpp
     concurrency/tests/ExtFutureTests.cpp
     concurrency/tests/AMLMJobTests.cpp
)
list(TRANSFORM AMLM_SOURCE_FILES_TEST PREPEND "../src/")

list(APPEND AMLM_HEADER_FILES_TEST
     concurrency/tests/ExtAsyncTests.h
     concurrency/tests/ExtAsyncTestCommon.h
     concurrency/tests/ExtFutureTests.h
     concurrency/tests/AMLMJobTests.h
)
list(TRANSFORM AMLM_HEADER_FILES_TEST PREPEND "../src/")

list(APPEND AMLM_SOURCE_FILES_TEST
	AlgorithmTests.cpp
	${PROJECT_SOURCE_DIR}/tests/CueSheetTests.cpp
	${PROJECT_SOURCE_DIR}/tests/FlagsAndEnumsTests.cpp
	${PROJECT_SOURCE_DIR}/tests/treetest.cpp
	${PROJECT_SOURCE_DIR}/tests/SerializationTests.cpp
)
list(APPEND AMLM_HEADER_FILES_TEST
	AlgorithmTests.h
	${PROJECT_SOURCE_DIR}/tests/CueSheetTests.h
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
					${PROJECT_BINARY_DIR}/resources/VersionInfo.cpp
)
target_include_directories(${TEST_EXE_TARGET}
                           PRIVATE
                           "../src/logic"
                           "../src"
                           # For config.h
                           ${PROJECT_BINARY_DIR}/src
)

target_compile_definitions(${TEST_EXE_TARGET}
                           PRIVATE
                            "TEST_FWK_IS_GTEST=1"
)
target_compile_definitions(${TEST_EXE_TARGET}
	PUBLIC
		"GTEST_LANGUAGE_CXX11"
)
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
		cxx_compile_options
		cxx_settings
		# Qt-specific -D's.
		cxx_definitions_qt
	PRIVATE
		stdc++exp
#		Threads::Threads
		gtest
		gmock
		tests
        libapp
#		utils
		Qt6::Test
		${PROJECT_COMMON_LINK_LIBS}
#		${QT_LINK_LIB_TARGETS}
		${KF_LINK_LIB_TARGETS}
)
add_test(NAME ${TEST_EXE_TARGET} COMMAND $<TARGET_FILE:${TEST_EXE_TARGET}>)
ecm_mark_as_test(${TEST_EXE_TARGET})

########################

if(TRUE)
	###
	### Qt QTests
	###
	add_executable(test_qtestamlmjobtests ${WIN32_OR_MACOS_BUNDLE} EXCLUDE_FROM_ALL) #${test_qtestamlmjobtests_SRCS} ${} ${amlm_settings_SRCS})
	target_sources(test_qtestamlmjobtests
	               PRIVATE
	               ../src/concurrency/tests/QTestAMLMJobTests.cpp
	               ../src/concurrency/tests/ExtAsyncTestCommon.cpp
	               ../src/concurrency/tests/ExtAsyncTestCommon.h
	               #${AMLM_SOURCE_FILES_TEST} ${AMLM_HEADER_FILES_TEST}
#	               ${amlm_SRCS} ### @todo ${SOURCE_FILES} ${HEADER_FILES} ${META_FILES_TO_INCLUDE} ${RESOURCE_FILES}
	               ${PROJECT_BINARY_DIR}/resources/VersionInfo.cpp
	)
	target_compile_definitions(test_qtestamlmjobtests
	                           PRIVATE
#	                           "QT_SHAREDPOINTER_TRACK_POINTERS=1"
#	                           "QT_DEPRECATED_WARNINGS"
#	                           "QT_NO_KEYWORDS"
	                           "TEST_FWK_IS_QTEST=1"
	)
	target_compile_options(test_qtestamlmjobtests PRIVATE ${EXTRA_CXX_COMPILE_FLAGS})
	set_target_properties(test_qtestamlmjobtests PROPERTIES
	                      AUTOMOC ON
	                      AUTOUIC ON)
	target_include_directories(test_qtestamlmjobtests
	                           PRIVATE
	                           "../src/logic"
	                           "../src"
	                           # For config.h
	                           ${PROJECT_BINARY_DIR}/src
	)
	target_link_libraries(test_qtestamlmjobtests
	                      PUBLIC
							# Targetized C++ compile settings.
							cxx_compile_options
							cxx_settings
							# Qt-specific -D's.
							cxx_definitions_qt
	                      PRIVATE
								stdc++exp
								utils
								concurrency
								logic
								libapp
								Qt6::Test
								${PROJECT_COMMON_LINK_LIBS}
								${KF_LINK_LIB_TARGETS}

	)

	add_test(NAME test_qtestamlmjobtests COMMAND $<TARGET_FILE:test_qtestamlmjobtests>)
	ecm_mark_as_test(test_qtestamlmjobtests)
	###
	### END Qt QTests
	###
endif()

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
		${QtGui_PRIVATE_INCLUDE_DIRS}
)
target_link_libraries(tests
	PUBLIC
		cxx_compile_options
		stdc++exp
	PRIVATE
		gtest
		gmock
)


