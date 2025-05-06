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

/// @file

#include "ExpandingFrameWidget.h"

// Qt
#include <QLayout>
#include <QWidget>

// Ours
#include <utils/TheSimplestThings.h>
#include <gui/MainWindow.h>

ExpandingFrameWidget::ExpandingFrameWidget(QWidget *main_progress_bar_widget, QWidget *parent) : BASE_CLASS(parent,
                                                                               Qt::Window | Qt::FramelessWindowHint | Qt::ToolTip)
{
    m_cumulative_status_bar_main_widget = main_progress_bar_widget;

    setWindowFlags(Qt::WindowDoesNotAcceptFocus | windowFlags());
    qApp->installEventFilter(this);

    // Set up the layout.
    Q_ASSERT(layout() == nullptr);
    setLayout(new QVBoxLayout());
    layout()->setSpacing(0);
//QT6 No member: layout()->setMargin(0);
    layout()->setSizeConstraint(QLayout::SetFixedSize);

    setBackgroundRole( QPalette::Window );
    setAutoFillBackground( true );

    setMinimumWidth( 26 );
    setMinimumHeight( 26 );

    setContentsMargins( 4, 4, 4, 4 );
    // Make the widget size conform to the size of the contained widgets,
    // i.e. the user can't resize it.
    layout()->setSizeConstraint(QLayout::SetFixedSize);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    updateGeometry();
}

ExpandingFrameWidget::~ExpandingFrameWidget()
{
}

void ExpandingFrameWidget::setVisible(bool visible)
{
    BASE_CLASS::setVisible(visible);
    Q_EMIT visibilityChanged(visible);
}

void ExpandingFrameWidget::addWidget(QWidget *new_widget)
{
    // Set the widget width to the same as the summary widget.
    new_widget->setFixedWidth(m_cumulative_status_bar_main_widget->width());

    // Widget will be reparented.
    layout()->addWidget(new_widget);

    reposition();
}

void ExpandingFrameWidget::removeWidget(QWidget *new_widget)
{
    layout()->removeWidget(new_widget);

    reposition();
}

QSize ExpandingFrameWidget::sizeHint() const
{
    return BASE_CLASS::sizeHint();
}

void ExpandingFrameWidget::reposition()
{
	if(m_cumulative_status_bar_main_widget == nullptr)
    {
        return;
    }

    QPoint p;
    p.setX(m_cumulative_status_bar_main_widget->width() - width());
    p.setY(-height());
    auto global_point = m_cumulative_status_bar_main_widget->mapToGlobal(p);
    move(global_point);
}

void ExpandingFrameWidget::resizeEvent(QResizeEvent *ev)
{
    reposition();
    BASE_CLASS::resizeEvent(ev);
}

bool ExpandingFrameWidget::eventFilter(QObject *o, QEvent *e)
{
    if(e->type() == QEvent::Move || e->type() == QEvent::Resize)
    {
        reposition();
    }
    else if(e->type() == QEvent::Close)
    {
        close();
    }

    return BASE_CLASS::eventFilter(o,e);
}

