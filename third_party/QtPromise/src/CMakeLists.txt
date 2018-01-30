
set(QT5PROMISE_SOURCES
	Deferred.h
	Deferred.cpp
	Promise.h
	Promise.cpp
	ChildDeferred.h
	ChildDeferred.cpp
	NetworkDeferred.h
	NetworkDeferred.cpp
	NetworkPromise.h
	NetworkPromise.cpp
	PromiseSitter.h
	PromiseSitter.cpp
	FutureDeferred.h
	FutureDeferred.cpp
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(qt5promise ${QT5PROMISE_SOURCES})
target_link_libraries(qt5promise Qt5::Core Qt5::Network)

if (Qt5::Concurrent_FOUND)
	target_link_libraries(qt5promise Qt5::Concurrent)
endif()
