#
# Adapted from https://github.com/gsauthof/tree-model.
# LGPL3+
#


if(FALSE)

#cmake_minimum_required(VERSION 2.8)
cmake_minimum_required(VERSION 3.14.0)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project(tree_model
	LANGUAGES
		CXX
	)

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

###include (${CMAKE_SOURCE_DIR}/cmake/modules/coverage.cmake)
###include (${CMAKE_SOURCE_DIR}/cmake/modules/sanitize.cmake)

# can't use because cmake currently has a
# bug the same filename in different directories
# yields collisions:
# http://public.kitware.com/Bug/view.php?id=12873
# -> using qt5_wrap_cpp() instead ...
#set(CMAKE_AUTOMOC ON)
# same goes for autouic ...
#set(CMAKE_AUTOUIC ON)
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
	Core
	Gui
	Widgets
	#Multimedia
	#XmlPatterns
	Concurrent
	Test # For Q...ModelTester.
	)
set(QT5_LINK_LIB_TARGETS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Concurrent Qt5::Test)
# find_package(Qt5Core REQUIRED)
# find_package(Qt5Widgets REQUIRED)
# find_package(Qt5Test REQUIRED)
#find_package(LibXml2 REQUIRED)
if(FALSE)
find_package(Boost 1.55
    COMPONENTS
#      unit_test_framework
      regex
      filesystem
      system
  REQUIRED)
endif()
#add_subdirectory(libixxx)
#add_subdirectory(libixxxutil)
#add_subdirectory(libxxxml)
#add_subdirectory(libgrammar)
#set(OLD_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS false)
### GRVS add_subdirectory(cppformat)
#set(BUILD_SHARED_LIBS ${OLD_BUILD_SHARED_LIBS})
#add_subdirectory(libxfsx)

### GRVS
# qt5_wrap_ui(EDITOR_UI
#   editor/main_window.ui
#   editor/subtree_window.ui
#   editor/tree_widget.ui
#   editor/slider_bar.ui
#   editor/child_dialog.ui
#   editor/result_window.ui
#   )
# 
# qt5_wrap_cpp(EDITOR_MOC
#   editor/main_window.hh
#   editor/subtree_window.hh
#   editor/tree_widget.hh
#   editor/slider_bar.hh
#   editor/child_dialog.hh
#   editor/command/new.hh
#   editor/command/open.hh
#   editor/command/async_open.hh
#   editor/command/save.hh
#   editor/command/async_save.hh
#   editor/command/remove.hh
#   editor/command/write_aci.hh
#   editor/command/read_grammar.hh
#   editor/command/search.hh
#   editor/command/async_search.hh
#   editor/gui_command/select_open.hh
#   editor/gui_command/open.hh
#   editor/gui_command/save.hh
#   editor/gui_command/edit.hh
#   editor/gui_command/add.hh
#   editor/gui_command/clipboard_copy.hh
#   editor/gui_command/clipboard_cut.hh
#   editor/gui_command/clipboard_paste.hh
#   editor/gui_command/display_subtree.hh
#   editor/gui_command/write_aci.hh
#   editor/gui_command/search.hh
#   editor/controller.hh
#   editor/gui_controller.hh
#   editor/arguments.hh
#   editor/preview_file_dialog.hh
#   editor/preview.hh
#   editor/progress_dialog.hh
#   editor/tree_view.hh
#   editor/recent_menu.hh
#   editor/instance.hh
#   editor/instance_manager.hh
#   editor/delegate/tag.hh
#   editor/delegate/value.hh
#   editor/delegate/editor/local_time.hh
#   editor/table_view.hh
#   )

### GRVS
### Modernize QT integration.
#qt5_wrap_cpp(TREE_MOC
set(TREE_MODEL_MOC
  tree_model/item_adaptor.hh
  tree_model/base.hh
  tree_model/recorder.hh
  )

#qt5_wrap_cpp(UT_MOC
#  )
# qt5_wrap_cpp(UT_SELECT_OPEN_MOC
#   test/editor/gui_command/select_open.hh
#   )

# qt5_add_resources(EDITOR_RES
#   QtAwesome/QtAwesome/QtAwesome.qrc
#   )

