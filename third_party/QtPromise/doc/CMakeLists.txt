find_package(Doxygen)
if(DOXYGEN_FOUND)
	if(NOT QT5_LATEST_DOCS_DIR)
		set(QT5_DOCS_BASE_DIR "${QT5_BIN_DIR}/../../../Docs")
		file(GLOB QT5_DOCS_DIRS "${QT5_DOCS_BASE_DIR}/*")
		list(SORT QT5_DOCS_DIRS)
		list(REVERSE QT5_DOCS_DIRS)
		find_path(QT5_LATEST_DOCS_DIR qtcore.qch qtcore/qtcore.tags
		          PATHS ${QT5_DOCS_DIRS} "/usr/share/qt5/doc"
		          DOC "Directory containing Qt5 documentation")
	endif()
	
	find_file(QT5_CORE_TAGS qtcore.tags PATHS "${QT5_LATEST_DOCS_DIR}/qtcore" DOC "Qt5::Core documentation tags file")
	find_file(QT5_NETWORK_TAGS qtnetwork.tags PATHS "${QT5_LATEST_DOCS_DIR}/qtnetwork" DOC "Qt5::Network documentation tags file")
	
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	add_custom_target(docs
		${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
	)
endif(DOXYGEN_FOUND)