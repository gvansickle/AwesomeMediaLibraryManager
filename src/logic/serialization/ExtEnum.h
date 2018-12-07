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
#include <map>
#include <cstring>

/**
 * A way-improved enum facility that's been too long in coming to C++.
 * @note Tell me I'm wrong. ;-)
 * @note Also, Better Enums doesn't work with Q_DECLARE_METATYPE, but is otherwise awesome.
 *          @link http://aantron.github.io/better-enums/index.html
 *
 * Aspiring to, but not sure we'll get there:
 * - Typesafe.
 * - Representations:
 * -- integer
 * -- string
 * -- ...other?
 * - Maps
 * - @todo Sort order can be separated from any of the representations.
 * - Hashable
 * - As close as practical to built-in enum-like syntax and semantics, e.g. "DerivedExtEnum val = TypeSafeEnumerator1;"
 */

// Fwd declaration for ExtEnum->SomethingElse maps.
template <class ScopeTypeEnumType, class ToType>
struct ExtEnumMapBase;

/**
 *
 */
template <class DerivedType>
class ExtEnum
{
public:
	/**
     * Static map factory function.
     * @returns An object mapping ScopeTypeEnumType's to ToType's via the at(index) and operator[](index) member
     *          functions.
     */
	template <class ScopeTypeEnumType, class ToType>
	static ExtEnumMapBase<ScopeTypeEnumType, ToType>
	make_map(std::initializer_list<typename ExtEnumMapBase<ScopeTypeEnumType, ToType>::maptype::value_type> init_list)
	{
		return ExtEnumMapBase<ScopeTypeEnumType, ToType>(init_list);
	}

};

/**
 * Base class for ExtEnum-indexed maps.
 *
 * @tparam ScopeTypeEnumType
 * @tparam ToType
 */
template <class ScopeTypeEnumType, class ToType>
struct ExtEnumMapBase
{
	using maptype = std::map<ScopeTypeEnumType, ToType>;
public:
	ExtEnumMapBase(std::initializer_list<typename maptype::value_type> init_list)
		: m_ExtEnum_to_ToType_map(init_list) { };

	/**
	 * Both operator[] and .at() call the underlying map's .at() function.  This
	 * prevents accidentally adding default constructed elements by throwing
	 * an exception regardless of how the map is indexed.
	 *
	 * @param i
	 * @return
	 */
	const ToType operator[](ScopeTypeEnumType i) const { return m_ExtEnum_to_ToType_map.at(i); };
	const ToType at(ScopeTypeEnumType i) const { return m_ExtEnum_to_ToType_map.at(i); };

private:
	const std::map<ScopeTypeEnumType, ToType> m_ExtEnum_to_ToType_map;

};

/**
 * Static map factory function.
 * @return
 */
template <class ScopeTypeEnumType, class ToType>
inline static ExtEnumMapBase<ScopeTypeEnumType, ToType>
make_map(std::initializer_list<typename ExtEnumMapBase<ScopeTypeEnumType, ToType>::maptype::value_type> init_list)
{
	return ExtEnumMapBase<ScopeTypeEnumType, ToType>(init_list);
}

#if 0
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
#endif // 0

#endif //AWESOMEMEDIALIBRARYMANAGER_EXTENUM_H
