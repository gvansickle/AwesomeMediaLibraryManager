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

#define M_GH_RULE_OF_ZERO(classname) /* Nothing we can really do here, just for documentation purposes. */

#define IMPL_RULE_OF_FIVE(classname, default_or_delete) \
	/** Default constructor. */ \
	classname() = default_or_delete; \
	/** Copy constructor. */ \
	classname(const classname&) = default_or_delete; \
	/** Copy assignment. */ \
	classname& operator=(const classname&) = default_or_delete; \
	/** Move constructor. */ \
	classname(classname&&) = default_or_delete; \
	/** Move assignment. */ \
	classname& operator=(classname&&) = default_or_delete; \


#define M_GH_RULE_OF_FIVE_DEFAULT_C21(classname) IMPL_RULE_OF_FIVE(classname, default)
#define M_GH_RULE_OF_FIVE_DELETE_C21(classname) IMPL_RULE_OF_FIVE(classname, delete)

/**
 * @todo Rules for passing smart ptrs.
 * @link http://www.modernescpp.com/index.php/c-core-guidelines-passing-smart-pointer
 * "
 *	R.32: Take a unique_ptr<widget> parameter to express that a function assumes ownership of a widget
 *  R.33: Take a unique_ptr<widget>& parameter to express that a function reseats the widget
 *	R.34: Take a shared_ptr<widget> parameter to express that a function is part owner
 *	R.35: Take a shared_ptr<widget>& parameter to express that a function might reseat the shared pointer
 *	R.36: Take a const shared_ptr<widget>& parameter to express that it might retain a reference count to the object ???
 *	R.37: Do not pass a pointer or reference obtained from an aliased smart pointer
 * "
 */


#endif /* SRC_FUTURE_GUIDELINE_HELPERS_H_ */
