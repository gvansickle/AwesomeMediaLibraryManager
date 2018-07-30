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

	QSqlDatabase OpenDatabaseConnection(const QString& connection_name = QLatin1String(QSqlDatabase::defaultConnection),
										bool write = false, bool create = false);

	void LogDriverFeatures(QSqlDriver* driver) const;

	void LogConnectionInfo(const QSqlDatabase& db_connection) const;

	QSqlRelationalTableModel* make_reltable_model(QObject* parent, QSqlDatabase db_conn);

	void LogModelInfo(QSqlRelationalTableModel* model) const;

	/**
	 * Helper function to inefficiently run a simple query (e.g. PRAGMAs) and return
	 * a single result value.
	 */
	QVariant RunQuery(const QString& query, QSqlDatabase& db_conn);

public Q_SLOT:
	QSqlError SLOT_addDirScanResult(DirScanResult dsr);

	QVariant addDirScanResult(QSqlQuery &q, const DirScanResult& dsr);

protected:

	/// @name A mechanism for keeping track of the connections we have opened per thread.
	/// We need this so we don't delete an existing connection accidentally.
	/// @{
	QMutex m_db_mutex;
	QHash<QThread*, QHash<QString, QSqlDatabase>> m_db_instances;
	void register_root_database_connection(const QSqlDatabase& connection, const QString& connection_name);
	QSqlDatabase database(const QString& connection_name = QLatin1String(QSqlDatabase::defaultConnection), bool read_only = true);
	/// @}

    bool IfExistsAskForDelete(const QUrl& filename);

	QSqlDatabase AddInitAndOpenDBConnection(const QUrl& db_file, const QString& connection_name = QLatin1String(QSqlDatabase::defaultConnection),
											   bool write = false, bool create = false);

	QSqlError SqlPRAGMA(QSqlDatabase& db_conn, const QString& str);

	QSqlError ApplyPragmas(QSqlDatabase& db_conn);

	QSqlError CreateSchema(QSqlDatabase &db);

    void InitializeModel();

private:

	QUrl m_db_file;

    QString m_connection_name = "the_connection_name";

    QSqlRelationalTableModel* m_relational_table_model {nullptr};
	QSqlQuery* m_prepped_insert_query;

};

#endif /* SRC_LOGIC_DBMODELS_COLLECTIONDATABASEMODEL_H_ */
