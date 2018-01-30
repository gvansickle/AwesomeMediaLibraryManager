
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(test_Deferred
	DeferredTest.cpp
	${PROJECT_SOURCE_DIR}/src/Deferred.cpp
)
target_link_libraries(test_Deferred Qt5::Core Qt5::Test)

add_test(NAME Deferred COMMAND test_Deferred)
set_tests_properties(Deferred PROPERTIES TIMEOUT 30)
