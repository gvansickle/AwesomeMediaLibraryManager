#
# Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

# Adapted from https://stackoverflow.com/a/34292622

# Get all propreties that cmake supports
execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)

# Convert command output into a CMake list
STRING(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
STRING(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
# Expand <LANG>
set(outlist "")
foreach(prop ${CMAKE_PROPERTY_LIST})
	#message(STATUS "Processing prop: ${prop}")
	set(lang_found 0)
	string(FIND ${prop} "<LANG>" lang_found)
	if(NOT (${lang_found} EQUAL -1))
		get_cmake_property(langs ENABLED_LANGUAGES)
		#message(STATUS "Found <LANG> in prop: ${prop}, Expanding to languages: ${langs}")
		foreach(lang ${langs})
			set(new_prop ${prop})
			string(REPLACE "<LANG>" ${lang} new_prop ${new_prop})
			list(APPEND outlist ${new_prop})
			#message(STATUS "Expanded: ${new_prop}")
		endforeach()				
	else()
		list(APPEND outlist ${prop})
	endif()
endforeach()
# Dedup.
list(REMOVE_DUPLICATES outlist)
# Allow <CONFIG> (we handle that in the property_get() loop), but remove
# all other properties containing "<whatever>", they won't match anything.
list(FILTER outlist EXCLUDE REGEX "<[^C][^O][^N][^F][^I][^G].*>")
set(CMAKE_PROPERTY_LIST ${outlist})


function(print_properties)
    message ("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction(print_properties)

function(print_target_properties tgt)
    if(NOT TARGET ${tgt})
        message("There is no target named '${tgt}'")
        return()
    endif()

    message(STATUS "*** START print_target_properties(${tgt}) ***")

    foreach (prop ${CMAKE_PROPERTY_LIST})
        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" prop ${prop})
        # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
        if(prop STREQUAL "LOCATION" OR prop MATCHES "^LOCATION_" OR prop MATCHES "_LOCATION$")
            continue()
        endif()
        # message ("Checking ${prop}")
        get_property(propval TARGET ${tgt} PROPERTY ${prop} SET)
        if (propval)
            get_target_property(propval ${tgt} ${prop})
            get_property(prop_brief_docs TARGET ${tgt} PROPERTY ${prop} BRIEF_DOCS) 
            message(STATUS "${tgt} ${prop}: ${prop_brief_docs} == ${propval}")
        endif()
    endforeach(prop)

    message("*** END print_target_properties(${tgt}) ***")

endfunction(print_target_properties)
