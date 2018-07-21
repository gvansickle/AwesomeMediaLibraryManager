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
 * @file
 * Tests for the function_traits.hpp header.
 * All compile-time tests, no runtime hit.
 */

#include "function_traits.hpp"

// Std C++
#include <utility>

// Std C++ backfill.
#include <future/future_type_traits.hpp>

// Ours
#include <utils/DebugHelpers.h>

namespace function_traits_impl
{
	namespace test
	{

		// Normal free function taking/returning POD types.
		int ft_test_func1(long a) { return static_cast<int>(a); };

		using functraits1 = function_traits<decltype(ft_test_func1)>;
		static_assert(std::is_same_v<functraits1::return_type_t, int>, "Wrong return type");
		static_assert(std::is_same_v<functraits1::arg_t<0>, long>, "Wrong type for arg 0");
		static_assert(functraits1::arity_v == 1, "Wrong number of args");
		static_assert(functraits1::return_type_is_v<int>, "Wrong return type");
		static_assert(functraits1::argtype_is_v<0, int> == false, "Function does not take int as arg 0");
		static_assert(functraits1::argtype_is_v<0, long> == true, "Function takes long as arg 0, but check failed");

		// Class with member functions.
		class ft_test2_class
		{
		public:
			ft_test2_class* ft_test2a_member_fn(double d) { return this; };

            void* ft_test2a_const_member_fn() const { return (void*)this; };

			void ft_test2b_returns_void(void) { volatile int a; a = 2; };
		};

		using functraits2 = function_traits<decltype(&ft_test2_class::ft_test2a_member_fn)>;
		static_assert(std::is_same_v<functraits2::return_type_t, ft_test2_class*>, "Wrong return type");
		static_assert(functraits2::arity_v == 2, "Wrong number of args");
		static_assert(std::is_same_v<functraits2::arg_t<1>, double>, "Wrong type for arg 1");
		static_assert(functraits2::argtype_is_v<0, ft_test2_class&>, "Function does not take correct this ptr type as arg 0");

		using ft_test2b_returns_void_traits = function_traits<decltype(&ft_test2_class::ft_test2b_returns_void)>;
		static_assert(ft_test2b_returns_void_traits::arity_v == 1, "Wrong number of args");
		static_assert(ft_test2b_returns_void_traits::return_type_is_v<void>, "Wrong return type");
		static_assert(ft_test2b_returns_void_traits::argtype_is_v<0, ft_test2_class&>, "Function does not take correct this ptr type as arg 0");

		using functraits3 = function_traits<decltype(&ft_test2_class::ft_test2a_const_member_fn)>;
		static_assert(std::is_same_v<functraits3::return_type_t, void*>, "Wrong return type");
		static_assert(functraits3::arity_v == 1, "Wrong number of args");
		static_assert(functraits3::return_type_is_v<void*>, "Wrong return type");
		static_assert(functraits3::argtype_is_v<0, const ft_test2_class&>, "Function does not take correct this ptr type as arg 0");

		// Lambda.
		auto lambda1 = [](const char *str){ return str;};
		using lambda1_traits = function_traits<decltype(lambda1)>;
		static_assert(std::is_same_v<lambda1_traits::return_type_t, const char*>, "Wrong return type");
		static_assert(std::is_same_v<lambda1_traits::arg_t<0>, const char*>, "Wrong type for arg 0");
		static_assert(lambda1_traits::arity_v == 1, "Wrong number of args");
		static_assert(lambda1_traits::return_type_is_v<const char*>, "Wrong return type");
		static_assert(lambda1_traits::argtype_is_v<0, const char*>, "Wrong type for arg 0");

		// Convenience templates.
		static_assert(function_return_type_is_v<decltype(lambda1), const char*>, "Wrong return type");

		template <typename T>
		struct test_struct
		{
			using internal_type = T;
		};

		test_struct<long> test_struct_longs;
		int test_var;
		static_assert(std::is_same_v<contained_type_t<decltype(test_struct_longs)>, long>, "Wrong contained type");
//		static_assert(std::is_same_v<!contained_type_t<decltype(test_var)>, long>, "Wrong contained type");
//		static_assert(std::is_same_v<argtype_t<decltype(test_struct_longs), 0>, long>, "Wrong contained type");
	}

}


