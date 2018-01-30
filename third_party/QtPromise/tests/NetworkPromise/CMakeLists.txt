
include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(test_NetworkPromise
	NetworkPromiseTest.cpp
	${PROJECT_SOURCE_DIR}/src/NetworkPromise.cpp
	${PROJECT_SOURCE_DIR}/src/NetworkDeferred.cpp
	${PROJECT_SOURCE_DIR}/src/Promise.cpp
	${PROJECT_SOURCE_DIR}/src/Deferred.cpp
	${PROJECT_SOURCE_DIR}/src/ChildDeferred.cpp
)
target_link_libraries(test_NetworkPromise Qt5::Core Qt5::Network Qt5::Test)

add_test(NAME NetworkPromise COMMAND test_NetworkPromise)
set_tests_properties(NetworkPromise PROPERTIES TIMEOUT 30)
