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
 * @file ModelIterators.h
 */
#ifndef SRC_LOGIC_MODELS_MODELITERATORS_H_
#define SRC_LOGIC_MODELS_MODELITERATORS_H_

// Std C++
#include <iterator>

#include "AbstractTreeModelItem.h"

namespace AMLM
{

/*
 *
 */
class ModelIterators
{
};

//////////////////
/// DO NOT USE, THIS DOES NOT WORK YET.
//////////////////////////////////////
class bfs_iterator : public std::iterator<
														// Category: bfs will be a LegacyInputIterator (can only be incremented, may invalidate all copies of prev value).
														/// @link https://en.cppreference.com/w/cpp/named_req/InputIterator
														std::input_iterator_tag,
														//ItemType,
														AbstractTreeModelItem,
														// Distance is meaningless
														void,
														// Pointer and Reference need to be smart.
														std::shared_ptr<AbstractTreeModelItem>,
														AbstractTreeModelItem&
														>
{
public:
	using iterator_concept = std::input_iterator_tag;

	bfs_iterator();
	explicit bfs_iterator(std::shared_ptr<AbstractTreeModelItem> root_node);

	bfs_iterator& operator++();

	bfs_iterator operator++(int);

	bool operator==(const bfs_iterator& other) const;

	bool operator!=(const bfs_iterator& other) const;

	reference operator*() const;

private:
	std::shared_ptr<AbstractTreeModelItem> m_root_node;
	std::shared_ptr<AbstractTreeModelItem> m_current_node;
	std::shared_ptr<bfs_iterator> m_child_bfs_it;
//	AbstractTreeModelItem::CICTIteratorType m_child_list_it;
	bool m_is_at_end {false};
};

#if 0
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///
/// AbstractTreeModelItem::bfs_iterator
///

AbstractTreeModelItem::bfs_iterator::bfs_iterator() { }

AbstractTreeModelItem::bfs_iterator::bfs_iterator(std::shared_ptr<AbstractTreeModelItem> root_node)
	: m_root_node(root_node), m_current_node(root_node),
	  m_child_list_it(root_node->m_child_items.begin())
{
	m_child_bfs_it = std::make_shared<bfs_iterator>(root_node->begin_bfs());
}

AbstractTreeModelItem::bfs_iterator AbstractTreeModelItem::bfs_iterator::operator++(int)
{
	auto retval = *this;
	++(*this);
	return retval;
}

bool AbstractTreeModelItem::bfs_iterator::operator==(const AbstractTreeModelItem::bfs_iterator& other) const
{
	return (m_current_node == other.m_current_node);
}

bool AbstractTreeModelItem::bfs_iterator::operator!=(const AbstractTreeModelItem::bfs_iterator& other) const
{
	return !(*this == other);
}

AbstractTreeModelItem::bfs_iterator::reference AbstractTreeModelItem::bfs_iterator::operator*() const
{
	return *m_current_node;
}

AbstractTreeModelItem::bfs_iterator& AbstractTreeModelItem::bfs_iterator::operator++()
{
	// Steps of a DFS at each node:
	// Perform pre-order operation.
	// For each i from 1 to the number of children do:
	//     Visit i-th, if present.
	//     Perform in-order operation.
	// Perform post-order operation.

	// Are we already at the end?
	if(m_current_node == nullptr || m_is_at_end)
	{
		// end() iterator doesn't increment.
		return *this;
	}

	/// Preorder return here?

	// Lock our weak parent ptr.  We should have a parent unless we're the true root.
//	auto parent = m_current_node->parent_item().lock();

//	if(parent == nullptr || parent == m_root_node) /// Handle no-parent differently?
//	{
//		// We hit the node we started at on the way up, next state is end().

//		/// Post-order return here?

//		m_current_node = nullptr;
//		m_is_at_end = true;
//		return *this;
//	}

	// Else we should have a valid m_current_node and it's parent, which should be us?
	// So we visit all children of this node in-order.
//	m_current_node = *m_child_list_it;
	if(m_child_list_it == m_current_node->m_child_items.end())
	{
		// Reached the end of the current node's child list.
		// Now we go back to the parent of m_current_node.
		auto parent = m_current_node->parent_item().lock();
		if(parent != m_root_node)
		{
			m_current_node = m_current_node->parent_item().lock();
		}
	}
	else
	{
		// Still iterating over the child items.
		++m_child_list_it;
		// Recurse on this node as a new root node.
		(*m_child_bfs_it)++;
	}

	return *this;
}

#endif

} /* namespace AMLM */

#endif /* SRC_LOGIC_MODELS_MODELITERATORS_H_ */
