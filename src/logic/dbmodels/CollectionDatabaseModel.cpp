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

#include "CollectionDatabaseModel.h"

/// Qt5
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QSqlRelationalTableModel>
#include <QSqlRecord>
#include <QSqlDriver>
#include <QSqlIndex>
#include <QFile>
#include <QUrl>
#include <QUrlQuery>
#include <QString>

/// Ours
#include "SQLHelpers.h"
#include <utils/DebugHelpers.h>
#include <AMLMApp.h>
#include <gui/MainWindow.h>


CollectionDatabaseModel::CollectionDatabaseModel(QObject* parent) : QObject(parent)
{

}

CollectionDatabaseModel::~CollectionDatabaseModel()
{

}

QSqlError CollectionDatabaseModel::InitDb(const QUrl& db_file, const QString& connection_name)
{
	// Ask to delete file if it exists.
	/// @todo Temp, remove this after debugging.
	if(!IfExistsAskForDelete(db_file))
	{
		// File exists and user doesn't want to delete or delete failed.
		return QSqlError("driver text", "File exists", QSqlError::ConnectionError);
	}

	// Store the filename and connection name.
	m_db_file = db_file;
	m_connection_name = connection_name;

//	auto db = AddInitAndOpenDBConnection(db_file.toLocalFile(), connection_name, true, true);
	auto db = database(connection_name, false);

	Q_ASSERT(db.isValid());

//	register_root_database_connection(db, connection_name);

	auto err = CreateSchema(db);

	return err;
}

QSqlDatabase CollectionDatabaseModel::OpenDatabaseConnection(const QString &connection_name, bool write, bool create)
{
	QSqlDatabase db_conn = /*QSqlDatabase::*/database(connection_name, !write);
//	QSqlDatabase db_conn = AddInitAndOpenDBConnection(m_db_file, connection_name, write, create);
	Q_ASSERT(db_conn.isValid());

	RunQuery("PRAGMA foreign_keys;", db_conn);

//	register_root_database_connection(db_conn, connection_name);

	return db_conn;
}

void CollectionDatabaseModel::register_root_database_connection(const QSqlDatabase& connection, const QString& connection_name)
{
	QMutexLocker locker(&m_db_mutex);
	QThread *thread = QThread::currentThread();

	qIno() << "REGSITERING ROOT DB CONNECTION:" << thread << connection_name;
	m_db_instances[thread][connection_name] = connection;
}

QSqlDatabase CollectionDatabaseModel::database(const QString& connection_name, bool read_only)
{
	QMutexLocker locker(&m_db_mutex);
	QThread *thread = QThread::currentThread();

	QString rwdecorated_connection_name = QString("%1_%2")
			.arg(connection_name)
			.arg(read_only?"r":"rwc");

	QString new_connection_name = QString("%1_%2")
			.arg(rwdecorated_connection_name)
			.arg(reinterpret_cast<std::uintptr_t>(thread));

	// If we have a connection with the given name for this thread, return it.
	auto it_thread = m_db_instances.find(thread);
	if (it_thread != m_db_instances.end())
	{
		auto it_conn = it_thread.value().find(new_connection_name);
		if (it_conn != it_thread.value().end())
		{
			qDb() << "ALREADY HAVE DATABASE CONNECTION FOR THREAD/CONNECTION:" << thread->objectName() << rwdecorated_connection_name;
			return it_conn.value();
		}
	}

	// Otherwise, create a new connection for this thread by cloning from one existing in
	// a different thread.
//	qDb() << "CLONING DATABASE CONNECTION FOR THREAD/CONNECTION:" << thread->objectName() << rwdecorated_connection_name;
//	QSqlDatabase connection = QSqlDatabase::cloneDatabase(
//				QSqlDatabase::database(connection_name),
//				new_connection_name);

//	// open the database connection
//	if (!connection.open())
//	{
//		throw std::runtime_error("Unable to open the new database connection.");
//	}

//	// Re-Apply the pragmas.
//	ApplyPragmas(connection);

	// We need to do a full addDatabase().
	qDb() << "ADDING NEW DATABASE CONNECTION FOR THREAD/CONNECTION:" << thread->objectName() << rwdecorated_connection_name;
	QSqlDatabase connection = AddInitAndOpenDBConnection(m_db_file, new_connection_name, !read_only);

	Q_ASSERT(connection.isValid());

	// Log some debug info.
	LogConnectionInfo(connection);

	qIno() << "REGSITERING NEW DB CONNECTION:" << thread << new_connection_name;
	m_db_instances[thread][new_connection_name] = connection;
	return connection;
}

