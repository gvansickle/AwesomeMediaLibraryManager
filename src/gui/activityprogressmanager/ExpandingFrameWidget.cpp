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

#include <QLayout>

#include "ExpandingFrameWidget.h"

ExpandingFrameWidget::ExpandingFrameWidget(QWidget *parent) : BASE_CLASS(parent)
{
    // Set up the layout.
    Q_ASSERT(layout() == nullptr);
    setLayout(new QVBoxLayout(this));
    layout()->setSpacing(0);
    layout()->setMargin(0);
//    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
//    setWindowFlags(Qt::Tool);//Qt::Dialog);
}

ExpandingFrameWidget::~ExpandingFrameWidget()
{
    // TODO Auto-generated destructor stub
}

void ExpandingFrameWidget::addWidget(QWidget *new_widget)
{
    // Widget will be reparented.
    layout()->addWidget(new_widget);

//    // Resize if necessary.
//    if(width() < new_widget->width())
//    {
//        setMinimumWidth(new_widget->width());
//    }

//    /// @todo
//    setMinimumHeight(new_widget->height());
}

void ExpandingFrameWidget::removeWidget(QWidget *new_widget)
{
//    new_widget->setParent(this);
    layout()->removeWidget(new_widget);
}