# set(EDITOR_CC
#   editor/main_window.cc
#   editor/subtree_window.cc
#   editor/tree_widget.cc
#   editor/slider_bar.cc
#   editor/child_dialog.cc
#   editor/controller.cc
#   editor/gui_controller.cc
#   editor/vc_matchmaker.cc
#   editor/recent_menu.cc
#   editor/command/new.cc
#   editor/command/open.cc
#   editor/command/open_xml.cc
#   editor/command/open_ber.cc
#   editor/command/async_open.cc
#   editor/command/save.cc
#   editor/command/save_ber.cc
#   editor/command/async_save.cc
#   editor/command/remove.cc
#   editor/command/write_aci.cc
#   editor/command/preview.cc
#   editor/command/read_grammar.cc
#   editor/command/search.cc
#   editor/command/async_search.cc
#   editor/gui_command/open.cc
#   editor/gui_command/select_open.cc
#   editor/gui_command/save.cc
#   editor/gui_command/edit.cc
#   editor/gui_command/add.cc
#   editor/gui_command/clipboard_copy.cc
#   editor/gui_command/clipboard_cut.cc
#   editor/gui_command/clipboard_paste.cc
#   editor/gui_command/display_subtree.cc
#   editor/gui_command/write_aci.cc
#   editor/gui_command/search.cc
#   editor/arguments.cc
#   editor/preview_file_dialog.cc
#   editor/preview.cc
#   editor/progress_dialog.cc
#   editor/tree_view.cc
#   editor/absolute_slider_style.cc
#   editor/instance.cc
#   editor/instance_manager.cc
#   editor/file_type.cc
#   editor/delegate/tag.cc
#   editor/delegate/value.cc
#   editor/delegate/model_validator.cc
#   editor/delegate/int64_validator.cc
#   editor/typed_model.cc
#   editor/delegate/value/local_time.cc
#   editor/delegate/editor/local_time.cc
#   editor/delegate/factory.cc
#   editor/setup.cc
#   editor/result_window.cc
#   editor/table_view.cc
#   )
 
set(TREE_MODEL_CC
  tree_model/index.cc
  tree_model/base.cc
  tree_model/item_adaptor.cc
#  tree_model/xml.cc
  tree_model/deep_model_index.cc
  tree_model/recorder.cc
  tree_model/operation/base.cc
  tree_model/operation/transaction.cc
  tree_model/operation/set_data.cc
  tree_model/operation/insert_rows.cc
  tree_model/operation/remove_rows.cc
  tree_model/util.cc
  tree_model/df_qmi_traverser.cc
  tree_model/qmi_iterator.cc
  tree_model/protected_item_model_impl.cc
  tree_model/qpmi_vector_model.cc
  )

### GRVS
add_library(tree_model STATIC)
	
target_sources(tree_model
	PRIVATE
		${TREE_MODEL_MOC}
	PRIVATE
		${TREE_MODEL_CC}
	) 
set_target_properties(tree_model
    PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        )
target_link_libraries(tree_model
	PUBLIC
		${QT5_LINK_LIB_TARGETS}	
	)
       
# GRVS
# set(QT_AWESOME_CC
#   QtAwesome/QtAwesome/QtAwesomeAnim.cpp
#   QtAwesome/QtAwesome/QtAwesome.cpp
# 
#   editor/qt_awesome.cc
#   )
# qt5_wrap_cpp(QT_AWESOME_MOC
#   QtAwesome/QtAwesome/QtAwesomeAnim.h
#   QtAwesome/QtAwesome/QtAwesome.h
#   )

### GRVS
# add_executable(main
#   editor/main.cc
#   ${EDITOR_CC}
#   ${TREE_MODEL_CC}
#   ${EDITOR_MOC}
#   ${TREE_MOC}
#   ${EDITOR_UI}
#   ${QT_AWESOME_CC}
#   ${QT_AWESOME_MOC}
#   ${EDITOR_RES}
#   )
# 
# # WIN32 also true on win64 ...
# if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
#   # By default, cmake built executables show a console window on windows
#   # setting that property to true disables that console window.
#   # Should be equivalent to add_executable(main WIN32 ...)
#   # If we want to unconditionally set it for all build types we can
#   # just add WIN32 to the above add_executable() call, since
#   # it only has an effect when compiling on windows.
#   set_property(TARGET main PROPERTY WIN32_EXECUTABLE true)
# endif()
# set_property(TARGET main PROPERTY INCLUDE_DIRECTORIES
#   ${CMAKE_CURRENT_SOURCE_DIR}
#   ${CMAKE_CURRENT_SOURCE_DIR}/libxxxml
#   # for autogenerated moc/ui files ...
#   ${CMAKE_CURRENT_BINARY_DIR}
#   ${Boost_INCLUDE_DIRS}
#   ${XML2_INCLUDE_DIR}
#   ${CMAKE_CURRENT_SOURCE_DIR}/libixxx
#   ${CMAKE_CURRENT_SOURCE_DIR}/libixxxutil
#   ${CMAKE_CURRENT_SOURCE_DIR}/libgrammar
#   ${CMAKE_CURRENT_SOURCE_DIR}/libxfsx
#   ${CMAKE_CURRENT_SOURCE_DIR}/QtAwesome
#   )
# target_link_libraries(main
#   xfsx
#   grammar
#   ixxxutil
#   ixxx
#   xxxml
#   ${Boost_FILESYSTEM_LIBRARY}
#   ${Boost_SYSTEM_LIBRARY}
#   ${Boost_REGEX_LIBRARY}
#   ${XML2_LIB}
#   Qt5::Widgets
#   )

