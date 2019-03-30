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

/**
 * @file preproc.h  As long as C++ preprocessor shenanigans like this are necessary, the C++ preprocessor will
 *                  not only survive, it will prevail.
 */

#ifndef SRC_FUTURE_PREPROC_H_
#define SRC_FUTURE_PREPROC_H_

#include <cstddef>

namespace detail
{
	template<typename ...Args>
    constexpr std::size_t va_nargs(Args&&...) { return sizeof...(Args); }
}

#define PP_VA_NARGS(...) detail::va_nargs(__VA_ARGS__)

/// Two-level token-pasting helpers.  Params are expanded before token pasting.
#define TOKENPASTE2(a, b) a ## b
#define TOKENPASTE(a, b) TOKENPASTE2(a, b)
#define CAT_BASE(a, ...) a ## __VA_ARGS__
#define CAT(a, ...) CAT_BASE(a, __VA_ARGS__)


/// @name Preprocessor helpers for string expansion etc.
/// Used in M_WARNING() and elsewhere.
/// @{
#define STRINGISE_IMPL(x) #x
#define STRINGISE(x) STRINGISE_IMPL(x)
//#define FILE_LINE_LINK __FILE__ "(" STRINGISE(__LINE__) "): "
#define DEFER(M, ...) M(__VA_ARGS__)
/// @}

#endif /* SRC_FUTURE_PREPROC_H_ */
