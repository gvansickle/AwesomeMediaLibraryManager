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
#include <mutex>
#include <unordered_set>

/// Qt5
#include <QString>

/// Ours
#include <utils/crtp.h>
#include <utils/DebugHelpers.h>

template <typename T>
class UniqueIDMixin : crtp<T, UniqueIDMixin>
{
	using atomic_uuid_type = std::atomic_uintmax_t;

	/// The per-type ID counter.
	static atomic_uuid_type m_next_id_num;

	static std::mutex m_deleted_ids_mutex;

	/// The already-deleted map for this type.
    static std::unordered_set<uintmax_t> m_deleted_ids;

	/// The private unique ID for the instance of the class we're mixed-in to.
	uintmax_t m_id_num;

public:
	virtual ~UniqueIDMixin()
	{
		std::unique_lock<std::mutex> lock(m_deleted_ids_mutex);

		// Record that this ID has been deleted.
        auto retval = m_deleted_ids.insert(m_id_num);
        if(retval.second == false)
        {
            // Was already in the map.
            // It's already been deleted.
            Q_ASSERT_X(0, "", "DOUBLE DELETE");
        }
        else
        {
            qDb() << "No double delete detected:" << id();
        }
	}

    QString uniqueQObjectName() const
    {
        return T::staticMetaObject.className() + QString("_") + id();
    }
    QString id() const
    {
        return QString("%1").arg(m_id_num);
    }

private:
    /// @note Private constructor and friended to T to avoid ambiguities
    /// if this CRTP class is used as a base in several classes in a class hierarchy.
    UniqueIDMixin()
    {
        m_id_num = m_next_id_num;
        m_next_id_num++;
    }
    friend T;
};

/// Per-type ID counter.
template <typename T>
std::atomic_uintmax_t UniqueIDMixin<T>::m_next_id_num;

template <typename T>
std::mutex UniqueIDMixin<T>::m_deleted_ids_mutex;

template <typename T>
std::unordered_set<uintmax_t> UniqueIDMixin<T>::m_deleted_ids;

#endif /* SRC_UTILS_UNIQUEIDMIXIN_H_ */
