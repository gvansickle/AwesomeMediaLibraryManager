/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef SRC_LOGIC_DBMODELS_COLLECTIONDATABASEMODEL_H_
#define SRC_LOGIC_DBMODELS_COLLECTIONDATABASEMODEL_H_

#include <config.h>

// Qt5
#include <QObject>
#include <QUrl>
class QSqlDatabase;
class QSqlRelationalTableModel;

/*
 *
 */
class CollectionDatabaseModel : public QObject
{
public:
    CollectionDatabaseModel(QObject *parent);
	virtual ~CollectionDatabaseModel();

    bool open_db_connection(QUrl db_file);

    void create_db_tables(QSqlDatabase *db = nullptr);

    QSqlRelationalTableModel* get_rel_table(QObject* parent = nullptr);
};

#endif /* SRC_LOGIC_DBMODELS_COLLECTIONDATABASEMODEL_H_ */
