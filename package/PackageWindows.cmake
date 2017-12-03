#
# Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

add_custom_target(do_windeploy)
add_dependencies(packaging do_windeploy)
add_dependencies(do_windeploy ${PROJECT_NAME})

add_custom_command(TARGET do_windeploy POST_BUILD
    COMMENT "Doing windeploy..."
    COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Scanning for Qt dependencies, exe: $<TARGET_FILE:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/windeployqt_stuff
    COMMAND $ENV{QTDIR}/bin/windeployqt.exe --compiler-runtime --dir ${CMAKE_BINARY_DIR}/windeployqt_stuff $<TARGET_FILE:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Scanning for Qt dependencies complete.  Intermediate directory is: ${CMAKE_BINARY_DIR}/windeployqt_stuff"
    )
install(
        DIRECTORY ${CMAKE_BINARY_DIR}/windeployqt_stuff/
        DESTINATION .
        COMPONENT coreapp
        )

#macro(do_windeploy arg_coreapp_target arg_install_component)
#    if(WIN32)
#        message(STATUS "Setting up Qt-related dependencies for installer.")
#        if(EXISTS $ENV{QTDIR}/bin/windeployqt.exe)
#                add_custom_command(
#                            TARGET ${arg_coreapp_target} POST_BUILD
#                                COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Scanning for Qt dependencies, exe: $<TARGET_FILE:${arg_coreapp_target}>"
#                                COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/windeployqt_stuff
#                                COMMAND $ENV{QTDIR}/bin/windeployqt.exe --compiler-runtime --dir ${CMAKE_BINARY_DIR}/windeployqt_stuff $<TARGET_FILE:${arg_coreapp_target}>
#                                COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Scanning for Qt dependencies complete.  Intermediate directory is: ${CMAKE_BINARY_DIR}/windeployqt_stuff"
#                                )

#                install(
#                        DIRECTORY ${CMAKE_BINARY_DIR}/windeployqt_stuff/
#                        DESTINATION .
#                        COMPONENT ${arg_install_component}
#                        )
#                message(STATUS "Qt-related dependencies set up complete.  Intermediate directory is ${CMAKE_BINARY_DIR}/windeployqt_stuff")
#        else()
#                message("Unable to find executable QTDIR/bin/windeployqt.")
#        endif()
#    endif()
#endmacro()
