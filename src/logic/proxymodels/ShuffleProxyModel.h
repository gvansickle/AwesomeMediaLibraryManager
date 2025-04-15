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

/**
 * @file
 */

// Std C++
#include <vector>

// Qt
#include <ConnectHelpers.h>
#include <QSortFilterProxyModel>
#include <QModelIndex>


/**
* A proxy model which shuffles the rows of the source model.
* The source model is not altered.
*/
class ShuffleProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
	explicit ShuffleProxyModel(QObject* parent = nullptr);
    ~ShuffleProxyModel() noexcept override = default;

	/**
	 * 
	 * @param shuffle_model_rows true == shuffle model rows, false == shuffle navigation order.
	 */
	void shuffleModelRows(bool shuffle_model_rows);

	/**
	 * Shuffles the proxy<->source model map.
	 * @param shuffle true = shuffle, false = unshuffle.
	 */
    void shuffle(bool shuffle = false);

	QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;

	void setSourceModel(QAbstractItemModel* sourceModel) override;

public Q_SLOTS:

    void onNumRowsChanged();

protected:
	/**
	 * Connect the necessary signals and slots between this proxy model
	 * and the (proxy) model it's attached to.  Pass nullptr to disconnect any existing connections.
	 * @param model The model to connect to.  Pass nullptr to disconnect.
	 */
	void connectToModel(QAbstractItemModel *model);

private:
	/**
	 * Sets whether shuffling is on or off.
	 */
	bool m_shuffle {false};

	bool m_shuffle_model_rows {false};

	int m_current_row {0};

	/**
	* The map of proxy indices <-> source model indices.
	*/
	std::vector<int> m_indices;

	Disconnector m_disconnector;
};

// For queued-connection support (i.e. thread-safe slots)
Q_DECLARE_METATYPE(ShuffleProxyModel)

#endif //SHUFFLEPROXYMODEL_H
