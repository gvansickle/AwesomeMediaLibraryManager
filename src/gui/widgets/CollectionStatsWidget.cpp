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

#include "CollectionStatsWidget.h"

#include <type_traits>

#include <QLabel>

CollectionStatsWidget::CollectionStatsWidget(QWidget *parent) : QWidget(parent)
{
    // No layout, specify how to size this widget's children.
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

//    m_widget_text = new QLabel(tr("Hello"), this);
    m_widget_text = new QTextEdit(tr("Hello"), this);
    m_widget_text->setReadOnly(true);
//    m_widget_text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_sources_model_watcher = new ModelChangeWatcher(this);

    connect(m_sources_model_watcher, &ModelChangeWatcher::modelHasRows, this, &std::decay_t<decltype(*this)>::SLOT_modelChanged);
}

void CollectionStatsWidget::setModel(QPointer<LibraryModel> model)
{
    m_sources_model = model;
    m_sources_model_watcher->setModelToWatch(m_sources_model);
}

void CollectionStatsWidget::SLOT_modelChanged()
{
    // Model changed, update stats.
    auto num_tracks = m_sources_model->rowCount();
    QString new_txt = "<h3>Collection Stats</h3>";
    new_txt += QString("Number of tracks: %1").arg(num_tracks);
    m_widget_text->setText(new_txt);
}
