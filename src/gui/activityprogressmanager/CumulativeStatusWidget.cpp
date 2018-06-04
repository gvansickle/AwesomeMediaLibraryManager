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

#include "CumulativeStatusWidget.h"

/// Qt5
#include <QToolButton>
#include <QLabel>

/// Ours
#include "utils/TheSimplestThings.h"


CumulativeStatusWidget::CumulativeStatusWidget(KJob* job, ActivityProgressStatusBarTracker* tracker, QWidget *parent)
    : BASE_CLASS(nullptr, tracker, parent)
{
    /// @note Requires base class init() to have been called so that sub-widgets are set up.

    // Add an "Expand jobs" button.
    auto button_show_all_jobs = new QToolButton(this);
    button_show_all_jobs->setPopupMode(QToolButton::InstantPopup);
    button_show_all_jobs->setArrowType(Qt::UpArrow); // Instead of a normal icon.
    button_show_all_jobs->setCheckable(true);

    addButton(button_show_all_jobs);

M_WARNING("TODO: This should depend on contained jobs count/state");
    m_cancel_button->setEnabled(true);

    connect(button_show_all_jobs, &QToolButton::toggled, this, &CumulativeStatusWidget::show_hide_subjob_display);
}

CumulativeStatusWidget::~CumulativeStatusWidget()
{

}

void CumulativeStatusWidget::slot_number_of_jobs_changed(long long new_num_jobs)
{
	// Update the subwidget contents.
    if(new_num_jobs > 0)
    {
        m_current_activity_label->setText(tr("Running"));
    }
    else
    {
        m_current_activity_label->setText(tr("Idle"));
    }
    m_text_status_label->setText(tr("%1").arg(new_num_jobs));
}

