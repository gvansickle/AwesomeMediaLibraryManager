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

set(HEADER_FILES_GUI_ACTIVITYPROGRESSMANAGER
	activityprogressmanager/ActivityProgressPopup.h
	activityprogressmanager/ActivityProgressStatusBarTracker.h
	activityprogressmanager/BaseActivityProgressStatusBarWidget.h
	activityprogressmanager/ExpandingFrameWidget.h
	activityprogressmanager/CumulativeStatusWidget.h
	activityprogressmanager/CumulativeAMLMJob.h
	PARENT_SCOPE
	)
set(SOURCE_FILES_GUI_ACTIVITYPROGRESSMANAGER
	activityprogressmanager/ActivityProgressPopup.cpp
	activityprogressmanager/ActivityProgressStatusBarTracker.cpp
	activityprogressmanager/BaseActivityProgressStatusBarWidget.cpp
	activityprogressmanager/ExpandingFrameWidget.cpp
	activityprogressmanager/CumulativeStatusWidget.cpp
	activityprogressmanager/CumulativeAMLMJob.cpp
	PARENT_SCOPE
	)
set(UI_FILES_GUI_ACTIVITYPROGRESSMANAGER
	PARENT_SCOPE
	)

### @todo Remove dirs above, use these instead.
# prepend_to_each(HEADER_FILES_GUI_ACTIVITYPROGRESSMANAGER "gui/" ${HEADER_FILES_GUI_ACTIVITYPROGRESSMANAGER})
# prepend_to_each(SOURCE_FILES_GUI_ACTIVITYPROGRESSMANAGER "gui/" ${SOURCE_FILES_GUI_ACTIVITYPROGRESSMANAGER})
# prepend_to_each(UI_FILES_GUI_ACTIVITYPROGRESSMANAGER "gui/" ${UI_FILES_GUI_ACTIVITYPROGRESSMANAGER})
