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
#include <QToolTip>
#include <QHelpEvent>

/// KF5
//#include <KJob>
#include <KAbstractWidgetJobTracker>
#include <KToolTipWidget>
#include <QPalette>

/// Ours
#include <gui/helpers/Tips.h>
#include <utils/TheSimplestThings.h>
#include <utils/ConnectHelpers.h>
#include "concurrency/AMLMJob.h"
/// @todo Make these two unnecessary if possible.
#include "ActivityProgressStatusBarTracker.h"
#include <gui/MainWindow.h>


BaseActivityProgressStatusBarWidget::BaseActivityProgressStatusBarWidget(QWidget *parent) : BASE_CLASS(parent)
{

}

BaseActivityProgressStatusBarWidget::BaseActivityProgressStatusBarWidget(KJob *job, ActivityProgressStatusBarTracker *tracker, QWidget *parent)
    : BaseActivityProgressStatusBarWidget(parent)
{
    m_tracker = tracker;
    m_kjob = job;
    m_refcount = 1;

    // We have a vtable to this.
    this->init(job, parent);
}

BaseActivityProgressStatusBarWidget::~BaseActivityProgressStatusBarWidget()
{
    /// @todo KWidgetJobTracker::Private::ProgressWidget deletes "tracker->d->eventLoopLocker" in here.
    qDb() << "BaseActivityProgressStatusBarWidget DESTRUCTOR:" << this;
}

void BaseActivityProgressStatusBarWidget::addButton(QToolButton *new_button)
{
    // Reparents the button.
    layout()->addWidget(new_button);
}

void BaseActivityProgressStatusBarWidget::description(KJob *kjob, const QString &title, const QPair<QString, QString> &field1, const QPair<QString, QString> &field2)
{
    qDb() << "GOT DESCRIPTION FROM KJOB:" << kjob << "TITLE:" << title;
    /// @todo Don't really have anywhere to put fields.
    Q_UNUSED(field1);
    Q_UNUSED(field2);
    m_current_activity_label->setText(title);
}

void BaseActivityProgressStatusBarWidget::infoMessage(KJob* kjob, const QString &text)
{
    m_text_status_label->setText(text);
}

void BaseActivityProgressStatusBarWidget::warning(KJob *kjob, const QString &text)
{
M_WARNING("TODO");
qWr() << "WARNING:" << text;
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
//        connect_or_die(m_cancel_button, &QToolButton::clicked, this, &BaseActivityProgressStatusBarWidget::stop);

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
        Q_ASSERT(0);
        // null Job (i.e. it's the root tracker/widget).
        qDb() << "INIT() CALL FOR ROOT TRACKER WIDGET, JOB IS NULL";
        m_pause_resume_button->setEnabled(false);
        m_cancel_button->setEnabled(false);
    }

    // The tooltip widget, and the widget within the widget.
    m_tool_tip_widget = new KToolTipWidget(this);
    m_tool_tip_label = new QLabel(m_tool_tip_widget);
    m_tool_tip_label->setBackgroundRole(QPalette::ToolTipBase);
    m_tool_tip_label->setForegroundRole(QPalette::ToolTipText);

    // The main layout.
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_current_activity_label);
    layout->addWidget(m_text_status_label);
    layout->addWidget(m_progress_bar);
    layout->addWidget(m_pause_resume_button);
    layout->addWidget(m_cancel_button);

    setLayout(layout);

    updateMainTooltip();
}

