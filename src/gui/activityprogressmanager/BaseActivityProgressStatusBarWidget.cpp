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

/// Qt5
#include <QLabel>
#include <QToolButton>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QMutexLocker>

//#include <KJob>
#include <KAbstractWidgetJobTracker>

/// Ours
#include <gui/helpers/Tips.h>
#include <utils/TheSimplestThings.h>
#include <utils/ConnectHelpers.h>
#include "concurrency/AMLMJob.h"
/// @todo Make these two unnecessary if possible.
#include "ActivityProgressStatusBarTracker.h"


BaseActivityProgressStatusBarWidget::BaseActivityProgressStatusBarWidget(QWidget *parent) : BASE_CLASS(parent)
{

}

BaseActivityProgressStatusBarWidget::BaseActivityProgressStatusBarWidget(KJob *job, ActivityProgressStatusBarTracker *tracker, QWidget *parent)
    : BaseActivityProgressStatusBarWidget(parent)
{
    m_tracker = tracker;
    m_kjob = job;
    m_refcount = 1;

    // We have a vtable to this, go nuts with the virtual calls.
    init(job, parent);

    /// Make the connections.
    /// @todo
//    make_connections();
}

BaseActivityProgressStatusBarWidget::~BaseActivityProgressStatusBarWidget()
{
    /// @todo KWidgetJobTracker::Private::ProgressWidget deletes "tracker->d->eventLoopLocker" in here.
    qDb() << "BaseActivityProgressStatusBarWidget DELETED";
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
    qDb() << "Setting INFOMESSAGE" << text;
    m_text_status_label->setText(text);
}

void BaseActivityProgressStatusBarWidget::setWarning(const QString &text)
{
M_WARNING("TODO");
qWr() << text;
}

void BaseActivityProgressStatusBarWidget::setTotalAmount(KJob *kjob, KJob::Unit unit, qulonglong amount)
{
//    qDb() << "GOT HERE";
    switch (unit)
    {
    case KJob::Bytes:
        m_is_total_size_known = true;
        // size is measured in bytes
        if (kjob->totalAmount(unit) == amount)
        {
            return;
        }
        /// @todo Already handled by tracker?: w->m_totalSize = amount;
//        if (w->m_start_time.isNull())
//        {
//            w->m_start_time.start();
//        }
        m_progress_bar->setRange(0, kjob->totalAmount(unit));
        break;
    case KJob::Files:
        if (kjob->totalAmount(unit) == amount)
        {
            return;
        }
//        totalFiles = amount;
        /// @todo ???
//        showTotals();
        break;

    case KJob::Directories:
        if (kjob->totalAmount(unit) == amount)
        {
            return;
        }
//        totalDirs = amount;
//        showTotals();
        break;
    }
}

void BaseActivityProgressStatusBarWidget::setProcessedAmount(KJob *kjob, KJob::Unit unit, qulonglong amount)
{
    auto prev_total_amount = kjob->totalAmount(unit);

    QString tmp;

    switch (unit)
    {
        case KJob::Bytes:
            if (kjob->processedAmount(unit) == amount)
            {
                return;
            }

            /// @todo "TODO Allow user to specify QLocale::DataSizeIecFormat/DataSizeTraditionalFormat/DataSizeSIFormat");
            /// @link http://doc.qt.io/qt-5/qlocale.html#DataSizeFormat-enum
            DataSizeFormats fmt = DataSizeFormats::DataSizeTraditionalFormat;
            auto str_processed = formattedDataSize(kjob->processedAmount(unit), 1, fmt);

            if (m_is_total_size_known)
            {
                //~ singular %1 of %2 complete
                //~ plural %1 of %2 complete
                auto str_total = formattedDataSize(kjob->processedAmount(unit), 1, fmt);
                tmp = tr("%1 of %2 complete")
                      .arg(str_processed)
                      .arg(str_total);

                /// @todo GRVS
                if(prev_total_amount < amount)
                setRange(0, amount);
                setValue(qBound(0ULL, amount, prev_total_amount));
            }
            else
            {
                tmp = str_processed; //KJobTrackerFormatters::byteSize(amount);
            }
    //        sizeLabel->setText(tmp);
            if (!m_is_total_size_known)
            {
                // update jumping progressbar
                setRange(0, 0);
                setValue(amount);
            }
            break;

    //    case KJob::Directories:
    //        if (processedDirs == amount) {
    //            return;
    //        }
    //        processedDirs = amount;

    //        //~ singular %1 / %n folder
    //        //~ plural %1 / %n folders
    //        tmp = QCoreApplication::translate("KWidgetJobTracker", "%1 / %n folder(s)", "", totalDirs).arg(processedDirs);
    //        tmp += QLatin1String("   ");
    //        //~ singular %1 / %n file
    //        //~ plural %1 / %n files
    //        tmp += QCoreApplication::translate("KWidgetJobTracker", "%1 / %n file(s)", "", totalFiles).arg(processedFiles);
    //        progressLabel->setText(tmp);
    //        break;

    //    case KJob::Files:
    //        if (processedFiles == amount) {
    //            return;
    //        }
    //        processedFiles = amount;

    //        if (totalDirs > 1) {
    //            //~ singular %1 / %n folder
    //            //~ plural %1 / %n folders
    //            tmp = QCoreApplication::translate("KWidgetJobTracker", "%1 / %n folder(s)", "", totalDirs).arg(processedDirs);
    //            tmp += QLatin1String("   ");
    //        }
    //        //~ singular %1 / %n file
    //        //~ plural %1 / %n files
    //        tmp += QCoreApplication::translate("KWidgetJobTracker", "%1 / %n file(s)", "", totalFiles).arg(processedFiles);
    //        progressLabel->setText(tmp);
    }
}