void CollectionDatabaseModel::LogDriverFeatures(QSqlDriver* driver) const
{
#define M_NAME_AND_VAL(name) #name ":" << driver->hasFeature( QSqlDriver::name )

	qIno() << "DB Driver Features:";
	qIno() << M_NAME_AND_VAL(Transactions);
	qIno() << M_NAME_AND_VAL(QuerySize);
	qIno() << M_NAME_AND_VAL(BatchOperations);
	qIno() << M_NAME_AND_VAL(SimpleLocking);
	qIno() << M_NAME_AND_VAL(EventNotifications);
	qIno() << "DB Notification Subscriptions:";
	qIno() << driver->subscribedToNotifications();

#undef M_NAME_AND_VAL
}

void CollectionDatabaseModel::LogConnectionInfo(const QSqlDatabase& db_connection) const
{
	qIno() << "DB Connection info:";
	qIno() << "  Connection Name:" << db_connection.connectionName();
	qIno() << "  Connection's Database Name:" << db_connection.databaseName();
	qIno() << "  Connection's Driver Name:" << db_connection.driverName();
	qIno() << "  Connection Options string:" << db_connection.connectOptions();
	qIno() << "DB Connection summary:";
	qIno() << "  Connection Names:" << QSqlDatabase::connectionNames();
}

QSqlError CollectionDatabaseModel::CreateSchema(QSqlDatabase &db)
{
//    auto db_conn = QSqlDatabase::database("experimental_db_connection");
    auto& db_conn = db;

    QStringList tables;

	tables.append(CREATE_TABLE "DirScanResults ("
				  "dirscanid " IPK ","
				  "media_url" TEXT NOT_NULL ","
				  "sidecar_cuesheet_url" TEXT ","
                  /// @todo experimental
				  "dirscanrelease INTEGER,"
				  "FOREIGN KEY(dirscanrelease) REFERENCES Release(releaseid)"
				  ");");
    tables.append("CREATE TABLE Artist ("
                  "artistid INTEGER PRIMARY KEY,"
                  "name TEXT"
				  ");");
    tables.append("CREATE TABLE Release ("
                  "releaseid INTEGER PRIMARY KEY,"
                  "releasename TEXT,"
                  "releaseartist INTEGER REFERENCES Artist"
				  ");");
	tables.append("CREATE INDEX dirscanindex ON Release(releaseid);"
				  );

    // Dummy data.
	tables.append("INSERT INTO Artist values(0, '<all>');");
	tables.append("INSERT INTO Artist values(1, 'Tom Petty');");
	tables.append("INSERT INTO Release values(0, '<all>', 0);");
	tables.append("INSERT INTO Release values(1, 'Free Fallin', 1);");
	tables.append("INSERT INTO DirScanResults values(0, 'http://', NULL, 1);");

    // Create tables.
    for(int i = 0; i < tables.count(); ++i)
    {
        QSqlQuery query(db_conn);
        if (!query.exec(tables[i]))
        {
            qCro() << query.lastError();
            qCro() << query.executedQuery();
            return query.lastError();
        }
    }

    return QSqlError();
}

