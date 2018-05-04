/*
 * BaseActivityProgressStatusBarWidget.cpp
 *
 *  Created on: May 4, 2018
 *      Author: gary
 */

#include "BaseActivityProgressStatusBarWidget.h"

#include <QLabel>
#include <QToolButton>
#include <QProgressBar>
#include <QHBoxLayout>

#include <KJob>

#include <gui/helpers/Tips.h>

BaseActivityProgressStatusBarWidget::BaseActivityProgressStatusBarWidget(QWidget *parent)
    : QFrame(parent)
{

}

BaseActivityProgressStatusBarWidget::~BaseActivityProgressStatusBarWidget()
{

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

    m_widget->setLayout(layout);
}
