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

#ifndef IN_H
#define IN_H

namespace impl
{
	template<typename C, typename K>
	auto in_impl(C const& c, K const& key, int) -> decltype(c.find(key), true)
	{
		return c.find(key) != c.end();
	}

	template<typename C, typename K>
	bool in_impl(C const& c, K const& key, ...)
	{
		using std::begin;
		using std::end;
		return std::find(begin(c), end(c), key) != end(c);
	}
}

template<typename C, typename K>
bool in(C const& c, K const& k)
{
	return impl::in_impl(c, k, 0);
}

#endif // IN_H
