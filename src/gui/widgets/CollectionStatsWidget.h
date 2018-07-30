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

#ifndef COLLECTIONSTATSWIDGET_H
#define COLLECTIONSTATSWIDGET_H

// Qt5
#include <QLabel>
#include <QPointer>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QWidget>

// Ours
#include <logic/proxymodels/ModelChangeWatcher.h>
#include <logic/LibraryModel.h>

class CollectionStatsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CollectionStatsWidget(QWidget *parent = nullptr);

    void setModel(QPointer<LibraryModel> model);

Q_SIGNALS:

public Q_SLOTS:

    void SLOT_modelChanged();

protected:

//    QLabel* m_widget_text;
    QTextEdit* m_widget_text;
    QPointer<LibraryModel> m_sources_model;

    ModelChangeWatcher* m_sources_model_watcher { nullptr };
};

#endif // COLLECTIONSTATSWIDGET_H