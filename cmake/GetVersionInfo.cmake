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

find_package(Git)
if(GIT_FOUND)
    message(STATUS "Git found: ${GIT_EXECUTABLE}")
    message(STATUS "Git version: ${GIT_VERSION_STRING}")
    execute_process(
            COMMAND "${GIT_EXECUTABLE}" describe --always --long --dirty
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            OUTPUT_VARIABLE GVI_GIT_DESCRIBE_OUTPUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+" GVI_VERSION_TAG_QUAD ${GVI_GIT_DESCRIBE_OUTPUT})
    string(REGEX REPLACE "-g([0-9a-fA-F]+)" "\\1" DUMMY ${GVI_GIT_DESCRIBE_OUTPUT})
    set(GVI_VERSION_HASH ${CMAKE_MATCH_1})
    if(${GVI_GIT_DESCRIBE_OUTPUT} MATCHES "-dirty")
        set(GVI_VERSION_IS_DIRTY TRUE)
        set(GVI_VERSION_DIRTY_POSTFIX "-dirty")
    else()
        set(GVI_VERSION_IS_DIRTY FALSE)
        set(GVI_VERSION_DIRTY_POSTFIX "")
    endif()
    message(STATUS "git describe reports: ${GVI_GIT_DESCRIBE_OUTPUT} (${GVI_VERSION_TAG_QUAD}/${GVI_VERSION_HASH}/${GVI_VERSION_DIRTY_POSTFIX})")
    set(GVI_PROJECT_VERSION "0.0.0.0")
else()
    message(STATUS "Git not found")
    set(GVI_PROJECT_VERSION "0.0.0.0")
endif()
