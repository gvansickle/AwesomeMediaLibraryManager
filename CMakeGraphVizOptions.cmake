#
# Copyright 2018, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

# GraphViz settings

set(GRAPHVIZ_GRAPH_NAME "AMLMDeps")
#set(GRAPHVIZ_GRAPH_HEADER "node [n fontsize = “12”];") # "AMLM Target Dependency Graph"
set(GRAPHVIZ_GRAPH_TYPE "strict digraph") ## max one line between each node in each direction
# Regexes
set(GRAPHVIZ_IGNORE_TARGETS "")
# Set this to FALSE to exclude per target graphs foo.dot.<target>.
# GRAPHVIZ_GENERATE_PER_TARGET
# Set this to FALSE to exclude depender graphs foo.dot.<target>.dependers.
# GRAPHVIZ_GENERATE_DEPENDERS