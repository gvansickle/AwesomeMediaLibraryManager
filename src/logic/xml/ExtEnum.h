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

#ifndef AWESOMEMEDIALIBRARYMANAGER_EXTENUM_H
#define AWESOMEMEDIALIBRARYMANAGER_EXTENUM_H

#include <initializer_list>
#include <string>
#include <string_view>
#include <cstring>

//#include <boost/preprocessor.hpp>
//#include <boost/preprocessor/variadic/elem.hpp>

/**
 * A way-improved enum that's been too long in coming to C++.
 * @note Tell me I'm wrong. ;-)
 *
 * - Typesafe.
 * - Representations:
 * -- integer
 * -- string
 * -- ...other?
 * - Sort order can be separated from any of the representations.
 * - Hashable
 * - enum-like representation, i.e. "DerivedExtEnum val = TypeSafeEnumerator1;
 */
/// @note Giving up for the moment and using Better Enums: @link https://github.com/aantron/better-enums
// Better Enums.
#ifndef BETTER_ENUMS_CONSTEXPR_TO_STRING
#define BETTER_ENUMS_CONSTEXPR_TO_STRING
#endif
#include <src/third_party/better_enums/enum.h>

#if 0
struct ExtEnum
{
	struct ExtEnumBase;
	struct ExtEnumHelper;

	constexpr ExtEnum() {};
//	ExtEnum(std::initializer_list<ExtEnumHelper> init_list) {};

};

/**
 * We need this helper struct to give us something to forward declare.
 */
//struct ExtEnumHelper
//{
//	ExtEnumHelper(auto name) {};
//};

// Testing.
struct A {  };
struct B : public A
{
	int m_val;

	static inline constexpr B AMEMA {1};
	static inline constexpr B AMEMB {1};
};

void func()
{
//	auto enumval = B::AMEMA;
}

/**
 * Minimal constexpr compile-time string class.
 */
class const_string
{
private:
	const char* const m_string;
	const std::size_t m_size;

public:
	template<std::size_t N>
	constexpr const_string(const char(&string)[N]) : m_string(string), m_size(N-1) {}

	constexpr const_string(const char* const pString) : m_string {pString}, m_size { std::strlen(pString) } {};

	/// Return the length of the string.
	constexpr std::size_t size() const { return m_size; }

	constexpr const char* operator*() const
	{
		return m_string;
	}
	constexpr const char* c_str() const { return m_string; };

	constexpr bool operator==(const char * cstr) const
	{
		for(int i=0; i<m_size; ++i)
		{
			if(cstr[i] == '\0')
			{
				// Other string ended before we did.  Not equal.
				return false;
			}
			if(cstr[i] != m_string[i])
			{
				// chars at position i not equal.
				return false;
			}
		}

		// All chars matched, equal.
		return true;
	};
};

//template <class DerivedClass>
struct ExtEnumerator
{
//	template<std::size_t N>
//	constexpr ExtEnumerator(const char(&string)[N], uint64_t value, uint64_t sort_index)
//				: m_sort_index(sort_index), m_string(string), m_value(value) {};

	constexpr ExtEnumerator(uint64_t value, uint64_t sort_index) : ExtEnumerator("##### TODO #####", value, sort_index) {};
//			: m_sort_index(sort_index), m_value(value) //, m_string(std::string_view(__func__))
//			 m_string(__func__/*"xxxx"*/) {};
//			{ m_string = const_string(__func__); };

	constexpr ExtEnumerator(const char* string, uint64_t value, uint64_t sort_index)
			: m_sort_index(sort_index), m_string(string), m_value(value) { m_funcname = __func__; };

	/*constexpr*/ const char* c_str() const
	{
		return m_string.c_str();
//		return m_funcname;
	};
	constexpr uint64_t toInt() const { return m_value; };

	// Equality Compare to other ExtEnumerators.
	constexpr bool operator==(const ExtEnumerator& other) const { return m_value == other.m_value; };

