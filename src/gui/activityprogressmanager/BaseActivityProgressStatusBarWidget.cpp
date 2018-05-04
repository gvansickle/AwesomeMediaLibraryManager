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

#include "BaseActivityProgressStatusBarWidget.h"

#include <QLabel>
#include <QToolButton>
#include <QProgressBar>
#include <QHBoxLayout>

#include <KJob>

#include <gui/helpers/Tips.h>
#include <utils/DebugHelpers.h>

BaseActivityProgressStatusBarWidget::BaseActivityProgressStatusBarWidget(QWidget *parent) : BASE_CLASS(parent)
{

}

BaseActivityProgressStatusBarWidget::BaseActivityProgressStatusBarWidget(KJob *job, QWidget *parent)
    : BaseActivityProgressStatusBarWidget(parent)
{
    // We have a vtable to this, go nuts with the virtual calls.
    init(job, parent);
}

BaseActivityProgressStatusBarWidget::~BaseActivityProgressStatusBarWidget()
{

}

void BaseActivityProgressStatusBarWidget::setDescription(const QString &title, const QPair<QString, QString> &field1, const QPair<QString, QString> &field2)
{
    /// @todo Don't really have anywhere to put fields.
    Q_UNUSED(field1);
    Q_UNUSED(field2);
    m_current_activity_label->setText(title);
}

void BaseActivityProgressStatusBarWidget::setInfoMessage(const QString &text)
{
    m_text_status_label->setText(text);
}

void BaseActivityProgressStatusBarWidget::setWarning(const QString &text)
{
M_WARNING("TODO");
    qWr() << text;
}

void BaseActivityProgressStatusBarWidget::setRange(int min, int max)
{
    m_progress_bar->setRange(min, max);
}

void BaseActivityProgressStatusBarWidget::setValue(int val)
{
    m_progress_bar->setValue(val);
}

void BaseActivityProgressStatusBarWidget::init(KJob *job, QWidget *parent)
{
    // Create the widget.
    /// @link https://github.com/KDE/kjobwidgets/blob/master/src/kstatusbarjobtracker.cpp

    m_current_activity_label = new QLabel(tr("Idle"), parent);
    m_current_activity_label->setToolTip("Current operation");
    m_current_activity_label->setWhatsThis("This text shows the current operation in progress.");

    // This is for displaying the KJob::infoMessage().
    // ""Resolving host", "Connecting to host...", etc."
    m_text_status_label = new QLabel(tr("Idle"), parent);
    m_text_status_label->setToolTip("Status of the current operation");
    m_text_status_label->setWhatsThis("This text shows the status of the current operation in progress.");

    // The progress bar.
    m_progress_bar = new QProgressBar(parent);
    m_progress_bar->setFormat(tr("%p%"));
    m_progress_bar->setTextVisible(true);

    // The buttons.
    m_cancel_button = new QToolButton(parent);
    m_cancel_button->setIcon(QIcon::fromTheme("process-stop"));
    setTips(m_cancel_button, tr("Abort"), tr("Abort this operation"), tr("<h3>Abort Button</h3><br/>Stops the operation"));
    m_pause_resume_button = new QToolButton(parent);
    m_pause_resume_button->setIcon(QIcon::fromTheme("media-playback-pause"));
    setTips(m_pause_resume_button, tr("Pause/Resume"), tr("Pause or resume this operation"),
            tr("<h3>Pause/Resume</h3><br/>Pauses or resumes the operation"));

    // The main layout.
    auto layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_current_activity_label);
    layout->addWidget(m_text_status_label);
    layout->addWidget(m_progress_bar);
    layout->addWidget(m_pause_resume_button);
    layout->addWidget(m_cancel_button);
//    layout->addWidget(button_jobs);

    setLayout(layout);
}
