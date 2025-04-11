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

	void nowPlayingIndexChanged(const QModelIndex& current, const QModelIndex& previous);

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

	void onNumRowsChanged();

	/// @name Slots for player-connected messages.
	/// @{

	/**
	 * Start next song.
	 * Makes the next item in the model the current item in the view.
	 */
	void next();

	/// Start previous song.
	void previous();

	/**
	 * Shuffles the unshuffled<->shuffled item index map.
	 * @param shuffle true = shuffle, false = unshuffle.
	 */
	void shuffle(bool shuffle = false);

	void jump(const QModelIndex& index);

	/// @}

protected:

	void connectToModel(QAbstractItemModel* model);

	/**
	 * The Now Playing view isn't directly saveable, but serves as a volatile "scratch" playlist for
	 * the player.  So it doesn't make sense to indicate that it's modified, even when it is.
	 */
	bool isModified() const override { return false; }

protected Q_SLOTS:

	/// Invoked when user double-clicks on an entry.
	/// According to Qt5 docs, index will always be valid:
	/// http://doc.qt.io/qt-5/qabstractitemview.html#doubleClicked:
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

	void setCurrentIndexAndRow(const QModelIndex& new_index, const QModelIndex& old_index);

	/**
	* The map of proxy indices <-> source model indices.
	*/
	std::vector<int> m_indices;

	/// The index into m_indices which should be played.
	int m_current_shuffle_index {-1};

	/// Current shuffle mode.
	bool m_shuffle {false};

	/// Current loop mode
	bool m_loop_at_end {false};

    BoldRowDelegate* m_brdelegate;

	Disconnector m_disconnector;

};




#endif //AWESOMEMEDIALIBRARYMANAGER_MDINOWPLAYINGVIEW_H
