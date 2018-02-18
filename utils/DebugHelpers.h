/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef DEBUGHELPERS_H
#define DEBUGHELPERS_H

#include <type_traits>

#include <QString>
#include <QDebug>
#include <QThread>

#include "StringHelpers.h"

/**
 * Streaming operator for qDebug() << std::string.
 */
inline static QDebug& operator<<(QDebug& d, const std::string& s)
{
	return d << toqstr(s);
}

/**
 * Stream to qDebug() to log the current thread name.
 */
#define M_THREADNAME() "[" << QThread::currentThread()->objectName() << "]"

/**
 * qDebug() etc. replacements which prepends the current thread name.
 */
#define qDb() qDebug() << M_THREADNAME()
#define qIn() qInfo() << M_THREADNAME()
#define qWr() qWarning() << M_THREADNAME()
#define qCr() qCritical() << M_THREADNAME()

template <typename T>
static inline auto idstr(const char *id_as_c_str, T id) -> std::enable_if_t<std::is_convertible<T, std::string>::value == true, std::string> /// SFINAE version for T already convertible to std::string.
{
	return std::string(id_as_c_str) + "(" + id + ")";
}

template <typename T>
static inline
std::enable_if_t<std::is_convertible<T, std::string>::value == false, std::string> /// SFINAE version for T not convertible to std::string.
idstr(const char *id_as_c_str, T id)
{
	return std::string(id_as_c_str) + "(" + std::to_string(id) + ")";
}

#define M_IDSTR(id) idstr(#id ": ", id) + ", " +

/// Attempts to get the compiler to print a human-readable type at compile time.
/// @note In the 21st century, this should be a solved problem.  It isn't.
/// @see https://stackoverflow.com/a/46339450, https://stackoverflow.com/a/30276785
//		typedef typename ft_test2_class::something_made_up X;
//		bool x = decltype(&ft_test2_class::ft_test2_class)::nothing;
template<typename T>
void print_type_in_compilation_error(T&&)
{
	static_assert(std::is_same<T, int>::value && !std::is_same<T, int>::value, "Compilation failed because you wanted to read the type. See below");
}
/// Usage: use this anywhere to print a type name.
/// Prints as a side-effect of failing the compile.
/// @note Only works if t is a variable.
//#define M_PRINT_TYPE_IN_ERROR(t) void dummy(void) { print_type_in_compilation_error((t)); }
//#define M_PRINT_TYPE_IN_ERROR(T) typedef typename T::something_made_up X;
#define M_PRINT_TYPEOF_VAR_IN_ERROR(v) bool x = decltype((v))::no_such_member_so_you_can_see_the_type_name;

/// @name Preprocessor helpers for M_WARNING().
/// @{
#define STRINGISE_IMPL(x) #x
#define STRINGISE(x) STRINGISE_IMPL(x)
#define FILE_LINE_LINK __FILE__ "(" STRINGISE(__LINE__) "): "
/// @}

/**
 * Portable compile-time warning message.
 * Use: M_WARNING("My message")
 */
#if defined(_MSC_VER)
#   define M_WARNING(exp) __pragma(message(FILE_LINE_LINK "warning C2660: " exp))
#elif defined(__clang__)
#   define DEFER(M, ...) M(__VA_ARGS__)
#   define M_WARNING(X) _Pragma(STRINGISE_IMPL(GCC warning(X " at line " DEFER(STRINGISE_IMPL, __LINE__))))
#elif defined(__GNUC__) || defined(__GNUCXX__)
#	define DO_PRAGMA(x) _Pragma(#x)
#   define M_WARNING(exp) DO_PRAGMA(message FILE_LINE_LINK "warning: " exp)
#endif

#endif // DEBUGHELPERS_H
