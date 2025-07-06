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
	setObjectName("NowPlayingView");
	m_brdelegate = new BoldRowDelegate(this);
    setItemDelegate(m_brdelegate);

    // Partially connect to the row-bolding delegate
	connect_or_die(m_brdelegate, &BoldRowDelegate::updateRequested, this, [this]()
	{
		this->viewport()->update();
	});

	// Hook up double-click handler.
	connect_or_die(this, &MDINowPlayingView::doubleClicked, this, &MDINowPlayingView::onDoubleClicked);

    // connect_or_die(this, &MDINowPlayingView::activated, this, &MDINowPlayingView::jump);

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
		// m_disconnector << connect_or_die();
		// ShuffleProxyModel* shuffle_proxy_model = dynamic_cast<ShuffleProxyModel*>(model);
		m_disconnector
			<< connect_or_die(model, &QAbstractItemModel::modelAboutToBeReset, m_brdelegate, &BoldRowDelegate::clearAll);

		// 	<< connect_or_die(model, &QAbstractItemModel::modelReset, shuffle_proxy_model, &ShuffleProxyModel::onNumRowsChanged)
		// 	<< connect_or_die(model, &QAbstractItemModel::rowsInserted, shuffle_proxy_model, &ShuffleProxyModel::onNumRowsChanged)
		// 	<< connect_or_die(model, &QAbstractItemModel::rowsRemoved, shuffle_proxy_model, &ShuffleProxyModel::onNumRowsChanged);

	}
}

void MDINowPlayingView::onDoubleClicked(const QModelIndex& index)
{
	// Should always be valid.
	qDebug() << "Double-clicked index:" << index;
	Q_ASSERT(index.isValid());

	// M_WARNING("TODO: Fix assumption");
	if (true) // we're the playlist connected to the player.
	{
		startPlaying(index);
	}
}

void MDINowPlayingView::onActivated(const QModelIndex& index)
{
    // M_WARNING("TODO: Fix assumption");
	if (true) // we're the playlist connected to the player.
	{
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
	// Q_EMIT nowPlayingIndexChanged(new_index, old_index);
}

