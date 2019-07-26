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
class AbstractTreeModelItem::bfs_iterator : public std::iterator<
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
	CICTIteratorType m_child_list_it;
	bool m_is_at_end {false};
};

} /* namespace AMLM */

#endif /* SRC_LOGIC_MODELS_MODELITERATORS_H_ */