void BaseActivityProgressStatusBarWidget::make_connections()
{
    qDb() << "BASE MAKE_CONNECTIONS";
    if(true /* not summary widget */)
    {
        // Directly connect the cancel button to this class' stop() slot, which stops the job.
        /// @note If we have the summary job fully working, this same connection would be fine;
        /// the job would respond to the stop() by stopping all child jobs.
        /// But we don't, so this function is overridden in the CumulativeStatusWidget class.
        connect_or_die(m_cancel_button, &QToolButton::clicked, this, &BaseActivityProgressStatusBarWidget::stop);
        // Description.
        connect_or_die(m_kjob, &KJob::description, this, &BaseActivityProgressStatusBarWidget::description);
    }

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

void BaseActivityProgressStatusBarWidget::showTotals()
{
    // Show the totals in the progress label, if we still haven't
    // processed anything. This is useful when the stat'ing phase
    // of CopyJob takes a long time (e.g. over networks).

    auto processedFiles = m_kjob->processedAmount(KJob::Unit::Files);
    auto processedDirs = m_kjob->processedAmount(KJob::Unit::Directories);
    auto totalFiles = m_kjob->processedAmount(KJob::Unit::Files);
    auto totalDirs = m_kjob->totalAmount(KJob::Unit::Directories);

    if (processedFiles == 0 && processedDirs == 0)
    {
        // No files or dirs processed yet.
        QString tmps;
        if (totalDirs > 1)
        {
            // We know we have at least one directory to process.

            //~ singular %n folder
            //~ plural %n folders
            tmps = QCoreApplication::translate("KWidgetJobTracker", "%n folder(s)", "", totalDirs) + "   ";
        }
        //~ singular %n file
        //~ plural %n files
        tmps += QCoreApplication::translate("KWidgetJobTracker", "%n file(s)", "", totalFiles);

        // Set the resulting string.
        m_text_status_label->setText(tmps);
    }
}

void BaseActivityProgressStatusBarWidget::updateMainTooltip()
{
    QString tooltip_text;

//    setToolTip(tooltip);
    QPoint pos;
//    QToolTip::showText(pos, QStringLiteral(""), this, QRect());

//    if(!QToolTip::isVisible())
//    {
//        return;
//    }

    tooltip_text = tr("Async Job: %1<br/>").arg(m_current_activity_label->text())
            + tr("Current Status: %1<br/>").arg(m_text_status_label->text())
            + tr("Speed: %1<br/>").arg("N/A")
               ;

//    m_tool_tip_label->setText(tooltip_text);
    m_tool_tip_label->setText(tooltip_text);

//    QToolTip::showText(pos, tooltip, this, QRect());
    //    setToolTip(tooltip);
}

bool BaseActivityProgressStatusBarWidget::event(QEvent *event)
{
    if(event->type() == QEvent::ToolTip)
    {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);

        bool in_this_rect = this->rect().contains(helpEvent->pos());
        if(in_this_rect)
        {
//            QToolTip::showText(helpEvent->globalPos(), m_tool_tip_label->text());
            m_tool_tip_widget->showAt(helpEvent->globalPos(), m_tool_tip_label, MainWindow::instance()->windowHandle());
        }
        else
        {
//            QToolTip::hideText();
            m_tool_tip_widget->hide();
            event->ignore();
        }

        return true;
    }
    return QWidget::event(event);
}

void BaseActivityProgressStatusBarWidget::closeEvent(QCloseEvent *event)
{
    if(m_is_job_registered && m_tracker->stopOnClose(m_kjob))
    {
        qDb() << "EMITTING SLOTSTOP";
        QMetaObject::invokeMethod(m_tracker, "slotStop", Qt::AutoConnection,
                                  Q_ARG(KJob*, m_kjob));
//        qDb() << "CALLING SLOTSTOP";
//        m_tracker->directCallSlotStop(m_kjob);
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

    /// @todo Haven't fully analyzed the following scenario, but I think what we have below covers it:
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

//       m_tracker->directCallSlotStop(m_kjob);
   }
   closeNow();
}

void BaseActivityProgressStatusBarWidget::pause_resume(bool)
{
    Q_ASSERT(0);
}

void BaseActivityProgressStatusBarWidget::totalAmount(KJob *kjob, KJob::Unit unit, qulonglong amount)
{
    Q_CHECK_PTR(kjob);

    /// @todo These are taking the place of KWidgetJobTracker::Private::ProgressWidget's member vars of the same names,
    /// which are kept up to date by the logic in these functions.  Unclear if this is correct, better, or worse.
    auto processedFiles = kjob->processedAmount(KJob::Unit::Files);
    auto totalFiles = kjob->totalAmount(KJob::Unit::Files);
    auto processedDirs = kjob->processedAmount(KJob::Unit::Directories);
    auto totalDirs = kjob->totalAmount(KJob::Unit::Directories);

    /// @todo Is this really what we should be using instead of m_total_size?
    /// And/or totalFiles and totalDirs?
    auto kjob_total_amount_in_current_units = kjob->totalAmount(unit);

    switch (unit)
    {
    case KJob::Bytes:
        m_is_total_size_known = true;
        // Size is measured in bytes
        /// @todo Bytes/size is already handled by the tracker.
        /// Don't quite know what to make of the time stuff here.
        if (m_start_time.isNull())
        {
            m_start_time.start();
        }
        break;
    case KJob::Files:
        // Shouldn't be getting signalled unless totalFiles() has actually changed.
        showTotals();
        break;

    case KJob::Directories:
        // Shouldn't be getting signalled unless total dirs has actually changed.
        showTotals();
        break;
    }

    updateMainTooltip();
}

