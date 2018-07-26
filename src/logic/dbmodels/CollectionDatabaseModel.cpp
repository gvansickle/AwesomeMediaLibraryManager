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

/// Ours
#include <utils/DebugHelpers.h>

CollectionDatabaseModel::CollectionDatabaseModel(QObject* parent) : QObject(parent)
{

}

CollectionDatabaseModel::~CollectionDatabaseModel()
{

}

QSqlError CollectionDatabaseModel::InitDb(QUrl db_file)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", /*connectionName=*/ "experimental_db_connection");

    /// @todo TEMP hardcoded db file name in home dir.
    auto db_dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString ab_file = db_dir + "/AMLMTestdb.sqlite3";

//    db.setDatabaseName(":memory:");
    db.setDatabaseName(ab_file);

    // Enable regexes.
    db.setConnectOptions("QSQLITE_ENABLE_REGEXP=1");
    db.setConnectOptions("PRAGMA foreign_keys = 1;");

    QSqlQuery q = db.exec("PRAGMA foreign_keys = 1;");
    if(db.lastError().type() != QSqlError::NoError)
    {
        qWro() << "PRAGMA FAILED";
    }

    // Open the db.
    if (!db.open()) {
        QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
            QObject::tr("Unable to establish a database connection.\n"
                        "This program needs SQLite support. Please read "
                        "the Qt SQL driver documentation for information how "
                        "to build it.\n\n"
                        "Click Cancel to exit."), QMessageBox::Cancel);
        return db.lastError();
    }

    /// @todo Return if the DB exists and looks to be in good condition.
    if(false /* DB looks good. */)
    {
        return QSqlError();
    }

    CreatePrimaryTables(db);
    CreateRelationalTables(db);

    db.commit();

    return QSqlError();
}

QSqlError CollectionDatabaseModel::CreatePrimaryTables(QSqlDatabase &db)
{
//    auto db_conn = QSqlDatabase::database("experimental_db_connection");
    auto& db_conn = db;

    QStringList tables;

    tables.append("CREATE TABLE DirScanResults ("
                  "dirscanid INTEGER PRIMARY KEY,"
                  "media_url TEXT NOT NULL,"
                  "sidecar_cuesheet_url TEXT,"
                  /// @todo experimental
                  "dirscanrelease INTEGER REFERENCES Release"
                  ")");
    tables.append("CREATE TABLE Artist ("
                  "artistid INTEGER PRIMARY KEY,"
                  "name TEXT"
                  ")");
    tables.append("CREATE TABLE Release ("
                  "releaseid INTEGER PRIMARY KEY,"
                  "name TEXT,"
                  "releaseartist INTEGER REFERENCES Artist"
                  ")");

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

QSqlError CollectionDatabaseModel::CreateRelationalTables(QSqlDatabase& db)
{
//    auto db = QSqlDatabase::database("experimental_db_connection");

//    QSqlQuery q(db);
//    if(!q.exec(""))
//    {
//        return q.lastError();
//    }

    return QSqlError();
}

QSqlRelationalTableModel *CollectionDatabaseModel::make_reltable_model(QObject *parent)
{
    qDbo() << "Here";
    auto db_conn = QSqlDatabase::database("experimental_db_connection");
    Q_ASSERT(db_conn.isValid());
    // Enable regexes.
    db_conn.setConnectOptions("QSQLITE_ENABLE_REGEXP=1");
    db_conn.setConnectOptions("PRAGMA foreign_keys = 1;");
    QSqlQuery q("PRAGMA foreign_keys = 1", db_conn);
    if(!q.exec())
    {
        qWro() << "PRAGMA FAILED";
    }
//    db_conn.setConnectOptions("PRAGMA foreign_keys = ON");

    QSqlRelationalTableModel* rel_table_model = new QSqlRelationalTableModel(parent, db_conn);
    Q_CHECK_PTR(rel_table_model);

    rel_table_model->setTable("DirScanResults");
    rel_table_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    // Left join to show rows with NULL foreign keys.
    rel_table_model->setJoinMode(QSqlRelationalTableModel::LeftJoin);

    rel_table_model->setRelation(4, QSqlRelation("Release", "releaseid", "name"));

    rel_table_model->setHeaderData(0, Qt::Horizontal, tr("HEADER_Id"));
    rel_table_model->setHeaderData(1, Qt::Horizontal, tr("HEADER_Media_Url"));
    rel_table_model->setHeaderData(2, Qt::Horizontal, tr("HEADER_SidecarCuesheet_Url"));
    rel_table_model->setHeaderData(3, Qt::Horizontal, tr("HEADER_Release_name"));

    rel_table_model->select();

    bool status = rel_table_model->submitAll();
    Q_ASSERT_X(status, "", "SUBMIT FAILED");
    m_relational_table_model = rel_table_model;

    return rel_table_model;
}

QSqlError CollectionDatabaseModel::addDirScanResult(const QUrl &medi_url)
{
//    QSqlQuery q;
//    if(!q.prepare(QLatin1String("INSERT INTO ")));

    QSqlRecord newrec = m_relational_table_model->record();

    qDb() << "NEWREC:" << newrec;
    newrec.setValue("media_url", QVariant(medi_url.toString()));
    qDb() << "NEWREC:" << newrec;

    bool status = m_relational_table_model->insertRecord(0, newrec);
    qDb() << "INSERT STATUS:" << status;
    m_relational_table_model->select();

    status = m_relational_table_model->submitAll();
    Q_ASSERT_X(status, "", "SUBMIT FAILED");

    return QSqlError();
}

QVariant CollectionDatabaseModel::addMediaUrl(QSqlQuery &q, const QUrl &url)
{


}

void CollectionDatabaseModel::InitializeModel()
{
    m_relational_table_model = new QSqlRelationalTableModel(this);
}

