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
 * @file grvs_functional.hpp
 */

#ifndef SRC_FUTURE_GRVS_FUNCTIONAL_HPP_
#define SRC_FUTURE_GRVS_FUNCTIONAL_HPP_


/**
 * A general-purpose control-flow construct for calling a lambda with a pointer-like argument
 * only if that argument is non-nullptr.
 *
 * If @arg ptr is not nullptr, runs lambda @arg l, passing it @arg ptr as a param.
 * Otherwise @arg l() is not called.
 *
 * @param ptr
 * @param l
 */
template <typename PointerType, typename Lambda>
void with_ptr_or_skip(PointerType ptr, Lambda l)
{
    if(ptr)
    {
        l(ptr);
    }
}

#endif /* SRC_FUTURE_GRVS_FUNCTIONAL_HPP_ */