### GRVS
# add_executable(ut
#   test/main.cc
#   test/test.cc
#   test/list/ranked_list.cc
#   test/tree_model/xml.cc
#   test/tree_model/index.cc
#   test/tree_model/item_adaptor.cc
#   test/tree_model/deep_model_index.cc
#   test/tree_model/recorder.cc
#   test/tree_model/util.cc
#   test/tree_model/df_qmi_traverser.cc
#   test/tree_model/qmi_iterator.cc
#   test/tree_model/protected_item_model.cc
#   test/tree_model/qpmi_vector_model.cc
#   test/qt_model.cc
# 
#   test/editor/command/remove.cc
#   test/editor/command/open.cc
#   test/editor/command/save.cc
#   test/editor/command/write_aci.cc
#   test/editor/command/preview.cc
#   test/editor/command/search.cc
#   test/editor/command/async_search.cc
#   test/editor/traverser/qmi_proxy.cc
#   editor/command/remove.cc
#   editor/command/open.cc
#   editor/command/open_xml.cc
#   editor/command/open_ber.cc
#   editor/command/save.cc
#   editor/command/save_ber.cc
#   editor/command/write_aci.cc
#   editor/command/preview.cc
#   editor/command/search.cc
#   editor/command/async_search.cc
#   editor/file_type.cc
#   editor/typed_model.cc
#   editor/delegate/value/local_time.cc
#   ${CMAKE_CURRENT_BINARY_DIR}/editor/command/moc_remove.cpp
#   ${CMAKE_CURRENT_BINARY_DIR}/editor/command/moc_open.cpp
#   ${CMAKE_CURRENT_BINARY_DIR}/editor/command/moc_save.cpp
#   ${CMAKE_CURRENT_BINARY_DIR}/editor/command/moc_write_aci.cpp
#   ${CMAKE_CURRENT_BINARY_DIR}/editor/command/moc_search.cpp
#   ${CMAKE_CURRENT_BINARY_DIR}/editor/command/moc_async_search.cpp
# 
#   ${TREE_MODEL_CC}
#   ${TREE_MOC}
#   #${UT_MOC}
#   
#   ${CMAKE_SOURCE_DIR}/cppformat/format.cc
# )
# set_property(TARGET ut PROPERTY INCLUDE_DIRECTORIES
#   ${CMAKE_CURRENT_SOURCE_DIR}
#   ${CMAKE_CURRENT_SOURCE_DIR}/Catch/single_include
#   ${CMAKE_CURRENT_SOURCE_DIR}/libxxxml
#   #${CMAKE_CURRENT_SOURCE_DIR}/list
#   # for autogenerated moc/ui files ...
#   ${CMAKE_CURRENT_BINARY_DIR}
#   ${Boost_INCLUDE_DIRS}
#   ${XML2_INCLUDE_DIR}
#   ${CMAKE_CURRENT_SOURCE_DIR}/libixxx
#   ${CMAKE_CURRENT_SOURCE_DIR}/libixxxutil
#   ${CMAKE_CURRENT_SOURCE_DIR}/libgrammar
#   ${CMAKE_CURRENT_SOURCE_DIR}/libxfsx
#   )
# target_link_libraries(ut
#   xxxml_static
#   xfsx_static
#   grammar_static
#   ixxxutil_static
#   ixxx_static
#   ${Boost_FILESYSTEM_LIBRARY}
#   ${Boost_SYSTEM_LIBRARY}
#   ${Boost_REGEX_LIBRARY}
#   ${XML2_LIB}
#   Qt5::Core
#   Qt5::Test
#   )

