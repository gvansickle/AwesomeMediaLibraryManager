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

#ifndef UTILS_NOMOC_NOMOCDEFS_H_
#define UTILS_NOMOC_NOMOCDEFS_H_

#ifdef USE_BUNDLED_VERDIGRIS
#include <wobjectdefs.h>
#else
#define W_OBJECT(classname) Q_OBJECT
#define W_SIGNAL(...) ;
#define W_SLOT(...) /* nothing */
#endif

#endif /* UTILS_NOMOC_NOMOCDEFS_H_ */