QSqlRelationalTableModel *CollectionDatabaseModel::make_reltable_model(QObject *parent, QSqlDatabase db_conn)
{
	qDbo() << "OPENCONNECTION";

	QSqlRelationalTableModel* rel_table_model = new QSqlRelationalTableModel(parent, db_conn);
    Q_CHECK_PTR(rel_table_model);

    rel_table_model->setTable("DirScanResults");
	rel_table_model->setEditStrategy(QSqlTableModel::OnRowChange);
    // Left join to show rows with NULL foreign keys.
    rel_table_model->setJoinMode(QSqlRelationalTableModel::LeftJoin);

    rel_table_model->setRelation(3, QSqlRelation("Release", "releaseid", "releasename"));

	rel_table_model->setHeaderData(0, Qt::Horizontal, tr("ID"));
	rel_table_model->setHeaderData(1, Qt::Horizontal, tr("Media URL"));
	rel_table_model->setHeaderData(2, Qt::Horizontal, tr("Sidecar Cuesheet URL"));
	rel_table_model->setHeaderData(3, Qt::Horizontal, tr("Release Name"));

	rel_table_model->select();

	bool status = rel_table_model->submitAll();
    Q_ASSERT_X(status, "", "SUBMIT FAILED");

	m_relational_table_model = rel_table_model;

	LogModelInfo(m_relational_table_model);

	m_prepped_insert_query = new QSqlQuery(db_conn);
	status = m_prepped_insert_query->prepare(QLatin1String(
		"INSERT INTO DirScanResults(media_url, sidecar_cuesheet_url, dirscanrelease) values (?, ?, ?)"));
	Q_ASSERT(status);


	return rel_table_model;
}

void CollectionDatabaseModel::LogModelInfo(QSqlRelationalTableModel* model) const
{
	qIno() << "RelTableModel info:";
	qIno() << "  Primary key:" << model->primaryKey().name();
	qIno() << "  Is dirty:" << model->isDirty();
	qIno() << "  DB connection name:" << model->database().connectionName();
	qIno() << "  Current Edit Strategy:" << model->editStrategy();
}

QSqlError CollectionDatabaseModel::SLOT_addDirScanResult(DirScanResult dsr)
{
#if 0
	addDirScanResult(*m_prepped_insert_query, dsr);
#elif 0
	qDb() << "GETTING NEWREC";
    QSqlRecord newrec = m_relational_table_model->record();
	qDb() << "GOT NEWREC";// << newrec;

	newrec.setValue("media_url", QVariant(dsr.getMediaQUrl().toString()));
	QUrl sidecar_cuesheet = dsr.getSidecarCuesheetQUrl();
	if(sidecar_cuesheet.isValid())
	{
		newrec.setValue("sidecar_cuesheet_url", QVariant(dsr.getSidecarCuesheetQUrl().toString()));
	}
//    if(release != 0)
//    {
//        newrec.setValue("releasename", release);// QVariant(/*release*/"Free Fallin"));//1);
//    }
//    qDb() << "NEWREC:" << newrec;

    if(m_relational_table_model->insertRecord(-1, newrec))
    {
        qDb() << "INSERTRECORD SUCCEEDED:";
		bool status = m_relational_table_model->submit();
		qDb() << "SUBMIT SUCCEEDED:";
		Q_ASSERT_X(status, "", "SUBMIT FAILED");
    }
    else
    {
        Q_ASSERT_X(0, "", "INSERTRECORD FAILED");
    }
#else
	int row = m_relational_table_model->rowCount();
	qDbo() << "ROW:" << row;
	bool status = m_relational_table_model->insertRow(row);
	Q_ASSERT(status);
	auto index = m_relational_table_model->index(row, 1);
	status = m_relational_table_model->setData(index, QVariant(dsr.getMediaQUrl().toString()));
	Q_ASSERT(status);
	m_relational_table_model->submit();
#endif

	return QSqlError();
}

QVariant CollectionDatabaseModel::addDirScanResult(QSqlQuery& q, const DirScanResult& dsr)
{
	qDbo() << "START";

	q.addBindValue(dsr.getMediaQUrl());
	q.addBindValue(dsr.getSidecarCuesheetQUrl());
	q.addBindValue(QVariant());
	q.exec();
	qDbo() << "END";
	return q.lastInsertId();
}


