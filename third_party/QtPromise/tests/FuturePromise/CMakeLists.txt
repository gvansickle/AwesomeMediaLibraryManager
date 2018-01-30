
include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(test_FuturePromise
	FuturePromiseTest.cpp
	${PROJECT_SOURCE_DIR}/src/FuturePromise.cpp
	${PROJECT_SOURCE_DIR}/src/FutureDeferred.cpp
	${PROJECT_SOURCE_DIR}/src/Promise.cpp
	${PROJECT_SOURCE_DIR}/src/Deferred.cpp
	${PROJECT_SOURCE_DIR}/src/ChildDeferred.cpp
)
target_link_libraries(test_FuturePromise Qt5::Core Qt5::Concurrent Qt5::Test)

add_test(NAME FuturePromise COMMAND test_FuturePromise)
set_tests_properties(FuturePromise PROPERTIES TIMEOUT 30)
