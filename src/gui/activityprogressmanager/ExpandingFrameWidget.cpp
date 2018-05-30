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

#include "ExpandingFrameWidget.h"

/// Qt5
#include <QLayout>
#include <QWidget>

/// Ours
#include <utils/TheSimplestThings.h>
#include <gui/MainWindow.h>

ExpandingFrameWidget::ExpandingFrameWidget(QWidget *parent, Qt::WindowFlags f) : BASE_CLASS(parent, f)
{
    /// GRVS
//    setWindowFlags(Qt::Dialog);
//    show();

    // Set up the layout.
    Q_ASSERT(layout() == nullptr);
    setLayout(new QVBoxLayout());
    layout()->setSpacing(0);
    layout()->setMargin(0);
    layout()->setSizeConstraint(QLayout::SetFixedSize);

    setBackgroundRole( QPalette::Window );
    setAutoFillBackground( true );

//    setFrameStyle( QFrame::Box );

    setMinimumWidth( 26 );
    setMinimumHeight( 26 );

    setContentsMargins( 4, 4, 4, 4 );
    // Make the frame confirm to the size of the contained widgets,
    // i.e. the user can't resize it.
    layout()->setSizeConstraint(QLayout::SetFixedSize);
//    QSizePolicy sp();
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    updateGeometry();

//    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
//    layout()->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

}

ExpandingFrameWidget::~ExpandingFrameWidget()
{
    // TODO Auto-generated destructor stub
}

void ExpandingFrameWidget::setMainProgressWidget(QWidget *status_bar_widget)
{
    m_cumulative_status_bar_main_widget = status_bar_widget;
}

void ExpandingFrameWidget::addWidget(QWidget *new_widget)
{
    // Set the widget width to the same as the summary widget.
    new_widget->setFixedWidth(parentWidget()->width());
    // Widget will be reparented.
    layout()->addWidget(new_widget);

    reposition();

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

//    setFixedHeight(new_widget->height() * children().size() + 8);
    reposition();
}

QSize ExpandingFrameWidget::sizeHint() const
{
    return BASE_CLASS::sizeHint();
///
    // parent is also the status bar widget.
    auto sbw = parentWidget();

    qDb() << "PARENT SIZE HINT:" << sbw->sizeHint();

    return QSize(sbw->width(), 10);
}

void ExpandingFrameWidget::reposition()
{
    qDb() << "PARENT SIZE HINT:" << parentWidget()->sizeHint();

//    QSize s = sizeHint();
//    s.setWidth(parentWidget()->width());
//    resize(s);
    updateGeometry();

//    // parent is also the status bar widget.
////    auto sbw = parentWidget();

////    // Set the width of this to match the parent.
//////    setMinimumWidth(sbw->width());
//////    setMaximumWidth(sbw->width());
////    qDb() << "SIZE HINT:" << sbw->sizeHint();
////    setMinimumSizeHint(sbw->sizeHint().width(), sbw->sizeHint().height());

////    // Per instructions, call this after dinking with the widget's sizeHint()/Policy().
////    updateGeometry();

////    // Adjust the widget size to fit contents (via sizeHint()).
////    adjustSize();

////    if(!MainWindow::instance())
////        return;

////    QPoint p;
////    p.setX(MainWindow::instance()->width() - width());
////    p.setY(MainWindow::instance()->height() - height());
////    move(p);
}

