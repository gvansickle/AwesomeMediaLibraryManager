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
 * @file overloaded.h
 *
 * Basic work-alike support for the possible std::overload coming soon to a C++ compiler near you.
 * @link http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0051r3.pdf
 */
#ifndef SRC_FUTURE_OVERLOADED_H_
#define SRC_FUTURE_OVERLOADED_H_

#include <functional>
#include <variant>


/**
 * The "overloaded" trick for lambda dispatch over std::variant<> using std::visit<>.
 *
 * @note This doesn't do the std::reference stuff from here:
 * 		@link https://arne-mertz.de/2018/05/overload-build-a-variant-visitor-on-the-fly/
 * 		It's what's shown here:
 * 		@link https://en.cppreference.com/w/cpp/utility/variant/visit
 *
 * @note And these are all broken with C++17:
 * @link https://github.com/viboes/std-make/issues/16
 */
template <class... Fs>
struct overloaded : Fs... {
  template <class ...Ts>
  overloaded(Ts&& ...ts) noexcept : Fs{std::forward<Ts>(ts)}...
  {}

  using Fs::operator()...;
};

/// Deduction guide for the above template.
template<class... Ts>
overloaded(Ts&&...) -> overloaded<std::remove_reference_t<Ts>...>;


#endif /* SRC_FUTURE_OVERLOADED_H_ */
