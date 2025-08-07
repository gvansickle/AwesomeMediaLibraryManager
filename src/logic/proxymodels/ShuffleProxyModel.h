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
#include <QPointer>
#include <QModelIndex>
class QSignalSpy;
class QItemSelectionModel;

// Ours.
#include "BaseSortFilterProxyModel.h"
#include <utils/ConnectHelpers.h>
#include <utils/DebugSequence.h>

/**
* A proxy model which shuffles the rows of the source model.
* The source model is not altered.
*/
class ShuffleProxyModel : public BaseSortFilterProxyModel
{
    Q_OBJECT

	using BASE_CLASS = BaseSortFilterProxyModel;

public:
	explicit ShuffleProxyModel(QObject* parent = nullptr);
    ~ShuffleProxyModel() noexcept override = default;

	/**
	 * Shuffles the proxy<->source model map.
	 * @param shuffle true = shuffle, false = unshuffle.
	 */
    void setShuffle(bool shuffle = false);
	bool shuffle() const;

	void setLoopAtEnd(bool loop_at_end);
	bool loopAtEnd() const;

	void setSourceModel(QAbstractItemModel* sourceModel) override;

	QModelIndex currentIndex() const;

	DebugSequence m_ds;
	QPointer<QSignalSpy> m_spy;

Q_SIGNALS:
	void nowPlayingIndexChanged(const QModelIndex& current, const QModelIndex& previous, bool stop_playing);

public Q_SLOTS:

	void onNumRowsChanged();

	void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles);

	/**
	 * Start next song.
	 * Makes the next item in the model the current item in the view.
	 */
	void next();

	/**
	 * Start previous song.
	 */
	void previous();

	/**
	 * Go to @a index and start playing.
	 */
	void jump(const QModelIndex& index);

protected:
	/**
	 * Connect the necessary signals and slots between this proxy model
	 * and the (proxy) model it's attached to.  Pass nullptr to disconnect any existing connections.
	 * @param model The model to connect to.  Pass nullptr to disconnect.
	 */
	void connectToModel(QAbstractItemModel *model);

protected Q_SLOTS:

	void onModelAboutToBeReset() override;
	void resetInternalData() override;
	void onModelReset() override;

private:
	QPointer<QItemSelectionModel> m_sel_model;
	/**
	 * The PMI pointing to the bolded row.
	 */
	QPersistentModelIndex m_currentIndex {QModelIndex()};

	/**
	 * Sets whether shuffling is on or off.
	 */
	bool m_shuffle {false};

	/**
	 * Current loop mode
	 */
	bool m_loop_at_end {true};

	/// The index into m_indices which should be played.
	std::int64_t m_current_shuffle_index {-1};

	/**
	 * The map of proxy indices <-> source model indices.
	 * If not shuffled, this is just a 1-to-1 mapping.
	 * If shuffled, this is a randomized mapping.
	 */
	std::vector<int> m_indices;

	Disconnector m_disconnector;
};

// For queued-connection support (i.e. thread-safe slots)
Q_DECLARE_METATYPE(ShuffleProxyModel)

#endif //SHUFFLEPROXYMODEL_H
