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

#include "ModelChangeWatcher.h"

#include <QAbstractItemModel>
#include <QDebug>

ModelChangeWatcher::ModelChangeWatcher(QObject *parent) : QObject(parent)
{
}

void ModelChangeWatcher::setModelToWatch(QAbstractItemModel* model)
{
	m_the_model = model;

	// Connect to all the signals which might change the number of rows.
	connect(m_the_model, &QAbstractItemModel::modelReset, this, &ModelChangeWatcher::onRowCountChanged);
	QObject::connect(m_the_model, SIGNAL(rowsInserted()), this, SLOT(onRowCountChanged()));
	QObject::connect(m_the_model, SIGNAL(rowsRemoved()), this, SLOT(onRowCountChanged()));

}

void ModelChangeWatcher::onRowCountChanged()
{
	qDebug() << "EMITTING rowCountChanged for model" << m_the_model;
	emit rowCountChanged();
}