void BaseActivityProgressStatusBarWidget::setRange(int min, int max)
{
    m_progress_bar->setRange(min, max);
}

void BaseActivityProgressStatusBarWidget::setValue(int val)
{
    m_progress_bar->setValue(val);
}

void BaseActivityProgressStatusBarWidget::setPercent(unsigned long pct)
{
    m_progress_bar->setRange(0,100);
    m_progress_bar->setValue(pct);
}

void BaseActivityProgressStatusBarWidget::init(KJob* job, QWidget *parent)
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

        // Emit the cancel_job(KJob*) signal when the cancel button is clicked.
        /// @todo KWidgetJobTracker::Private::ProgressWidget only does click->stop signal here.
        /// Seems odd, should go back to the tracker to do the job stop etc.
//        connect(m_cancel_button, &QToolButton::clicked, this, [=]() {
//            qDb() << "CANCEL BUTTON CLICKED, JOB:" << m_kjob;
//            Q_EMIT cancel_job(m_kjob);
//        });
        connect_or_die(m_cancel_button, &QToolButton::clicked, this, &BaseActivityProgressStatusBarWidget::stop);

#if 0 // CRASHING
        // Connect up the disconnect signal from the job.
M_WARNING("CRASH: This is now crashing if you let the jobs complete.");
        connect(job, &AMLMJob::finished, this, [=](KJob* finished_job){
            Q_CHECK_PTR(this);
            Q_CHECK_PTR(this->parent());
            Q_CHECK_PTR(finished_job);
            Q_CHECK_PTR(job);
            Q_CHECK_PTR(m_cancel_button);

//            qDb() << "GOT FINISHED SIGNAL, DISCONNECTING FROM JOB:" << job;
            disconnect(job, nullptr, m_cancel_button, nullptr);
            disconnect(job, nullptr, this, nullptr);
            deleteLater();
            ;});
#endif
    }
    else
    {
        // null Job (i.e. it's the root tracker/widget).
        qDb() << "INIT() CALL FOR ROOT TRACKER WIDGET, JOB IS NULL";
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

void BaseActivityProgressStatusBarWidget::make_connections()
{
#if 0
    if(m_kjob && m_tracker)
    {
//        // Connect cancel-clicked signal to tracker's remove-job signal.
//        connect(m_cancel_button, &QToolButton::clicked, this, [=](){ Q_EMIT cancel_job(m_kjob);});
    }
    else
    {
        qWr() << "NO JOB/TRACKER:" << m_kjob << m_tracker;
        Q_ASSERT(0);
    }
#endif
}

void BaseActivityProgressStatusBarWidget::closeEvent(QCloseEvent *event)
{
    if(m_is_job_registered && m_tracker->stopOnClose(m_kjob))
    {
        qDb() << "EMITTING SLOTSTOP";
        QMetaObject::invokeMethod(m_tracker, "slotStop", Qt::AutoConnection,
                                  Q_ARG(KJob*, m_kjob));
//        m_tracker->directCallSlotStop(m_job);
    }

    BASE_CLASS::closeEvent(event);
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

    /// @todo Haven't analyzed the following scenario, but I think this covers it:
    /// // It might happen the next scenario:
    /// - Start a job which opens a progress widget. Keep it open. Address job is 0xdeadbeef
    /// - Start a new job, which is given address 0xdeadbeef. A new window is opened.
    ///   This one will take much longer to complete. The key 0xdeadbeef on the widget map now
    ///   stores the new widget address.
    /// - Close the first progress widget that was opened (and has already finished) while the
    ///   last one is still running. We remove its reference on the map. Wrong.
    /// For that reason we have to check if the map stores the widget as the current one.
    /// ereslibre

    Q_CHECK_PTR(m_tracker);
    if(m_tracker)
    {
//        m_tracker->removeJobAndWidgetFromMap(m_job, this);
        qDb() << "EMITTING signal_removeJobAndWidgetFromMap:" << m_kjob << this;
        Q_EMIT signal_removeJobAndWidgetFromMap(m_kjob, this);
    }

//    if (m_tracker->d->progressWidget[m_job] == this)
//    {
//        m_tracker->d->progressWidget.remove(m_job);
//        m_tracker->d->progressWidgetsToBeShown.removeAll(m_job);
//    }
}

void BaseActivityProgressStatusBarWidget::stop()
{
    /// KWidgetJobTracker::Private::ProgressWidget::_k_stop() does this here:
    /// if (jobRegistered) {
    ///    tracker->slotStop(job);
    /// }
    /// closeNow();
    ///
    /// ATM we're missing something, because when we do what should be ~equivalent below, the "cancel" button
    /// is ignored.

   if(m_is_job_registered)
   {
       // Notify tracker that the job has been killed.
       // Calls job->kill(KJob::EmitResults) then emits stopped(job).
       auto retval = QMetaObject::invokeMethod(m_tracker, "slotStop", Qt::AutoConnection,
                                 Q_ARG(KJob*, m_kjob));
       Q_ASSERT(retval);

//       m_tracker->directCallSlotStop(m_job);


       // Emit the TW:Job-has-been-cancelled signal.
       /// @todo I think this was our main problem with the crashing on completion:
       /// cancel_job() should be from the cancel button only, not as a notification such as this.
//       qDb() << "EMITTING CANCEL_JOB";
//       Q_EMIT cancel_job(this->m_job);
   }
   closeNow();
}

void BaseActivityProgressStatusBarWidget::pause_resume(bool)
{

}
