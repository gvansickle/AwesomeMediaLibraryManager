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

#include <gui/activityprogressmanager/ActivityEntryWidget.h>
#include <QObject>

#include <KMessageWidget>


ActivityEntryWidget::ActivityEntryWidget(QObject* parent) : QWidgetAction(parent)
{

}

ActivityEntryWidget::~ActivityEntryWidget()
{

}

QWidget* ActivityEntryWidget::createWidget(QWidget *parent)
{
    auto kmsg_wdgt = new KMessageWidget(tr("KMessageWidget test"), parent);
    kmsg_wdgt->setCloseButtonVisible(true);

    // Remove the QWidgetAction when the activity is done.
    connect(kmsg_wdgt, &KMessageWidget::hideAnimationFinished, [=]() { parent->removeAction(this); });

    return kmsg_wdgt;
}

void ActivityEntryWidget::deleteWidget(QWidget *widget)
{
    QWidgetAction::deleteWidget(widget);
}


