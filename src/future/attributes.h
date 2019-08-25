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
 * @file attributes.h
 */
#ifndef SRC_FUTURE_ATTRIBUTES_H_
#define SRC_FUTURE_ATTRIBUTES_H_


#define ATTR_DIAGNOSE_IF(...) __attribute__((diagnose_if(__VA_ARGS__)))

/**
 * GCC: "no observable effects on the state of the program other than to return a value".
 * - More restrictive than "pure".
 * - "prohibits a function from modifying the state of the program that is observable by means other than inspecting the function’s return value."
 * - "can safely read any non-volatile objects, and modify the value of objects in a way that does not affect their return value
 *    or the observable state of the program."
 * - Vs. const: "pure allows the function to read any non-volatile memory, even if it changes in between successive invocations of the function."
 * - "Because a pure function cannot have any observable side effects it does not make sense for such a function to return void.
 *    Declaring such a function is diagnosed. "
 * @link https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#Common-Function-Attributes
 */
#define ATTR_PURE() __attribute__((pure))

/**
 * GCC: "[R]eturn value is not affected by changes to the observable state of the program and that have no observable effects
 * on such state other than to return a value".
 * - More restrictive than "pure":
 * - "Note that a function that has pointer arguments and examines the data pointed to must not be declared const if the pointed-to data
 *    might change between successive invocations of the function. In general, since a function cannot distinguish data that might change
 *    from data that cannot, const functions should never take pointer or, in C++, reference arguments. Likewise, a function that
 *    calls a non-const function usually must not be const itself."
 * - "prohibits a function from reading objects that affect its return value between successive invocations."
 * - "can safely read objects that do not change their return value, such as non-volatile constants."
 * @link https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#Common-Function-Attributes
 */
#define ATTR_CONST() __attribute__((const))

#define ATTR_ARTIFICIAL() __attribute__((artificial))

/**
 * To allow debugging even when all optimizations are disabled.
 * @note Un. Be. Lievable.
 * @link https://stackoverflow.com/questions/28287064/how-not-to-optimize-away-mechanics-of-a-folly-function?noredirect=1&lq=1
 * @link https://answers.launchpad.net/gcc-arm-embedded/+question/280104
 */
/*
 * Call ATTR_VAR_MAX_DEBUG(var) against variables that you use for
 * benchmarking but otherwise are useless [GRVS: or that you want to see in the debugger regardless of usage].
 * The compiler tends to do a good job at eliminating unused [GRVS: and used] variables, and this function fools
 * it into thinking var is in fact needed.
 */
#ifdef _MSC_VER
#pragma optimize("", off)

template <class T>
void ATTR_VAR_MAX_DEBUG(T&& var)
{
	var = var;
}

#pragma optimize("", on)

#else
//#define ATTR_VAR_MAX_DEBUG(var) \
//	__attribute__((used))\
//	ATTR_VAR_MAX_DEBUG_TMPL(var)
template <class T>
void ATTR_VAR_MAX_DEBUG/*_TMPL*/(T&& var)
{
	__asm__ __volatile__ ("" :: "m" (var));
}
#endif

#endif /* SRC_FUTURE_ATTRIBUTES_H_ */
