/*
 * ActivityProgressPopup.cpp
 *
 *  Created on: Apr 30, 2018
 *      Author: gary
 */

#include "ActivityProgressPopup.h"

#include <QToolButton>

#include <KMessageWidget>


ActivityEntry::ActivityEntry(QObject* parent) : QWidgetAction(parent)
{

}

ActivityEntry::~ActivityEntry()
{

}

QWidget* ActivityEntry::createWidget(QWidget *parent)
{
    auto kmsg_wdgt = new KMessageWidget(tr("KMessageWidget test"), parent);
    kmsg_wdgt->setCloseButtonVisible(true);

    // Remove the QWidgetAction when the activity is done.
    connect(kmsg_wdgt, &KMessageWidget::hideAnimationFinished, [=]() { parent->removeAction(this); });

    return kmsg_wdgt;
}

void ActivityEntry::deleteWidget(QWidget *widget)
{
    QWidgetAction::deleteWidget(widget);
}


ActivityProgressPopup::ActivityProgressPopup(QWidget *parent) : QWidget(parent)
{
    auto button_jobs = new QToolButton();
    button_jobs->setArrowType(Qt::UpArrow); // Instead of a normal icon.

    auto progress_list_action = new QWidgetAction(this);
    progress_list_action->setDefaultWidget(button_jobs);
    addAction(progress_list_action);
}



