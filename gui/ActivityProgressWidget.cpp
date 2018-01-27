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

#include <nomocimpl.h>

#include <QLabel>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QDebug>

W_OBJECT_IMPL(ActivityProgressWidget)

ActivityProgressWidget::ActivityProgressWidget(QWidget *parent, const Qt::WindowFlags &f) : QWidget(parent, f)
{
	m_current_activity_label = new QLabel("Idle", this);
	m_current_activity_label->setToolTip("Current operation");
	m_current_activity_label->setWhatsThis("This text shows the current operation in progress.");

	m_text_status_label = new QLabel("Idle", this);
	m_text_status_label->setToolTip("Status of the current operation");
	m_text_status_label->setWhatsThis("This text shows the status of the current operation in progress.");

	m_progress_bar = new QProgressBar(this);

	auto layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_current_activity_label);
	layout->addWidget(m_text_status_label);
	layout->addWidget(m_progress_bar);
	setLayout(layout);
}

ActivityProgressWidget::~ActivityProgressWidget()
{

}

void ActivityProgressWidget::onProgressRangeChanged(int minimum, int maximum)
{
	m_progress_bar->setRange(minimum, maximum);
	m_last_min = minimum;
	m_last_max = maximum;

	m_text_status_label->setText(createTextStatusString());
}

void ActivityProgressWidget::onProgressTextChanged(const QString &progressText)
{
	m_current_activity_label->setText(progressText);
}

void ActivityProgressWidget::onProgressValueChanged(int progressValue)
{
	m_progress_bar->setValue(progressValue);
	m_last_val = progressValue;
	m_text_status_label->setText(createTextStatusString());
}

QString ActivityProgressWidget::createTextStatusString()
{
	return QString("%1/%2").arg(m_last_val).arg(m_last_max-m_last_min);
}
