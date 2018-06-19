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

/// Ours
#include <utils/DebugHelpers.h>

CollectionDatabaseModel::CollectionDatabaseModel(QObject* parent) : QObject(parent)
{

}

CollectionDatabaseModel::~CollectionDatabaseModel()
{

}

bool CollectionDatabaseModel::open_db_connection(QUrl db_file)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", /*connectionName=*/ "experimental_db_connection");

    /// @todo TEMP hardcoded db file name in home dir.
    auto db_dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString ab_file = db_dir + "/AMLMTestdb.sqlite3";

//    db.setDatabaseName(":memory:");
    db.setDatabaseName(ab_file);

    // Enable regexes.
    db.setConnectOptions("QSQLITE_ENABLE_REGEXP=1");

    // Open the db.
    if (!db.open()) {
        QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
            QObject::tr("Unable to establish a database connection.\n"
                        "This program needs SQLite support. Please read "
                        "the Qt SQL driver documentation for information how "
                        "to build it.\n\n"
                        "Click Cancel to exit."), QMessageBox::Cancel);
        return false;
    }

    return true;
}

void CollectionDatabaseModel::create_db_tables(QSqlDatabase *db)
{
    auto db_conn = QSqlDatabase::database("experimental_db_connection");

    QStringList tables;

    tables.append("CREATE TABLE IF NOT EXISTS DirScanResults ("
                  "id INTEGER DEFAULT NULL PRIMARY KEY AUTOINCREMENT,"
                  "url TEXT NOT NULL"
                  ")");
    for (int i = 0; i < tables.count(); ++i)
    {
        QSqlQuery query(db_conn);
        if (!query.exec(tables[i]))
        {
            qDb() << query.lastError();
            qDb() << query.executedQuery();
        }
    }
}

QSqlRelationalTableModel *CollectionDatabaseModel::get_rel_table(QObject *parent)
{
    auto db_conn = QSqlDatabase::database("experimental_db_connection");
    Q_ASSERT(db_conn.isValid());

    QSqlRelationalTableModel* rel_table_model = new QSqlRelationalTableModel(parent, db_conn);
    Q_CHECK_PTR(rel_table_model);

    rel_table_model->setTable("DirScanResults");
    rel_table_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    rel_table_model->select();
    rel_table_model->setHeaderData(0, Qt::Horizontal, tr("HEADER_Id"));
    rel_table_model->setHeaderData(1, Qt::Horizontal, tr("HEADER_Url"));
    bool status = rel_table_model->submitAll();
    Q_ASSERT_X(status, "", "SUBMIT FAILED");
    return rel_table_model;
}

