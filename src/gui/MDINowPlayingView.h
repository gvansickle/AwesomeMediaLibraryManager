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

#ifndef AWESOMEMEDIALIBRARYMANAGER_MDINOWPLAYINGVIEW_H
#define AWESOMEMEDIALIBRARYMANAGER_MDINOWPLAYINGVIEW_H


#include "MDIPlaylistView.h"
#include "delegates/BoldRowDelegate.h"


class MDINowPlayingView : public MDIPlaylistView
{
    Q_OBJECT

Q_SIGNALS:
	/// @name Signals for player-connected messages.
	/// @{

	/// Start playing the current song.
	void play();

	/// @}

public:
    MDINowPlayingView(QWidget *parent);
    
    /**
     * static member function which opens an MDILibraryView on the given model.
     */
    static MDINowPlayingView* openModel(QAbstractItemModel* model, QWidget* parent = nullptr);
	
    QString getDisplayName() const override;

	void setModel(QAbstractItemModel* model) override;

public Q_SLOTS:

    void setCurrentIndexAndRow(const QModelIndex& new_index, const QModelIndex& old_index);

protected:

	void connectToModel(QAbstractItemModel* model);

	/**
	 * The Now Playing view isn't directly saveable, but serves as a volatile "scratch" playlist for
	 * the player.  So it doesn't make sense to indicate that it's modified, even when it is.
	 */
	bool isModified() const override { return false; }

protected Q_SLOTS:

	/// Invoked when the user double-clicks on an entry.
	/// According to Qt docs, index will always be valid:
	/// https://doc.qt.io/qt-6/qabstractitemview.html#doubleClicked:
	/// "The [doubleClicked] signal is only emitted when the index is valid."
	void onDoubleClicked(const QModelIndex& index);

	/**
	 * Slot called when the user activates (hits Enter) on an item.
	 * @param index
	 */
	void onActivated(const QModelIndex& index) override;

private:
	Q_DISABLE_COPY(MDINowPlayingView)

	/**
	 * Tell the player component to start playing the song at @a index.
	 * @param index
	 */
	void startPlaying(const QModelIndex& index);

    QPointer<BoldRowDelegate> m_brdelegate;

	Disconnector m_disconnector;

};



#endif //AWESOMEMEDIALIBRARYMANAGER_MDINOWPLAYINGVIEW_H