### GRVS
# add_executable(ut_select_open
#   test/editor/gui_command/select_open.cc
#   test/test.cc
# 
#   ${TREE_MODEL_CC}
#   ${EDITOR_CC}
# 
#   ${UT_SELECT_OPEN_MOC}
#   ${EDITOR_MOC}
#   ${EDITOR_UI}
#   ${TREE_MOC}
# 
#   ${QT_AWESOME_CC}
#   ${QT_AWESOME_MOC}
#   ${EDITOR_RES}
# 
#   ${CMAKE_SOURCE_DIR}/cppformat/format.cc
#   )
# set_property(TARGET ut_select_open PROPERTY INCLUDE_DIRECTORIES
#   ${CMAKE_CURRENT_SOURCE_DIR}/libxxxml
#   ${CMAKE_CURRENT_SOURCE_DIR}
#   ${CMAKE_CURRENT_BINARY_DIR}
#   ${XML2_INCLUDE_DIR}
#   ${CMAKE_CURRENT_SOURCE_DIR}/libixxx
#   ${CMAKE_CURRENT_SOURCE_DIR}/libixxxutil
#   ${CMAKE_CURRENT_SOURCE_DIR}/libgrammar
#   ${CMAKE_CURRENT_SOURCE_DIR}/libxfsx
#   ${CMAKE_CURRENT_SOURCE_DIR}/QtAwesome
#   )
# target_link_libraries(ut_select_open
#   xxxml_static
#   xfsx_static
#   grammar_static
#   ixxxutil_static
#   ixxx_static
#   Qt5::Core
#   Qt5::Widgets
#   Qt5::Test
#   ${XML2_LIB}
#   ${Boost_REGEX_LIBRARY}
#   ${Boost_SYSTEM_LIBRARY}
#   ${Boost_FILESYSTEM_LIBRARY}
#   )
# 
# add_executable(ut_gui
#   test/gui_main.cc
#   test/test.cc
#   test/qt_gui.cc
#   test/editor/gui_command/save.cc
#   test/editor/gui_command/edit.cc
#   test/editor/gui_command/add.cc
#   test/editor/gui_command/clipboard_copy.cc
#   test/editor/gui_command/clipboard_cut.cc
#   test/editor/gui_command/clipboard_paste.cc
#   test/editor/gui_command/display_subtree.cc
#   test/editor/gui_command/write_aci.cc
#   test/editor/gui_command/search.cc
#   test/editor/recent_menu.cc
#   test/editor/main_window.cc
#   test/editor/slider_bar.cc
#   test/editor/instance.cc
#   test/editor/instance_manager.cc
#   test/editor/tree_view.cc
#   test/editor/preview.cc
#   test/editor/delegate/tag.cc
#   test/editor/delegate/value.cc
#   test/editor/delegate/model_validator.cc
# 
#   ${TREE_MODEL_CC}
#   ${EDITOR_CC}
# 
#   ${EDITOR_MOC}
#   ${EDITOR_UI}
#   ${TREE_MOC}
# 
#   ${QT_AWESOME_CC}
#   ${QT_AWESOME_MOC}
#   ${EDITOR_RES}
# 
#   ${CMAKE_SOURCE_DIR}/cppformat/format.cc
#   )
# set_property(TARGET ut_gui PROPERTY INCLUDE_DIRECTORIES
#   ${CMAKE_CURRENT_SOURCE_DIR}/libxxxml
#   ${CMAKE_CURRENT_SOURCE_DIR}
#   ${CMAKE_CURRENT_BINARY_DIR}
#   ${XML2_INCLUDE_DIR}
#   ${CMAKE_CURRENT_SOURCE_DIR}/Catch/single_include
#   ${CMAKE_CURRENT_SOURCE_DIR}/libixxx
#   ${CMAKE_CURRENT_SOURCE_DIR}/libixxxutil
#   ${CMAKE_CURRENT_SOURCE_DIR}/libgrammar
#   ${CMAKE_CURRENT_SOURCE_DIR}/libxfsx
#   ${CMAKE_CURRENT_SOURCE_DIR}/QtAwesome
#   )
# target_link_libraries(ut_gui
#   xxxml_static
#   xfsx_static
#   grammar_static
#   ixxxutil_static
#   ixxx_static
#   Qt5::Core
#   Qt5::Widgets
#   Qt5::Test
#   ${XML2_LIB}
#   ${Boost_REGEX_LIBRARY}
#   ${Boost_SYSTEM_LIBRARY}
#   ${Boost_FILESYSTEM_LIBRARY}
#   )
# 
# # for executing it from a quickfix environment
# add_custom_target(check COMMAND ut)

endif(FALSE)
