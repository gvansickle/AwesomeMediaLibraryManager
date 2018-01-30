
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(test_Promise
	PromiseTest.cpp
	${PROJECT_SOURCE_DIR}/src/Promise.cpp
	${PROJECT_SOURCE_DIR}/src/Deferred.cpp
	${PROJECT_SOURCE_DIR}/src/ChildDeferred.cpp
)
target_link_libraries(test_Promise Qt5::Core Qt5::Test)

add_test(NAME Promise COMMAND test_Promise)
set_tests_properties(Promise PROPERTIES TIMEOUT 30)
