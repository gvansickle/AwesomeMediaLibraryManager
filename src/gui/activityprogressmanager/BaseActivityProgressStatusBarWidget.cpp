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

//#include <KJob>
#include <KAbstractWidgetJobTracker>

#include <gui/helpers/Tips.h>
#include <utils/DebugHelpers.h>
#include <utils/ConnectHelpers.h>

#include "concurrency/AMLMJob.h"

BaseActivityProgressStatusBarWidget::BaseActivityProgressStatusBarWidget(QWidget *parent) : BASE_CLASS(parent)
{

}

BaseActivityProgressStatusBarWidget::BaseActivityProgressStatusBarWidget(AMLMJobPtr job, KAbstractWidgetJobTracker* tracker, QWidget *parent)
    : BaseActivityProgressStatusBarWidget(parent)
{
    m_tracker = tracker;
    m_job = job;

    // We have a vtable to this, go nuts with the virtual calls.
    /// @note job is currently unused by init().
    init(job, parent);
}

BaseActivityProgressStatusBarWidget::~BaseActivityProgressStatusBarWidget()
{

}

void BaseActivityProgressStatusBarWidget::addButton(QToolButton *new_button)
{
    // Reparents the button.
    layout()->addWidget(new_button);
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

void BaseActivityProgressStatusBarWidget::init(AMLMJobPtr job, QWidget *parent)
{
    // Create the widget.
    /// @link https://github.com/KDE/kjobwidgets/blob/master/src/kstatusbarjobtracker.cpp

    m_current_activity_label = new QLabel(tr("Idle"), this);
    m_current_activity_label->setToolTip("Current operation");
    m_current_activity_label->setWhatsThis("This text shows the current operation in progress.");

    // This is for displaying the KJob::infoMessage().
    // ""Resolving host", "Connecting to host...", etc."
    m_text_status_label = new QLabel(tr("Idle"), this);
    m_text_status_label->setToolTip("Status of the current operation");
    m_text_status_label->setWhatsThis("This text shows the status of the current operation in progress.");

    // The progress bar.
    m_progress_bar = new QProgressBar(this);
    m_progress_bar->setFormat(tr("%p%"));
    m_progress_bar->setTextVisible(true);
    m_progress_bar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // The buttons.
    m_pause_resume_button = new QToolButton(this);
    m_pause_resume_button->setIcon(QIcon::fromTheme("media-playback-pause"));
    setTips(m_pause_resume_button, tr("Pause/Resume"), tr("Pause or resume this operation"),
            tr("<h3>Pause/Resume</h3><br/>Pauses or resumes the operation"));
    m_cancel_button = new QToolButton(this);
    m_cancel_button->setIcon(QIcon::fromTheme("process-stop"));
    setTips(m_cancel_button, tr("Abort"), tr("Abort this operation"), tr("<h3>Abort Button</h3><br/>Stops the operation"));

    // Set button disable states/make connections/etc. based on what the job supports.
    if(job)
    {
        M_WARNING("TODO: The if() is FOR THE MAIN BAR WHICH IS CURRENTLY JOBLESS");
        m_pause_resume_button->setEnabled(job->capabilities() & KJob::Suspendable);
        m_cancel_button->setEnabled(job->capabilities() & KJob::Killable);
        connect(m_cancel_button, &QToolButton::clicked, this, [=](){
            qDb() << "EMITTING CANCEL_JOB";
            Q_EMIT cancel_job(job);});
        connect(m_cancel_button, &QToolButton::clicked, this, &BaseActivityProgressStatusBarWidget::stop);

        // Connect up the disconnect signal from the job.
        connect(job, &AMLMJob::finished, this, [=](){
            qDb() << "GOT FINISHED SIGNAL, DISCONNECTING FROM JOB:" << job;
            disconnect(job, nullptr, m_cancel_button, nullptr);
            disconnect(job, nullptr, this, nullptr);
            deleteLater();
            ;});
    }
    else
    {
        m_pause_resume_button->setEnabled(false);
        m_cancel_button->setEnabled(false);
    }

    // The main layout.
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_current_activity_label);
    layout->addWidget(m_text_status_label);
    layout->addWidget(m_progress_bar);
    layout->addWidget(m_pause_resume_button);
    layout->addWidget(m_cancel_button);

    setLayout(layout);
}

void BaseActivityProgressStatusBarWidget::closeEvent(QCloseEvent *event)
{
    if(m_is_job_registered && m_tracker->stopOnClose(m_job))
    {
        QMetaObject::invokeMethod(m_tracker, "slotStop", Qt::DirectConnection,
                                  Q_ARG(KJob*, m_job));
    }

    QWidget::closeEvent(event);
}

void BaseActivityProgressStatusBarWidget::ref()
{
    m_refcount++;
}

void BaseActivityProgressStatusBarWidget::deref()
{
    if(m_refcount)
    {
        m_refcount--;
    }

    if(!m_refcount)
    {
        if(true/*!keep open*/)
        {
            closeNow();
        }
        else
        {
//            slotClean();
        }
    }
}

void BaseActivityProgressStatusBarWidget::closeNow()
{
    close();

    /// @todo Haven't analyzed the following scenario:
    /// // It might happen the next scenario:
    /// - Start a job which opens a progress widget. Keep it open. Address job is 0xdeadbeef
    /// - Start a new job, which is given address 0xdeadbeef. A new window is opened.
    ///   This one will take much longer to complete. The key 0xdeadbeef on the widget map now
    ///   stores the new widget address.
    /// - Close the first progress widget that was opened (and has already finished) while the
    ///   last one is still running. We remove its reference on the map. Wrong.
    /// For that reason we have to check if the map stores the widget as the current one.
    /// ereslibre

//    if (m_tracker->d->progressWidget[m_job] == this)
//    {
//        m_tracker->d->progressWidget.remove(m_job);
//        m_tracker->d->progressWidgetsToBeShown.removeAll(m_job);
//    }
}

void BaseActivityProgressStatusBarWidget::stop()
{
   if(m_is_job_registered)
   {
       // Notify that the job has been killed.
       // Calls job->kill(KJob::EmitResults) then emits stopped(job).
       QMetaObject::invokeMethod(m_tracker, "slotStop", Qt::DirectConnection,
                                 Q_ARG(KJob*, m_job));
   }
   closeNow();
}

void BaseActivityProgressStatusBarWidget::pause_resume(bool)
{

}
