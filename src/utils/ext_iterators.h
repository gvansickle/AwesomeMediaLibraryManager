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
 * @file ext_iterators.h
 * Adapted from SO:
 * @link https://stackoverflow.com/a/35262398
 */
#ifndef SRC_UTILS_EXT_ITERATORS_H_
#define SRC_UTILS_EXT_ITERATORS_H_

// Std C++
#include <map>


template <typename Key, typename Value>
using Map = std::map<Key, Value>;

template <typename Key, typename Value>
using map_iterator = typename Map<Key, Value>::iterator;

/**
 * Adapter for iterating only over the keys of a std::map<>.
 */
template <typename Key, typename Value>
class map_key_iterator : public map_iterator<Key, Value>
{
public:

    map_key_iterator ( ) : map_iterator<Key, Value> ( ) { };
    map_key_iterator ( map_iterator<Key, Value> it ) : map_iterator<Key, Value> ( it ) { };

    Key *operator -> ( ) { return ( Key * const ) &( map_iterator<Key, Value>::operator -> ( )->first ); }
    Key operator * ( ) { return map_iterator<Key, Value>::operator * ( ).first; }
};

/**
 * Adapter for iterating only over the values of a std::map<>.
 */
template </*class MapType,*/ typename Key /*= typename MapType::key_type*/, typename Value /*= typename MapType::mapped_type*/>
class map_value_iterator : public map_iterator<Key, Value>
{
public:
    map_value_iterator ( ) : map_iterator<Key, Value> ( ) { };
    map_value_iterator ( map_iterator<Key, Value> it ) : map_iterator<Key, Value> ( it ) { };

    Value *operator -> ( ) { return ( Value * const ) &( map_iterator<Key, Value>::operator -> ( )->second ); }
    Value operator * ( ) { return map_iterator<Key, Value>::operator * ( ).second; }
};

/**
 * Map Transform adapter.  This one's mine.
 */
//template <typename Key, typename Value, typename TransformLambda>
//class map_transform_iterator : public map_iterator<Key, Value>
//{
//public:
//
//    map_value_iterator ( ) : map_iterator<Key, Value> ( ) { };
//    map_value_iterator ( map_iterator<Key, Value> it_ ) : map_iterator<Key, Value> ( it_ ) { };
//
//    Value *operator -> ( ) { return ( Value * const ) &( map_iterator<Key, Value>::operator -> ( )->second ); }
//    Value operator * ( ) { return map_iterator<Key, Value>::operator * ( ).second; }
//};

#endif /* SRC_UTILS_EXT_ITERATORS_H_ */
