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


#ifndef BASESORTFILTERPROXYMODEL_H
#define BASESORTFILTERPROXYMODEL_H

/// @file

#include <QSortFilterProxyModel>


/**
 * Base class for any proxies which want to derive from QSortFilterProxyModel.
 * Adds some infrastructure which will likely be needed in any derived class.
 */
class BaseSortFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	BaseSortFilterProxyModel(QObject* parent);

protected Q_SLOTS:

	virtual void onModelAboutToBeReset();
	virtual void onModelReset();

private:
	Q_DISABLE_COPY_MOVE(BaseSortFilterProxyModel);
};


#endif //BASESORTFILTERPROXYMODEL_H