void BaseActivityProgressStatusBarWidget::processedAmount(KJob *kjob, KJob::Unit unit, qulonglong amount)
{
    Q_CHECK_PTR(kjob);

    /// @todo These are taking the place of KWidgetJobTracker::Private::ProgressWidget's member vars of the same names,
    /// which are kept up to date by the logic in these functions.  Unclear if this is correct, better, or worse.
    auto processedFiles = kjob->processedAmount(KJob::Unit::Files);
    auto totalFiles = kjob->totalAmount(KJob::Unit::Files);
    auto processedDirs = kjob->processedAmount(KJob::Unit::Directories);
    auto totalDirs = kjob->totalAmount(KJob::Unit::Directories);
    auto processedBytes = kjob->processedAmount(KJob::Unit::Bytes);
    auto totalBytes = kjob->totalAmount(KJob::Unit::Bytes);

    /// KWidgetJobTracker uses totalSize as the member var for the total number of bytes
    /// which are to be processed.  Set it up here as an alias, const because we shouldn't need
    /// to ever update it (especially in this function).
    const auto totalSize = totalBytes;

    /// @todo KWidgetJobTracker uses two labels for size (bytes here) and progress (files and dirs).
    /// We'll set up aliases here, but for now we only have one label for both.
    QLabel* sizeLabel = m_text_status_label;
    QLabel* progressLabel = m_text_status_label;

    QString size_label_text;

    switch (unit)
    {
        case KJob::Bytes:
        {
            // Create the "current processedAmount" string.  We need it below regardless of whether
            // we know the total amount or not.
            /// @todo "TODO Allow user to specify QLocale::DataSizeIecFormat/DataSizeTraditionalFormat/DataSizeSIFormat");
            /// @link http://doc.qt.io/qt-5/qlocale.html#DataSizeFormat-enum
            DataSizeFormats fmt = DataSizeFormats::DataSizeTraditionalFormat;
            auto str_processed_bytes = formattedDataSize(amount, 1, fmt);

            if (m_is_total_size_known)
            {
                // We know the total size, so we can make a "n of n_total complete"-type string.

                //~ singular %1 of %2 complete
                //~ plural %1 of %2 complete
                // Create the "%2" (total amount) string.
                auto str_total_bytes = formattedDataSize(totalSize, 1, fmt);
                size_label_text = tr("%1 of %2 complete")
                      .arg(str_processed_bytes)
                      .arg(str_total_bytes);
            }
            else
            {
                // We don't have a total size, just use the new amount string.
                size_label_text = str_processed_bytes;
            }

            // Set the size text we just put together.
            sizeLabel->setText(size_label_text);

            if (!m_is_total_size_known)
            {
                // Don't know the total size, so set the progress bar's value to keep it animated.
                m_progress_bar->setValue(amount);
            }
            break;
        }
        case KJob::Directories:
        {
            //~ singular %1 / %2 folder
            //~ plural %1 / %2 folders
            size_label_text = tr("%1 / %2 folder(s)").arg(processedDirs).arg(totalDirs);
            size_label_text += QLatin1String("   ");
            //~ singular %1 / %2 file
            //~ plural %1 / %2 files
            size_label_text += tr("%1 / %2 file(s)").arg(processedFiles).arg(totalFiles);
            progressLabel->setText(size_label_text);
            break;
        }
        case KJob::Files:
        {
            if (totalDirs > 1)
            {
                //~ singular %1 / %2 folder
                //~ plural %1 / %2 folders
                size_label_text = tr("%1 / %2 folder(s)").arg(processedDirs).arg(totalDirs);
                size_label_text += QLatin1String("   ");
            }
            //~ singular %1 / %2 file
            //~ plural %1 / %2 files
            size_label_text += tr("%1 / %2 file(s)").arg(processedFiles).arg(totalFiles);
            progressLabel->setText(size_label_text);
            break;
        }
    }

    updateMainTooltip();
}

