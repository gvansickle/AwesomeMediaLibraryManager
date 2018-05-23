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
	activityprogressmanager/ActivityProgressManager.h
	activityprogressmanager/ActivityProgressDialog.h
	activityprogressmanager/ActivityProgressPopup.h
	activityprogressmanager/ActivityEntryWidget.h
	activityprogressmanager/ActivityProgressStatusBarTracker.h
	activityprogressmanager/ActivityProgressWidget.h
	activityprogressmanager/BaseActivityProgressStatusBarWidget.h
	activityprogressmanager/ExpandingFrameWidget.h
	activityprogressmanager/CumulativeStatusWidget.h
	PARENT_SCOPE
	)
set(SOURCE_FILES_GUI_ACTIVITYPROGRESSMANAGER
	activityprogressmanager/ActivityProgressManager.cpp
	activityprogressmanager/ActivityProgressDialog.cpp
	activityprogressmanager/ActivityProgressPopup.cpp
	activityprogressmanager/ActivityEntryWidget.cpp
	activityprogressmanager/ActivityProgressStatusBarTracker.cpp
	activityprogressmanager/ActivityProgressWidget.cpp
	activityprogressmanager/BaseActivityProgressStatusBarWidget.cpp
	activityprogressmanager/ExpandingFrameWidget.cpp
	activityprogressmanager/CumulativeStatusWidget.cpp
	PARENT_SCOPE
	)
set(UI_FILES_GUI_ACTIVITYPROGRESSMANAGER
	activityprogressmanager/ActivityProgressDialog.ui
	PARENT_SCOPE
	)

