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
/// @file

#include "ModelChangeWatcher.h"

#include <ConnectHelpers.h>
#include <QAbstractItemModel>
#include <QDebug>

ModelChangeWatcher::ModelChangeWatcher(QObject *parent) : QObject(parent)
{
}

void ModelChangeWatcher::setModelToWatch(QAbstractItemModel* model)
{
//	qDebug() << "Connecting to model" << model;

	// Disconnect any currently connected model.
	if(m_the_model)
	{
		// Disconnect all signals from m_the_model to this.
		m_the_model->disconnect(this);
	}

	m_the_model = model;

	// Connect to all the signals which might change the number of rows.
	connect_or_die(m_the_model, &QAbstractItemModel::modelReset, this, &ModelChangeWatcher::onRowCountChanged);
	connect_or_die(m_the_model, &QAbstractItemModel::rowsInserted, this, &ModelChangeWatcher::onRowCountChanged);
	connect_or_die(m_the_model, &QAbstractItemModel::rowsRemoved, this, &ModelChangeWatcher::onRowCountChanged);

}

void ModelChangeWatcher::disconnectFromCurrentModel()
{
//	qDebug() << "Disconnecting from model" << m_the_model;

	if(m_the_model)
	{
		// Disconnect all signals from m_the_model to this.
		m_the_model->disconnect(this);
	}

	m_the_model = nullptr;
}

void ModelChangeWatcher::onRowCountChanged()
{
//	qDebug() << "EMITTING rowCountChanged for model" << m_the_model << "num rows:" << m_the_model->rowCount();
	Q_EMIT modelHasRows(m_the_model->rowCount() > 0);
}