	// Equality Compare to integer.
	constexpr bool operator==(uint64_t rhs) const { return m_value == rhs; };

//	template <class DerivedType>
//	constexpr bool operator==(const DerivedType& lhs, const DerivedType& rhs)
//	{
//		if(lhs.)
//	}

	// A value used to sort instances of derived types.
	// For use by containers such as map if you want the ExtEnums ordered
	// differently than by their value or string.
	const uint64_t m_sort_index;

	// The integer value.
	const uint64_t m_value;

	// String representation of the ExtEnumerator instance.
	const_string m_string;

	const char* m_funcname {nullptr};
};

#define M_PRED_IS_EVEN(_, num) BOOST_PP_NOT(BOOST_PP_MOD(d, num, 2))
//#define M_PRED_IS_EVEN(s, data, element)

#define M_EXTENUM_FWD_DECL_ENUMERATOR(r, data, i, elem) BOOST_PP_CAT(data, BOOST_PP_CAT(elem, i))
#define M_EXTENUM_DEFINE_ENUMERATOR(r, data, i, elem)  data elem; //BOOST_PP_CAT(data, BOOST_PP_CAT(elem, i))

#define EXPAND_EXTENUM_VARARGS(...) /* Expand declarations */ __VA_ARGS__

#define DECL_EXTENUM(extenum_name, ...) \
	struct extenum_name : public ExtEnumerator \
	{\
		/*constexpr extenum_name (uint64_t value, uint64_t sort_index) : ExtEnumerator(value, sort_index) {};*/\
		template<std::size_t N> \
		constexpr extenum_name(const char(&string)[N], uint64_t value, uint64_t sort_index) \
			: ExtEnumerator(string, value, sort_index) { }; \
		constexpr extenum_name(uint64_t value, uint64_t sort_index)  \
			: ExtEnumerator( "" , value, sort_index) { }; \
		\
		using extenum_name_ref = extenum_name&;\
		BOOST_PP_SEQ_FOR_EACH_I( M_EXTENUM_DEFINE_ENUMERATOR, inline constexpr extenum_name_ref, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__) );\
		 \
	};

// extenum_name, /* The data passed to each invocation of M_EXTENUM_FWD_DECL_ENUMERATOR() */ \
///*BOOST_PP_SEQ_FILTER( M_PRED_IS_EVEN, nil, */BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__) )\


	/*static inline constexpr extenum_name EXPAND_EXTENUM_VARARGS(__VA_ARGS__);*/

	/* Seq == ()()() */
//BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)
//#define X(x) x
//#define MACRO(p, x) X p x )
//
//auto x = MACRO(BOOST_PP_LPAREN(), abc) // expands to abc
//
//#define Y(x)
//
//MACRO((10) Y BOOST_PP_LPAREN(), result) // expands to 10

//#define EXTENUMERATOR(extenumerator_name)

#if 0

/// Bitwise-or operator for FileCreationFlag.
/// @note Yeah, I didn't realize this was necessary for non-class enums in C++ either.  I've been writing too much C....
constexpr inline FileCreationFlag operator|(FileCreationFlag a, FileCreationFlag b)
{
	return static_cast<FileCreationFlag>(static_cast<std::underlying_type<FileCreationFlag>::type>(a)
	                                     | static_cast<std::underlying_type<FileCreationFlag>::type>(b));
}

inline std::ostream& operator<<(std::ostream& out, const FileType value){
	const char* s = 0;
#define M_ENUM_CASE(p) case(p): s = #p; break;
	switch(value){
		M_ENUM_CASE(FT_UNINITIALIZED);
		M_ENUM_CASE(FT_UNKNOWN);
		M_ENUM_CASE(FT_REG);
		M_ENUM_CASE(FT_DIR);
		M_ENUM_CASE(FT_SYMLINK);
		M_ENUM_CASE(FT_STAT_FAILED);
	}
#undef M_ENUM_CASE

	return out << s;
}
#endif

#endif // 0



#endif //AWESOMEMEDIALIBRARYMANAGER_EXTENUM_H