void BaseActivityProgressStatusBarWidget::totalSize(KJob *kjob, qulonglong amount)
{
    m_totalSize = amount;
    updateMainTooltip();
}

void BaseActivityProgressStatusBarWidget::processedSize(KJob *kjob, qulonglong amount)
{
    m_processedSize = amount;
    updateMainTooltip();
}

void BaseActivityProgressStatusBarWidget::percent(KJob *kjob, unsigned long percent)
{
    Q_CHECK_PTR(kjob);

    if(kjob == nullptr)
    {
        qWr() << "PERCENT GOT NULL KJOB, BALKING";
        return;
    }

    qulonglong totalSize;
    auto amlm_ptr = qobject_cast<AMLMJob*>(kjob);
    if(amlm_ptr != nullptr)
    {
        qWr() << "KJob not an AMLMJob, size is bytes:" << kjob;
        totalSize = amlm_ptr->totalSize();
    }
    else
    {
        totalSize = kjob->totalAmount(KJob::Unit::Bytes);
    }
    auto totalFiles = kjob->totalAmount(KJob::Unit::Files);

    QString title = toqstr("PCT") + " (";
    if (m_is_total_size_known)
    {
        /// @todo "TODO Allow user to specify QLocale::DataSizeIecFormat/DataSizeTraditionalFormat/DataSizeSIFormat");
        /// @link http://doc.qt.io/qt-5/qlocale.html#DataSizeFormat-enum
        DataSizeFormats fmt = DataSizeFormats::DataSizeTraditionalFormat;

        /// @note Regardless of what you might want, this text is always going to be ~"x% of NN.N GB" (i.e. units == bytes).
        title += tr("%1% of %2").arg(percent).arg(formattedDataSize(totalSize, 1, fmt));

    }
    else if (totalFiles)
    {
        //~ singular %1% of %2 file
        //~ plural %1% of %2 files
        title += tr("%1% of %2 file(s)").arg(percent).arg(totalFiles);
    }
    else
    {
        // Just percent.
        title += tr("%1%").arg(percent);
    }

    title += ')';

//    qDb() << "PCT:" << kjob << percent;

    // Set the progress bar values.
    m_progress_bar->setRange(0, 100);
    m_progress_bar->setValue(percent);

    // Do we need/care about this?
    setWindowTitle(title);

    updateMainTooltip();
}

void BaseActivityProgressStatusBarWidget::speed(KJob *kjob, unsigned long value)
{
    Q_CHECK_PTR(kjob);

    qDb() << "SPEED:" << kjob << value;

#if 1
    if(value == 0)
	{
    	// Stalled.
    	//m_speed_label->setText(tr("Stalled"));
	}
    else
	{
        qulonglong totalSize;
        qulonglong processedSize;
        auto amlm_ptr = qobject_cast<AMLMJob*>(kjob);
        if(amlm_ptr != nullptr)
        {
            qWr() << "KJob not an AMLMJob, size is bytes:" << kjob;
            totalSize = amlm_ptr->totalSize();
            processedSize = amlm_ptr->processedSize();
        }
        else
        {
            totalSize = kjob->totalAmount(KJob::Unit::Bytes);
            processedSize = kjob->processedAmount(KJob::Unit::Bytes);
        }

        /// @todo "TODO Allow user to specify QLocale::DataSizeIecFormat/DataSizeTraditionalFormat/DataSizeSIFormat");
        /// @link http://doc.qt.io/qt-5/qlocale.html#DataSizeFormat-enum
        DataSizeFormats fmt = DataSizeFormats::DataSizeTraditionalFormat;

        const QString speedStr = formattedDataSize(value, 1, fmt);
        if (m_is_total_size_known)
		{
			const int remaining = 1000 * (totalSize - processedSize) / value;
			//~ singular %1/s (%2 remaining)
			//~ plural %1/s (%2 remaining)
//			speedLabel->setText(QCoreApplication::translate("KWidgetJobTracker", "%1/s (%2 remaining)", "", remaining).arg(speedStr).arg(
//									KJobTrackerFormatters::duration(remaining)));
		}
		else
		{
			// total size is not known
//			speedLabel->setText(QCoreApplication::translate("KWidgetJobTracker", "%1/s", "speed in bytes per second").arg(speedStr));
		}
	}
#endif
    updateMainTooltip();
}
