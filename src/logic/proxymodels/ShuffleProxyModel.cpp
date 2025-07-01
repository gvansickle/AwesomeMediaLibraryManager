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

// Qt
#include <QTimer>

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

bool ShuffleProxyModel::shuffleOn() const
{
	return m_shuffle;
}


void ShuffleProxyModel::loopAtEnd(bool loop_at_end)
{
	m_loop_at_end = loop_at_end;
}

bool ShuffleProxyModel::loopAtEndOn() const
{
	return m_loop_at_end;
}

#if 0
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
#endif

void ShuffleProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
	beginResetModel();

	m_disconnector.disconnect();
	QSortFilterProxyModel::setSourceModel(sourceModel);
	connectToModel(sourceModel);

	onNumRowsChanged();

	endResetModel();
}

QModelIndex ShuffleProxyModel::currentIndex() const
{
	return m_currentIndex;
}

void ShuffleProxyModel::onNumRowsChanged()
{
	// Resize and re-iota-ize the shuffle map.
	auto num_rows = sourceModel()->rowCount();

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
	if (was_empty && sourceModel()->rowCount() > 0)
	{
		// Went from empty Now Playing to non-empty.  Set a current item and selected item,
		// so that the play button works.
		// QTimer stuff here because this is segfaulting:
		// selectionModel()->select(model()->index(0, 0),
		//           QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        QTimer::singleShot(0, this, [this]()
		{
			m_current_shuffle_index = m_indices.at(0);
			Q_EMIT nowPlayingIndexChanged(sourceModel()->index(0,0), QModelIndex());
		});
	}
}

// slot
void ShuffleProxyModel::next()
{
	if (rowCount() == 0)
	{
		return;
	}

	bool stop_playing {false};

	m_current_shuffle_index++;
	if (m_current_shuffle_index >= m_indices.size())
	{
		m_current_shuffle_index = 0;
		if (!m_loop_at_end)
		{
			stop_playing = true;
		}
	}
    qDb() << "NEXT CURRENT SHUFFLE INDEX:" << m_current_shuffle_index;

	// Map from sequential m_current_shuffle_index [0, 1, ..., n) to possibly shuffled track index.
	auto new_row = m_indices.at(m_current_shuffle_index);
	auto new_index = index(new_row, 0);
	auto old_index = m_currentIndex;
	m_currentIndex = new_index;

	qDb() << "NEW INDEX:" << new_index << "old_index:" << old_index << M_ID_VAL(m_current_shuffle_index);

    // Emit the necessary signals.

	if (old_index.isValid())
	{
		Q_EMIT dataChanged(old_index, old_index, QList<int>(Qt::FontRole));
	}
	if (new_index.isValid())
	{
		Q_EMIT dataChanged(new_index, new_index, QList<int>(Qt::FontRole));
    }

    if (stop_playing)
    {
#warning "TODO This stops the playback, but we lose the bolded line, and the current_index's aren't right."
        new_index = QModelIndex();
    }

    Q_EMIT nowPlayingIndexChanged(new_index, old_index);
}

// slot
void ShuffleProxyModel::previous()
{
	if (rowCount() == 0)
	{
		return;
	}

	bool stop_playing {false};

	m_current_shuffle_index--;
	if (m_current_shuffle_index < 0)
	{
		m_current_shuffle_index = m_indices.size() - 1;
	}
	if (!m_loop_at_end)
	{
		stop_playing = true;
	}

	auto new_row = m_indices.at(m_current_shuffle_index);
	auto new_index = index(new_row, 0);
	auto old_index = m_currentIndex;
	m_currentIndex = new_index;

	// Emit the necessary signals.

	if (old_index.isValid())
	{
		Q_EMIT dataChanged(old_index, old_index, QList<int>(Qt::FontRole));
	}
	if (new_index.isValid())
	{
		Q_EMIT dataChanged(new_index, new_index, QList<int>(Qt::FontRole));
	}
	Q_EMIT nowPlayingIndexChanged(new_index, old_index);
}

void ShuffleProxyModel::jump(const QModelIndex& index)
{
	if (index.isValid())
	{
		// Keep the shuffle index synced.
		m_current_shuffle_index = m_indices[index.row()];

		// old_index might be QModelIndex() here.
		auto old_index = currentIndex();
		Q_EMIT nowPlayingIndexChanged(index, old_index);
	}
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
