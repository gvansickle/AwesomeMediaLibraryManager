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

#ifndef MODELCHANGEWATCHER_H
#define MODELCHANGEWATCHER_H

/**
 * @file
 */

// Qt
#include <QObject>
#include <QPointer>

class QAbstractItemModel;



class ModelChangeWatcher : public QObject
{
    Q_OBJECT
    
Q_SIGNALS:

	void modelHasRows(bool);

public:
    explicit ModelChangeWatcher(QObject *parent = Q_NULLPTR);
    ~ModelChangeWatcher() override {};
    
    void setModelToWatch(QAbstractItemModel* model);
	void disconnectFromCurrentModel();

protected Q_SLOTS:
    void onRowCountChanged();
    
private:
    Q_DISABLE_COPY_MOVE(ModelChangeWatcher)

    /// Non-owning pointer to the model we're watching.
	QPointer<QAbstractItemModel> m_the_model { nullptr };
};

#endif /* MODELCHANGEWATCHER_H */

