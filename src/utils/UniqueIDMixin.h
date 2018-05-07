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

#ifndef SRC_UTILS_UNIQUEIDMIXIN_H_
#define SRC_UTILS_UNIQUEIDMIXIN_H_

/// Standard C++
#include <atomic>

/// Qt5
#include <QString>

template <typename T>
class UniqueIDMixin
{
	static std::atomic_uintmax_t m_next_id_num;

	uintmax_t m_id_num;

public:
	UniqueIDMixin()
	{
		m_id_num = m_next_id_num;
		m_next_id_num++;
	}

    QString uniqueQObjectName() const { return T::staticMetaObject.className() + QString("_") + id(); }
	QString id() const { return QString("%1").arg(m_id_num); }
};

//
template <typename T>
std::atomic_uintmax_t UniqueIDMixin<T>::m_next_id_num;

#endif /* SRC_UTILS_UNIQUEIDMIXIN_H_ */
