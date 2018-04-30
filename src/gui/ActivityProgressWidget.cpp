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

#include "ActivityProgressWidget.h"

// Qt5

#include <QLabel>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QDebug>
#include <QToolButton>

// KF5

#include <ThreadWeaver/QObjectDecorator>

// Ours

#include "utils/DebugHelpers.h"


ActivityProgressWidget::ActivityProgressWidget(QWidget *parent, const Qt::WindowFlags &f) : QWidget(parent, f)
{
	m_current_activity_label = new QLabel("Idle", this);
	m_current_activity_label->setToolTip("Current operation");
	m_current_activity_label->setWhatsThis("This text shows the current operation in progress.");

	m_text_status_label = new QLabel("Idle", this);
	m_text_status_label->setToolTip("Status of the current operation");
	m_text_status_label->setWhatsThis("This text shows the status of the current operation in progress.");

	m_progress_bar = new QProgressBar(this);

    // Pause/Resume button
    auto button_pause = new QToolButton(this);
    button_pause->setIcon(QIcon::fromTheme("media-playback-pause"));
//    button_pause->setMenu(menu);
    button_pause->setStyleSheet("* { border: none; }");

    // Stop button.
    auto button_stop = new QToolButton(this);
    button_stop->setIcon(QIcon::fromTheme("process-stop"));
//    button_pause->setMenu(menu);
    button_stop->setStyleSheet("* { border: none; }");

    // Expand jobs button.
    auto button_jobs = new QToolButton(this);
    button_jobs->setArrowType(Qt::UpArrow); // Instead of a normal icon.

	auto layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_current_activity_label);
	layout->addWidget(m_text_status_label);
	layout->addWidget(m_progress_bar);
    layout->addWidget(button_pause);
    layout->addWidget(button_stop);
    layout->addWidget(button_jobs);
	setLayout(layout);
}

ActivityProgressWidget::~ActivityProgressWidget()
{

}

void ActivityProgressWidget::onProgressChanged(int min, int val, int max, QString text)
{
	m_progress_bar->setRange(min, max);
	m_progress_bar->setValue(val);
	m_current_activity_label->setText(text);

	m_last_min = min;
	m_last_max = max;
	m_last_val = val;

    m_text_status_label->setText(createTextStatusString());
}

void ActivityProgressWidget::addActivity(ThreadWeaver::QObjectDecorator* activity)
{
    connect(activity, &ThreadWeaver::QObjectDecorator::done, this, &ActivityProgressWidget::onTWJobDone);
}

void ActivityProgressWidget::addActivity(AMLMJob *activity)
{
    //    connect();
}

void ActivityProgressWidget::TrackJob(KJob *job)
{

}

void ActivityProgressWidget::onTWJobDone(ThreadWeaver::JobPointer job)
{
M_WARNING("DO SOMETHING HERE");
    qDb() << "JOB COMPLETE, STATUS:" << job->status();
}

QString ActivityProgressWidget::createTextStatusString()
{
	return QString("%1/%2").arg(m_last_val).arg(m_last_max-m_last_min);
}
