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
#include <QMutex>
#include <QObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
class QSqlDatabase;
#include <QSqlRelationalTableModel>

// Ours.
#include <logic/DirScanResult.h>

/*
 *
 */
class CollectionDatabaseModel : public QObject
{
    Q_OBJECT

public:
    explicit CollectionDatabaseModel(QObject *parent);
     ~CollectionDatabaseModel() override;

    /**
     * Open or Create a SQLite database file at @p db_file.
	 *
	 * Should only be called once per database file.  Subsequently use OpenDatabaseConnection() to get connections.
	 *
     * @param db_file
     * @return
     */
	QSqlError InitDb(const QUrl& db_file, const QString& connection_name = QLatin1String(QSqlDatabase::defaultConnection));

    QSqlDatabase OpenDatabaseConnection(const QString& connection_name);

	static QSqlDatabase database(const QString& connection_name = QLatin1String(QSqlDatabase::defaultConnection));

	void LogDriverFeatures(QSqlDriver* driver) const;

	void LogConnectionInfo(const QSqlDatabase& db_connection) const;

    QSqlRelationalTableModel* make_reltable_model(QObject* parent = nullptr);
    QSqlRelationalTableModel* get_reltable_model() { return m_relational_table_model; }

    void RunQuery(const QString& query, QSqlDatabase& db_conn);

public Q_SLOT:
	QSqlError SLOT_addDirScanResult(DirScanResult dsr);

	QVariant addDirScanResult(QSqlQuery &q, const DirScanResult& dsr);

protected:

	static QMutex m_db_mutex;
	static QHash<QThread*, QHash<QString, QSqlDatabase>> m_db_instances;

    bool IfExistsAskForDelete(const QUrl& filename);

	QSqlDatabase CreateInitAndOpenDBConnection(const QUrl& db_file, const QString& connection_name = QLatin1String(QSqlDatabase::defaultConnection));

	QSqlError CreateSchema(QSqlDatabase &db);

    void InitializeModel();

private:

	QUrl m_db_file;

    QString m_connection_name = "the_connection_name";

    QSqlRelationalTableModel* m_relational_table_model {nullptr};
	QSqlQuery* m_prepped_insert_query;

};

#endif /* SRC_LOGIC_DBMODELS_COLLECTIONDATABASEMODEL_H_ */
