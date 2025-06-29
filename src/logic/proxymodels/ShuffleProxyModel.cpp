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
/// @file

#include "ShuffleProxyModel.h"

// Std C++
#include <numeric>
#include <algorithm>
#include <random>

// Ours
#include <ConnectHelpers.h>


ShuffleProxyModel::ShuffleProxyModel(QObject* parent): QSortFilterProxyModel(parent)
{

}

void ShuffleProxyModel::shuffle(bool shuffle)
{
	beginResetModel();

	m_shuffle = shuffle;
	m_indices.resize(sourceModel()->rowCount());
	std::ranges::iota(m_indices, 0);
	if (shuffle)
	{
		std::ranges::shuffle(m_indices, std::mt19937(std::random_device{}()));
	}

	endResetModel();
}
/// @todo Use or remove this
// void ShuffleProxyModel::shuffle(bool shuffle)
// {
// 	m_shuffle = shuffle;
// 	onNumRowsChanged();
// }

void ShuffleProxyModel::loopAtEnd(bool loop_at_end)
{
	m_loop_at_end = loop_at_end;
}

QModelIndex ShuffleProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
	if (!sourceIndex.isValid())
    {
        return {};
    }
	if (m_shuffle_model_rows)
	{
		return createIndex(m_indices[sourceIndex.row()], sourceIndex.column());
	}
	else
	{
		return sourceIndex;
	}
}

QModelIndex ShuffleProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
	if (!proxyIndex.isValid())
	{
		return {};
	}

	if (m_shuffle_model_rows)
	{
		return sourceModel()->index(m_indices[proxyIndex.row()], proxyIndex.column());
	}
	else
	{
		return proxyIndex;
	}
}

void ShuffleProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
	beginResetModel();

	m_disconnector.disconnect();
	QSortFilterProxyModel::setSourceModel(sourceModel);
	connectToModel(sourceModel);

	onNumRowsChanged();

	endResetModel();
}

void ShuffleProxyModel::onNumRowsChanged()
{
	// Resize the shuffle map.
	auto num_rows = sourceModel()->rowCount();
	m_indices.resize(num_rows);
	std::iota(m_indices.begin(), m_indices.end(), 0);
	if (m_shuffle)
	{
		std::ranges::shuffle(m_indices, std::mt19937(std::random_device{}()));
	}
}

void ShuffleProxyModel::setCurrentIndex(const QModelIndex& index)
{

}

#if 0
/// @todo From MDINowPlayingView.  Merge into above?
void MDINowPlayingView::onNumRowsChanged()
{
	// Resize and re-iota-ize the shuffle map.
	auto num_rows = model()->rowCount();

	// Maintain our shuffle index.
	if (num_rows == 0)
	{
		m_current_shuffle_index = -1;
	}
	bool was_empty = (m_current_shuffle_index == -1) ? true : false;
	ssize_t temp_shuffle_index = -1;
	if (m_current_shuffle_index >= 0 && m_current_shuffle_index<m_indices.size())
	{
		temp_shuffle_index = m_indices.at(m_current_shuffle_index);
	}

	m_indices.resize(num_rows);
	std::iota(m_indices.begin(), m_indices.end(), 0);

	if (m_shuffle)
	{
		std::ranges::shuffle(m_indices, std::mt19937(std::random_device{}()));
	}

	m_current_shuffle_index = temp_shuffle_index;
	if (was_empty && model()->rowCount() > 0)
	{
		// Went from empty Now Playing to non-empty.  Set a current item and selected item,
		// so that the play button works.
		// QTimer stuff here because this is segfaulting:
		// selectionModel()->select(model()->index(0, 0),
		//           QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
		QTimer::singleShot(0, [this]()
		{
			m_current_shuffle_index = m_indices.at(0);
			setCurrentIndexAndRow(model()->index(0, 0), QModelIndex());
		});
	}
}
#endif

// slot
void ShuffleProxyModel::next()
{
	QModelIndex current_index = currentIndex();
	if (!current_index.isValid())
	{
		/// @todo Not immediately clear how we recover from this situation.
		qDb() << "Model's current item is invalid.  Maybe no items in current playlist?";
		return;
	}

	bool stop_playing{false};

	m_current_shuffle_index++;
	if (m_current_shuffle_index >= m_indices.size())
	{
		m_current_shuffle_index = 0;
		if (!m_loop_at_end)
		{
			stop_playing = true;
		}
	}

	// Map from sequential m_current_shuffle_index [0, 1, ..., n) to possibly shuffled song index.
	auto next_row = m_indices.at(m_current_shuffle_index);

	auto next_index = current_index.sibling(next_row, 0);

	// Set the next index as the current item.
	/// @todo Not sure if the Q_EMITs here need to be non-queued or not.
	// Q_EMIT const_cast<QAbstractItemModel*>(model())->dataChanged(next_index, next_index, QList<int>(Qt::FontRole));
	// Q_EMIT const_cast<QAbstractItemModel*>(model())->
	// 	dataChanged(current_index, current_index, QList<int>(Qt::FontRole));
	Q_EMIT dataChanged(next_index, next_index, QList<int>(Qt::FontRole));
	Q_EMIT dataChanged(current_index, current_index, QList<int>(Qt::FontRole));

	if (stop_playing)
	{
		next_index = QModelIndex();
	}
	setCurrentIndexAndRow(next_index, current_index);
}

// slot
void ShuffleProxyModel::previous()
{
	QModelIndex current_index = currentIndex();
	if (!current_index.isValid())
	{
		/// @todo Not immediately clear how we recover form this situation.
		qDb() << "Model's current item is invalid.  Maybe no items in current playlist?";
		return;
	}

	m_current_shuffle_index--;
	if (m_current_shuffle_index < 0)
	{
		m_current_shuffle_index = m_indices.size() - 1;
	}
	auto prev_row = m_indices.at(m_current_shuffle_index);

	auto prev_index = current_index.sibling(prev_row, current_index.column());

	// Set the previous index as the current item.
	/// @todo Not sure if the Q_EMITs here need to be non-queued or not.
	// Q_EMIT const_cast<QAbstractItemModel*>(model())->dataChanged(prev_index, prev_index, QList<int>(Qt::FontRole));
	// Q_EMIT const_cast<QAbstractItemModel*>(model())->
	// 	dataChanged(current_index, current_index, QList<int>(Qt::FontRole));
	Q_EMIT dataChanged(prev_index, prev_index, QList<int>(Qt::FontRole));
	Q_EMIT dataChanged(current_index, current_index, QList<int>(Qt::FontRole));

	setCurrentIndexAndRow(prev_index, current_index);
}


void ShuffleProxyModel::connectToModel(QAbstractItemModel* model)
{
	if (model == nullptr)
	{
		// Disconnect from the model.
		m_disconnector.disconnect();
	}
	else
	{
		m_disconnector
			<< connect_or_die(model, &QAbstractItemModel::modelReset, this, &ShuffleProxyModel::onNumRowsChanged)
			<< connect_or_die(model, &QAbstractItemModel::rowsInserted, this, &ShuffleProxyModel::onNumRowsChanged)
			<< connect_or_die(model, &QAbstractItemModel::rowsRemoved, this, &ShuffleProxyModel::onNumRowsChanged);
	}
}
