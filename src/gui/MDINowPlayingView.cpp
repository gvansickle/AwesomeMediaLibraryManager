/*
 * Copyright 2017, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "MDINowPlayingView.h"

// Stc C++
#include <random>

// Ours.
#include <utils/DebugHelpers.h>
#include <proxymodels/ShuffleProxyModel.h>
#include <delegates/BoldRowDelegate.h>




MDINowPlayingView::MDINowPlayingView(QWidget *parent) : MDIPlaylistView(parent)
{
	m_brdelegate = new BoldRowDelegate(this);
    setItemDelegate(m_brdelegate);

    connect(model(), &QAbstractItemModel::modelAboutToBeReset, m_brdelegate, &BoldRowDelegate::clearAll);
	connect_or_die(m_brdelegate, &BoldRowDelegate::updateRequested, this, [this]()
	{
		this->viewport()->update();
	});

	// Hook up double-click handler.
	connect(this, &MDINowPlayingView::doubleClicked, this, &MDINowPlayingView::onDoubleClicked);

	connect_or_die(this, &MDINowPlayingView::activated, this, &MDINowPlayingView::jump);

	// Do not delete this window on close, just hide it.
//This doesn't seem to work as expected, Collection Widget still segfaults if Now Playing is closed then double-clicked.
//	setAttribute(Qt::WA_DeleteOnClose, false);
}

// static
MDINowPlayingView* MDINowPlayingView::openModel(QAbstractItemModel* model, QWidget* parent)
{
	auto view = new MDINowPlayingView(parent);
	view->setModel(model);
	return view;
}


QString MDINowPlayingView::getDisplayName() const
{
	return tr("Now Playing");
}

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
	if (m_current_shuffle_index >= 0 && m_current_shuffle_index < m_indices.size())
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
		QTimer::singleShot(0, [this](){
			m_current_shuffle_index = m_indices.at(0);
			setCurrentIndexAndRow(model()->index(0, 0), QModelIndex());
		});
	}
}

void MDINowPlayingView::next()
{
	QModelIndex current_index = currentIndex();
	if (!current_index.isValid())
	{
		/// @todo Not immediately clear how we recover from this situation.
		qDb() << "Model's current item is invalid.  Maybe no items in current playlist?";
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

	// Map from sequential m_current_shuffle_index [0, 1, ..., n) to possibly shuffled song index.
	auto next_row = m_indices.at(m_current_shuffle_index);

	auto next_index = current_index.sibling(next_row, 0);

	// Set the next index as the current item.
	/// @todo Not sure if the Q_EMITs here need to be non-queued or not.
	Q_EMIT const_cast<QAbstractItemModel*>(model())->dataChanged(next_index, next_index, QList<int>(Qt::FontRole));
	Q_EMIT const_cast<QAbstractItemModel*>(model())->dataChanged(current_index, current_index, QList<int>(Qt::FontRole));
	if (stop_playing)
	{
		next_index = QModelIndex();
	}
	setCurrentIndexAndRow(next_index, current_index);
}

void MDINowPlayingView::previous()
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
    Q_EMIT const_cast<QAbstractItemModel*>(model())->dataChanged(prev_index, prev_index, QList<int>(Qt::FontRole));
    Q_EMIT const_cast<QAbstractItemModel*>(model())->dataChanged(current_index, current_index, QList<int>(Qt::FontRole));
	setCurrentIndexAndRow(prev_index, current_index);
}

void MDINowPlayingView::shuffle(bool shuffle)
{
	m_shuffle = shuffle;
	onNumRowsChanged();
}

void MDINowPlayingView::loopAtEnd(bool loop_at_end)
{
	m_loop_at_end = loop_at_end;
}

void MDINowPlayingView::jump(const QModelIndex& index)
{
	if (index.isValid())
	{
		// old_index might be QModelIndex() here.
		auto old_index = currentIndex();
		setCurrentIndexAndRow(index, old_index);
	}
}

void MDINowPlayingView::setModel(QAbstractItemModel* model)
{
	// Disconnect from the old model.
	m_disconnector.disconnect();

	// Let the base class set up what it needs to.
	MDIPlaylistView::setModel(model);

	// Set up signals/slots we need.
	connectToModel(model);
}

void MDINowPlayingView::connectToModel(QAbstractItemModel* model)
{
	if (model == nullptr)
	{
		// Disconnect from the model.
		m_disconnector.disconnect();
	}
	else
	{
		m_disconnector
			<< connect_or_die(model, &QAbstractItemModel::modelReset, this, &MDINowPlayingView::onNumRowsChanged)
			<< connect_or_die(model, &QAbstractItemModel::rowsInserted, this, &MDINowPlayingView::onNumRowsChanged)
			<< connect_or_die(model, &QAbstractItemModel::rowsRemoved, this, &MDINowPlayingView::onNumRowsChanged);
	}
}

void MDINowPlayingView::onDoubleClicked(const QModelIndex& index)
{
	// Should always be valid.
	qDebug() << "Double-clicked index:" << index;
	Q_ASSERT(index.isValid());

	M_WARNING("TODO: Fix assumption");
	if (true) // we're the playlist connected to the player.
	{
		// Keep the shuffle index synced.
		m_current_shuffle_index = m_indices[index.row()];

		startPlaying(index);
	}
}

void MDINowPlayingView::onActivated(const QModelIndex& index)
{
	M_WARNING("TODO: Fix assumption");
	if (true) // we're the playlist connected to the player.
	{
		// Keep the shuffle index synced.
		m_current_shuffle_index = m_indices[index.row()];

		startPlaying(index);
	}
}

void MDINowPlayingView::startPlaying(const QModelIndex& index)
{
	// Tell the player to start playing the song at index.

	Q_ASSERT(index.isValid());

    Q_EMIT const_cast<QAbstractItemModel*>(model())->dataChanged(index, index, QList<int>(Qt::FontRole));
    m_brdelegate->setRow(index.row());
	setCurrentIndex(index);

	// If the player isn't already playing, the index change above won't start it.  Send a signal to it to
	// make sure it starts.
	Q_EMIT play();
}

void MDINowPlayingView::setCurrentIndexAndRow(const QModelIndex& new_index, const QModelIndex& old_index)
{
	m_brdelegate->setRow(new_index.row());
	setCurrentIndex(new_index);
	Q_EMIT nowPlayingIndexChanged(new_index, old_index);
}

