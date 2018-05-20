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

#ifndef SRC_CONCURRENCY_THREADSAFEMAP_H_
#define SRC_CONCURRENCY_THREADSAFEMAP_H_

#include <QMap>
#include <QMutex>
#include <QMutexLocker>


/*
 *
 */
template <typename Key, typename Value>
class ThreadsafeMap
{
    using T = Value;
    using ActiveActivitiesMap = QMap<Key, T>;

public:
    ThreadsafeMap() : m_map_mutex() {}

    const T value(const Key &key, const T &defaultValue = T()) const
    {
        QMutexLocker locker(&m_map_mutex);
        return m_kjob_to_widget_map.value(key, defaultValue);
    }
    void insert(const Key &key, const T &value)
    {
        QMutexLocker locker(&m_map_mutex);
        m_kjob_to_widget_map.insert(key, value);
    }
    int remove(const Key &key)
    {
        QMutexLocker locker(&m_map_mutex);
        return m_kjob_to_widget_map.remove(key);
    }

    const T operator[](const Key &key) const
    {
        return value(key);
    }

    QList<Key> keys() const
    {
        QList<Key> retval;

        QMutexLocker locker(&m_map_mutex);

        retval = m_kjob_to_widget_map.keys();

        return retval;
    }

    int size() const
    {
        QMutexLocker locker(&m_map_mutex);
        return m_kjob_to_widget_map.size();
    }

#if 0
    template<typename Lambda>
    void for_each_key_value_pair(Lambda the_lambda)
    {
        QMutexLocker locker(&m_map_mutex);
        for(ActiveActivitiesMap::iterator i = m_kjob_to_widget_map.begin(); i != m_kjob_to_widget_map.end(); ++i)
        {
            the_lambda(i);
        }
    }
#endif
    /// Public types
    using iterator = typename ActiveActivitiesMap::iterator;

private:

    /// Mutex for protecting the map.
    mutable QMutex m_map_mutex;

    ActiveActivitiesMap m_kjob_to_widget_map;
};

#endif /* SRC_CONCURRENCY_THREADSAFEMAP_H_ */
