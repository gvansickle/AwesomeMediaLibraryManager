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
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
class QSqlDatabase;
#include <QSqlRelationalTableModel>

/*
 *
 */
class CollectionDatabaseModel : public QObject
{
    Q_OBJECT

public:
    explicit CollectionDatabaseModel(QObject *parent);
     ~CollectionDatabaseModel() override;

    QSqlError InitDb(QUrl db_file);
    QSqlDatabase OpenDatabaseConnection(const QString& connection_name);

    QSqlRelationalTableModel* make_reltable_model(QObject* parent = nullptr);
    QSqlRelationalTableModel* get_reltable_model() { return m_relational_table_model; }

    QSqlError addDirScanResult(const QUrl& media_url, int release = 0);

    QVariant addMediaUrl(QSqlQuery &q, const QUrl& url);

    void RunQuery(const QString& query, QSqlDatabase& db_conn);

protected:

    bool IfExistsAskForDelete(const QUrl& filename);

    QSqlError CreatePrimaryTables(QSqlDatabase &db);
    QSqlError CreateRelationalTables(QSqlDatabase &db);

    void InitializeModel();

    QString m_connection_name = "the_connection_name";

    QSqlRelationalTableModel* m_relational_table_model {nullptr};

};

#endif /* SRC_LOGIC_DBMODELS_COLLECTIONDATABASEMODEL_H_ */
