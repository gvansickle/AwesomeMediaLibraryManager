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

#ifndef UTILS_NOMOC_NOMOCIMPL_H_
#define UTILS_NOMOC_NOMOCIMPL_H_

/**
 * @file
 *
 * Wrapper for Verdigris, the "Qt5 minus moc" library.  Lets us build with or without it.
 *
 * @link https://code.woboq.org/woboq/verdigris/tutorial/tutorial.cpp.html
 */

#ifdef USE_BUNDLED_VERDIGRIS
#include <wobjectimpl.h>
#else
#define W_OBJECT_IMPL(...) /* nothing */
#define W_GADGET_IMPL(...) /* nothing */
#define W_NAMESPACE_IMPL(...) /* nothing */
#endif

#endif /* UTILS_NOMOC_NOMOCIMPL_H_ */
