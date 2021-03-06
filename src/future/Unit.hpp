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

#ifndef AWESOMEMEDIALIBRARYMANAGER_UNIT_H
#define AWESOMEMEDIALIBRARYMANAGER_UNIT_H

#include <type_traits>
#include <functional>


/**
 * "Unit" vs. "void" concept from Facebook's "folly" library (Apache 2.0).
 *
 * "In functional programming, the degenerate case is often called "unit". In
/// C++, "void" is often the best analogue. However, because of the syntactic
/// special-casing required for void, it is frequently a liability for template
/// metaprogramming. So, instead of writing specializations to handle cases like
/// SomeContainer<void>, a library author may instead rule that out and simply
/// have library users use SomeContainer<Unit>. Contained values may be ignored.
/// Much easier.
///
/// "void" is the type that admits of no values at all. It is not possible to
/// construct a value of this type.
/// "unit" is the type that admits of precisely one unique value. It is
/// possible to construct a value of this type, but it is always the same value
/// every time, so it is uninteresting."
///
 * From http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0146r1.html:
 * "To stress [the need for void to be a real type], as a generic programmer I have used a variety of library-based techniques
 * over the years to help deal with void. The results of such techniques, while "better" than handling each case individually,
 *  are still not for the faint of heart due to the dependence on advanced C++ techniques and the fact that those techniques
 *  need to be explicitly employed by the developer of the generic code. A brief outline of the most successful of those techniques,
 *  in my experience, revolves around the following methodology:
 *
 *  1. Introduce a Void type that is just a Regular unit type.
 *  2. Introduce metafunctions and high-order functions that are already special-cased for void.
 *      - A metafunction that is an identity metafunction for all types except cv-void, in which case it converts to cv-Void
 *      - The inverse of the above metafunction
        - A version of std::invoke that returns Void instead of void when invoking a Callable that returns void
        - A high-order function that takes a unary or nullary function "F" and a separate function "I" to invoke, then invokes "F" with the result of "I", unless "I" returns void or "F" is only able to be invoked with no arguments, in which case, "I" is invoked followed by "F" being invoked with no arguments.
        - Other, more obscure high-order functions
    3. Write the bulk of the internals of generic code as normal -- do not try to account for void.
    4. At the interfaces between the generic code and the user, use the aforementioned metafunctions and high-order functions to avoid explicit branching and top-level specialization.
    5. Continue special-casing in the remaining places where the library-facilities aren't enough.
 * "
 */
struct Unit
{
    // These are structs rather than type aliases because MSVC 2017 RC has
    // trouble correctly resolving dependent expressions in type aliases
    // in certain very specific contexts, including a couple where this is
    // used. See the known issues section here for more info:
    // https://blogs.msdn.microsoft.com/vcblog/2016/06/07/expression-sfinae-improvements-in-vs-2015-update-3/

    // "Lift": Convert type T into Unit if it's void, T otherwise.
    template <typename T>
	struct Lift : std::conditional<std::is_same<T, void>::value, Unit, T> {};
    template <typename T>
    using LiftT = typename Lift<T>::type;

    // "Drop": Convert type into void if it's Unit, T otherwise.
    template <typename T>
    struct Drop : std::conditional<std::is_same<T, Unit>::value, void, T> {};
    template <typename T>
    using DropT = typename Drop<T>::type;

    constexpr bool operator==(const Unit& /*other*/) const {
    return true;
    }
    constexpr bool operator!=(const Unit& /*other*/) const {
    return false;
    }
};

constexpr Unit unit {};

template <class F, class... Args>
auto std_invoke_and_lift(F&& f, Args&&... args) -> Unit::LiftT< std::invoke_result_t<F, Args...> >
{
	using R = Unit::LiftT< std::invoke_result_t<F, Args...> >;
	if constexpr(std::is_convertible_v<R, Unit>)
	{
		std::invoke(std::move(DECAY_COPY(f)), args...);
		return unit;
	}
	else
	{
		return std::invoke(std::move(DECAY_COPY(f)), args...);
	}
};


#endif //AWESOMEMEDIALIBRARYMANAGER_UNIT_H
