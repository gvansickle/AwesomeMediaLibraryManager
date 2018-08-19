/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
 *
 * This file is part of AwesomeMediaLibraryManager.
 *
 * AwesomeMediaLibraryManager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AwesomeMediaLibraryManager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AwesomeMediaLibraryManager.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SRC_GUI_ACTIONS_STANDARDACTIONS_H_
#define SRC_GUI_ACTIONS_STANDARDACTIONS_H_

#include <config.h>

#if HAVE_KF501
/// @link https://api.kde.org/frameworks/kconfigwidgets/html/namespaceKStandardAction.html#a741bca99a57745c202717fa273bc7f9b
#include <KStandardAction>

namespace StandardActions = KStandardAction;

#else

// We'll roll our own.
/// @todo

#endif // HAVE_KF501



#endif /* SRC_GUI_ACTIONS_STANDARDACTIONS_H_ */
