/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/**
 * @file guideline_helpers.h
 *
 * Various helpers for DRY'ing some C++ Core Guidelines @link http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines
 * and other boilerplate, best practices, etc.
 */
#ifndef SRC_FUTURE_GUIDELINE_HELPERS_H_
#define SRC_FUTURE_GUIDELINE_HELPERS_H_

/**
 * Suppress copyability of a polymorphic class to eliminate slicing.
 * @link http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c67-a-polymorphic-class-should-suppress-copying
 */
#define M_GH_POLYMORPHIC_SUPPRESS_COPYING_C67(classname) \
	classname(const classname&) = delete; \
	classname& operator=(const classname&) = delete;

#endif /* SRC_FUTURE_GUIDELINE_HELPERS_H_ */
