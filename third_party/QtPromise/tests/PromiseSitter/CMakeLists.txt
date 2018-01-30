
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(test_PromiseSitter
	PromiseSitterTest.cpp
	${PROJECT_SOURCE_DIR}/src/Promise.cpp
	${PROJECT_SOURCE_DIR}/src/Deferred.cpp
	${PROJECT_SOURCE_DIR}/src/ChildDeferred.cpp
	${PROJECT_SOURCE_DIR}/src/PromiseSitter.cpp
)
target_link_libraries(test_PromiseSitter Qt5::Core Qt5::Test)

add_test(NAME PromiseSitter COMMAND test_PromiseSitter)
set_tests_properties(PromiseSitter PROPERTIES TIMEOUT 30)