QVariant CollectionDatabaseModel::RunQuery(const QString &query, QSqlDatabase& db_conn)
{
	QVariant retval;

    QSqlQuery q(query, db_conn);
    if(!q.exec())
    {
        qWro() << "QUERY FAILED:" << q.lastError() << q.lastQuery();
		return retval;
    }
    qDbo() << "QUERY:" << q.lastQuery();
    while(q.next())
    {
        qDbo() << "QUERY RESULT:" << q.value(0);
		retval = q.value(0);
    }

	return retval;
}

bool CollectionDatabaseModel::IfExistsAskForDelete(const QUrl &filename)
{
    QFile the_file(filename.toLocalFile());
    if(the_file.exists())
    {
        QMessageBox::StandardButton retval = QMessageBox::warning(MainWindow::instance(), tr("File exists"),
                                                                  tr("The file '%1' already exists.\n"
                                                               "Do you want to delete it?").arg(filename.toLocalFile()),
                                          QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if(retval == QMessageBox::Yes)
        {
            return the_file.remove();
        }
        else
        {
            return false;
        }
    }
    // else file doesn't exist.
	return true;
}

QSqlDatabase CollectionDatabaseModel::AddInitAndOpenDBConnection(const QUrl& db_file, const QString& connection_name,
																	bool write, bool create)
{
	qDb() << "CIO database connection:" << db_file << "connection_name:" << connection_name;

	// Add a database to the list of db connections using the QSQLITE driver,
	// and create a connection named m_connection_name.
	// Note that this connection can only be used in this thread which created it; to access the db
	// from other threads, that thread has to close or create a new connection.
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connection_name);

	if(!db.isValid())
	{
		qCro() << "Database connection is invalid:" << db;
		return db;
	}

	// Set up the database connection.
	// We have to do this setup prior to the final step of calling .open().

	// Enable regexes, OPEN_URI, and optionally read-only.
	/// @see @link https://www.sqlite.org/c3ref/open.html and @link https://www.sqlite.org/uri.html
	///  as to why we want URI filename support.
	/// Driver source: @link https://code.woboq.org/qt5/qtbase/src/plugins/sqldrivers/sqlite/qsql_sqlite.cpp.html#576openUriOption
	/// @note Seems that OPEN_URI doesn't work.
	db.setConnectOptions("QSQLITE_ENABLE_REGEXP=1;"); //QSQLITE_OPEN_URI=1

	if(!write)
	{
		db.setConnectOptions("QSQLITE_OPEN_READONLY=1;");
	}

	// Set the filename of the DB.
	db.setDatabaseName(db_file.toLocalFile());

	LogDriverFeatures(db.driver());

	// Finally open the db.
	if (!db.open()) {
		QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
			QObject::tr("Unable to establish a database connection.\n"
						"This program needs SQLite support. Please read "
						"the Qt SQL driver documentation for information how "
						"to build it.\n\n"
						"Click Cancel to exit."), QMessageBox::Cancel);
		return db;
	}

	// PRAGMAs.  Enable foreign key support.
	ApplyPragmas(db);

	LogConnectionInfo(db);
	RunQuery("PRAGMA foreign_keys;", db);

	/// @todo Final check if the DB exists and looks to be in good condition.

	return db;
}

QSqlError CollectionDatabaseModel::SqlPRAGMA(QSqlDatabase& db_conn, const QString& str)
{
	QSqlQuery q = db_conn.exec(QString("PRAGMA %1;").arg(str));
	if(db_conn.lastError().type() != QSqlError::NoError)
	{
		qWr() << "PRAGMA FAILED";
	}
	return db_conn.lastError();
}

QSqlError CollectionDatabaseModel::ApplyPragmas(QSqlDatabase& db_conn)
{
	// Any applied PRAGMAs don't appear to make it through a cloning, so we factor them
	// out here so we can apply them as needed.

	QSqlError err = SqlPRAGMA(db_conn, "foreign_keys = 1");
	Q_ASSERT(err.type() == QSqlError::NoError);

	return err;
}



void CollectionDatabaseModel::InitializeModel()
{
    m_relational_table_model = new QSqlRelationalTableModel(this);
}

