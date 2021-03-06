/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

class MDINowPlayingView : public MDIPlaylistView
{
    Q_OBJECT

public:
    MDINowPlayingView(QWidget *parent);
    
    /**
     * static member function which opens an MDILibraryView on the given model.
     */
    static MDINowPlayingView* openModel(QAbstractItemModel* model, QWidget* parent = nullptr);
	
    QString getDisplayName() const override;

protected:

	/**
	 * The Now Playing view isn't directly saveable, but serves as a volatile "scratch" playlist for
	 * the player.  So it doesn't make sense to indicate that it's modified, even when it is.
	 */
	bool isModified() const override { return false; }


private:
	Q_DISABLE_COPY(MDINowPlayingView)
};


#endif //AWESOMEMEDIALIBRARYMANAGER_MDINOWPLAYINGVIEW_H
