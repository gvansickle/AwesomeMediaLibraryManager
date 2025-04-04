/*
* Copyright 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef SHUFFLEPROXYMODEL_H
#define SHUFFLEPROXYMODEL_H

// Std C++
#include <vector>

// Qt
#include <QSortFilterProxyModel>
#include <QModelIndex>


/**
* A proxy model which shuffles the rows of the source model.
* The source model is not altered.
*/
class ShuffleProxyModel : public QSortFilterProxyModel
{
public:
	explicit ShuffleProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}
    ~ShuffleProxyModel() = default;

	/**
	 * Shuffles the proxy<->source model map.
	 * @param shuffle true = shuffle, false = unshuffle.
	 */
	void shuffle(bool shuffle);

	QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;

private:
	/**
	* The map of proxy indices <-> source model indices.
	*/
	std::vector<int> m_indices;
};



#endif //SHUFFLEPROXYMODEL_H
