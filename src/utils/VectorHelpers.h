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

#ifndef VECTORHELPERS_H
#define VECTORHELPERS_H

// Std C++
//#include <algorithms>
#include <iterator>

// Qt5
#include <QVector>

namespace stdex
{

	/**
	 * Analog of Qt5's T QVector::value(int i) const for any type with an .at() operator.
	 * @param i  Index.
	 * @returns The value at index i, or if i is out-of-bounds, a default-constructed value of type T.
	 *          Does not add the value to the container.
	 */
	template<class ContainerOfT, class T = typename ContainerOfT::value_type>
	T value(ContainerOfT& cont, int i)
	{
		try
		{
			return cont.at(i);
		}
		catch(std::out_of_range& e)
		{
			return T();
		}
	};

	/**
	 * Specialization for QVector<T>.
	 */
	template<class T>
	T value(QVector<T>& cont, int i)
	{
		return cont.value(i);
	};

	template <class ContainerOfT,
			  class T = typename ContainerOfT::value_type,
			  class SizeT = typename ContainerOfT::size_type>
	int indexOf(const ContainerOfT& cont, const T& value, SizeT from = 0)
	{
		auto result = std::find(std::cbegin(cont), std::cend(cont), value);
		if(result != std::cend(cont))
		{
			// Found it.
			return std::distance(std::cbegin(cont), result);
		}
		else
		{
			// Didn't find it.
			// Qt5 returns -1 in this case.
			return -1;
		}
	}

	template <class ContainerOfT,
			  class T = typename ContainerOfT::value_type,
			  class SizeT = typename ContainerOfT::size_type>
	T takeAt(ContainerOfT& cont, SizeT i)
	{
		auto it = std::begin(cont);
		std::advance(it, i);
		T retval = *it;
		cont.erase(it);
		return retval;
	}
};

#endif // VECTORHELPERS_H
