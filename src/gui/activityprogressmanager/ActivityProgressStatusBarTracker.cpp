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

#include <config.h>

#include "ActivityProgressStatusBarTracker.h"

/// Qt5
#include <QTimer>
#include <QToolButton>


/// Ours
#include <utils/TheSimplestThings.h>
//#include <gui/helpers/Tips.h>
#include "BaseActivityProgressStatusBarWidget.h"
#include "CumulativeStatusWidget.h"

#include <KIO/ListJob>


ActivityProgressStatusBarTracker::ActivityProgressStatusBarTracker(QWidget *parent) : BASE_CLASS(parent),
    m_tsi_mutex(QMutex::Recursive /** @todo Shouldn't need to do this. */)
{
    m_parent_widget = parent;

    // Create the summary widget
    /// @todo nullptr -> AMLMJob?
    m_cumulative_status_widget = new CumulativeStatusWidget(nullptr, this, parent);

    m_expanding_frame_widget = new ExpandingFrameWidget();

    /// @note Set Window type to be a top-level window, i.e. a Qt::Window, Qt::Popup, or Qt::Dialog (and a few others mainly Mac).
//    m_expanding_frame_widget->setWindowFlags(Qt::Popup);
//    m_expanding_frame_widget->setParent(parent);
//    m_expanding_frame_widget->windowHandle()->setTransientParent(MainWindow::instance()->windowHandle());

    m_expanding_frame_widget->hide();

    connect(m_cumulative_status_widget, &CumulativeStatusWidget::show_hide_subjob_display,
            this, &ActivityProgressStatusBarTracker::toggleSubjobDisplay);

    /// @todo m_cumulative_status_widget's cancel_job button state should be enabled/disabled based on child job cancelable capabilities.

    // Connect the cumulative status widget button's signals to slots in this class, they need to apply to all sub-jobs.
    auto retval = connect(m_cumulative_status_widget, &CumulativeStatusWidget::cancel_job,
            this, &ActivityProgressStatusBarTracker::cancelAll);
    Q_ASSERT((bool)retval);
}

ActivityProgressStatusBarTracker::~ActivityProgressStatusBarTracker()
{
    // All KWidgetJobTracker does here is delete the private pImpl pointer.

    qDb() << "ActivityProgressStatusBarTracker DELETED";

    /// @todo NEW, IS THIS CORRECT?
    delete m_expanding_frame_widget;
    m_expanding_frame_widget = nullptr;
}

QWidget *ActivityProgressStatusBarTracker::widget(KJob *job)
{
    QMutexLocker locker(&m_tsi_mutex);

    // Shouldn't ever get here before the widget is constructed (in the constructor).
    if(job == nullptr)
    {
        // The summary widget.
        M_WARNIF((m_cumulative_status_widget == nullptr));
        return m_cumulative_status_widget;
    }
    else
    {
        // A specific KJob's widget.  Should have been created when the KJob was registered.
        auto kjob_widget = m_amlmjob_to_widget_map.value(job, nullptr);
        M_WARNIF((kjob_widget == nullptr));
        Q_CHECK_PTR(kjob_widget);
        return kjob_widget;
    }
}

void ActivityProgressStatusBarTracker::registerJob(KJob* kjob)
{
    // Adapted from KWidgetJobTracker's version of this function.
    QMutexLocker locker(&m_tsi_mutex);

    Q_CHECK_PTR(this);
    Q_ASSERT(kjob);

    // Create the widget for this new job.
    auto wdgt = new BaseActivityProgressStatusBarWidget(kjob, this, m_expanding_frame_widget);
    wdgt->m_is_job_registered = true;
    /// @todo Doesn't seem to matter crash-wise.
    wdgt->setAttribute(Qt::WA_DeleteOnClose);

    // Insert the kjob/widget pair into our master map.
    m_amlmjob_to_widget_map.insert(kjob, wdgt);

    /// @todo enqueue on a widgets-to-be-shown queue?  Not clear why that exists in KWidgetJobTracker.

    // Add the new widget to the expanging frame.
    m_expanding_frame_widget->addWidget(wdgt);
    m_expanding_frame_widget->reposition();

    // Make connections.
    /// @todo Should this really be here, or better in the onShowProgressWidget() call?
    make_connections_with_newly_registered_job(kjob, wdgt);

    /// EXP
    connect_destroyed_debug(kjob);

//    connect(job, &KJob::finished, this, [=](KJob *self){ qDb() << "TRACKER GOT FINISHED SIGNAL FROM JOB/SELF:" << job << self;});

    // KAbstractWidgetJobTracker::registerJob(KJob *job) simply calls:
    //   KJobTrackerInterface::registerJob(KJob *job) does nothing but connect
    //   many of the KJob signals to slots of this.  Specifically finsihed-related:
    //     QObject::connect(job, SIGNAL(finished(KJob*)), this, SLOT(unregisterJob(KJob*)));
    //     QObject::connect(job, SIGNAL(finished(KJob*)), this, SLOT(finished(KJob*)));
    BASE_CLASS::registerJob(kjob);

    // KWidgetJobTracker does almost the following.
    // It does not pass the job ptr though.
    /// @todo Is that part of our problems?
    QTimer::singleShot(500, this, [=](){onShowProgressWidget(kjob);});
}

void ActivityProgressStatusBarTracker::unregisterJob(KJob* kjob)
{
    // Adapted from KWidgetJobTracker's version of this function.
    QMutexLocker locker(&m_tsi_mutex);

    qDb() << "UNREGISTERING JOB:" << kjob;

    Q_CHECK_PTR(this);
    Q_ASSERT(kjob != nullptr);

    // KAbstractWidgetJobTracker::unregisterJob() calls:
    //   KJobTrackerInterface::unregisterJob(job);, which calls:
    //     job->disconnect(this);

    // Call down to the base class first; widget may be deleted by deref() below.
    BASE_CLASS::unregisterJob(kjob);

    /// @todo The only thing KWidgetJobTracker does differently here is remove any instances of "job" from the queue.
    with_widget_or_skip(kjob, [=](auto w){
        w->m_is_job_registered = false;
        w->deref();
        ;});
}

void ActivityProgressStatusBarTracker::SLOT_removeJobAndWidgetFromMap(KJob *ptr, QWidget *widget)
{
    QMutexLocker locker(&m_tsi_mutex);
    removeJobAndWidgetFromMap(ptr, widget);
}

void ActivityProgressStatusBarTracker::SLOT_directCallSlotStop(KJob *kjob)
{
    QMutexLocker locker(&m_tsi_mutex);
    directCallSlotStop(kjob);
}

void ActivityProgressStatusBarTracker::onShowProgressWidget(KJob* kjob)
{
    QMutexLocker locker(&m_tsi_mutex);

    // Called on a timer timeout after a new job is registered.

    Q_CHECK_PTR(kjob);

    /// @todo If queue is empty return.

    /// else dequeue job, look up qwidget, and show it.

    // Look up the widget associated with this kjob.
    // If it's been unregistered before we get here, this will return nullptr.
    with_widget_or_skip(kjob, [=](auto w){
        qDb() << "SHOWING WIDGET:" << w;
        /// @todo without activating?
        w->show();
    });
}

void ActivityProgressStatusBarTracker::cancelAll()
{
    QMutexLocker locker(&m_tsi_mutex);

    qDb() << "CANCELLING ALL JOBS";

    //  Get a list of all the keys in the map.
    QList<KJob*> joblist = m_amlmjob_to_widget_map.keys();

    qDb() << "CANCELLING ALL JOBS: Num KJobs:" << m_amlmjob_to_widget_map.size() << "List size:" << joblist.size();

    for(auto job : joblist)
    {
        qDb() << "Cancelling job:" << job; // << "widget:" << it.value();
        job->kill();
    }

    qDb() << "CANCELLING ALL JOBS: KJobs REMAINING:" << m_amlmjob_to_widget_map.size();
}

void ActivityProgressStatusBarTracker::description(KJob *job, const QString &title, const QPair<QString, QString> &field1, const QPair<QString, QString> &field2)
{
    if(qobject_cast<KIO::ListJob*>(job) != 0)
    {
        qDb() << "WIDGET:" << job;
    }

    /// This follows the basic pattern of KWidgetJobTracker, with a little C++14 help.
    /// All these "tracker->widget ~signal" handlers get the widget ptr (from a map in that case),
    /// check for null, and if not null do the job.
    with_widget_or_skip(job, [=](auto w){
        w->setDescription(title, field1, field2);
    });
}

void ActivityProgressStatusBarTracker::infoMessage(KJob *job, const QString &plain, const QString &rich)
{
    if(qobject_cast<KIO::ListJob*>(job) != 0)
    {
        qDb() << "WIDGET:" << job;
    }

    // Prefer rich if it's given.
    with_widget_or_skip(job, [=](auto w){
        w->setInfoMessage(rich.isEmpty() ? plain : rich);
        ;});
}

void ActivityProgressStatusBarTracker::warning(KJob *job, const QString &plain, const QString &rich)
{
    with_widget_or_skip(job, [=](auto w){
        w->setWarning(rich.isEmpty() ? plain : rich);
    });

}

void ActivityProgressStatusBarTracker::totalAmount(KJob *job, KJob::Unit unit, qulonglong amount)
{
    if(qobject_cast<KIO::ListJob*>(job) != 0)
    {
        qDb() << "WIDGET:" << job;
    }

    switch (unit)
    {
    case KJob::Bytes:
        m_is_total_size_known = true;
        // size is measured in bytes
        if (m_totalSize == amount)
        {
            return;
        }
        m_totalSize = amount;
        if (m_start_time.isNull())
        {
            m_start_time.start();
        }
        break;

//    case KJob::Files:
//        if (totalFiles == amount) {
//            return;
//        }
//        totalFiles = amount;
//        showTotals();
//        break;

//    case KJob::Directories:
//        if (totalDirs == amount) {
//            return;
//        }
//        totalDirs = amount;
//        showTotals();
//        break;
    }
}

void ActivityProgressStatusBarTracker::processedAmount(KJob *job, KJob::Unit unit, qulonglong amount)
{
    if(qobject_cast<KIO::ListJob*>(job) != 0)
    {
        qDb() << "WIDGET:" << job << unit << amount;
    }

    with_widget_or_skip(job, [=](auto w)
    {
        QString tmp;

        switch (unit) {
        case KJob::Bytes:
            if (m_processedSize == amount)
            {
                return;
            }
            m_processedSize = amount;

            /// @todo "TODO Allow user to specify QLocale::DataSizeIecFormat/DataSizeTraditionalFormat/DataSizeSIFormat");
            /// @link http://doc.qt.io/qt-5/qlocale.html#DataSizeFormat-enum
            DataSizeFormats fmt = DataSizeFormats::DataSizeTraditionalFormat;
            auto str_processed = formattedDataSize(m_processedSize, 1, fmt);

            if (m_is_total_size_known)
            {
                //~ singular %1 of %2 complete
                //~ plural %1 of %2 complete
                auto str_total = formattedDataSize(m_totalSize, 1, fmt);
                tmp = tr("%1 of %2 complete")
                      .arg(str_processed)
                      .arg(str_total);

                /// @todo GRVS
                w->setRange(0, m_totalSize);
                w->setValue(qBound(0ULL, m_processedSize, m_totalSize));
            }
            else
            {
                tmp = str_processed; //KJobTrackerFormatters::byteSize(amount);
            }
    //        sizeLabel->setText(tmp);
            if (!m_is_total_size_known)
            {
                // update jumping progressbar
                w->setRange(0, 0);
                w->setValue(m_processedSize);
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
    });
}

void ActivityProgressStatusBarTracker::percent(KJob *job, unsigned long percent)
{
    if(qobject_cast<KIO::ListJob*>(job) != 0)
    {
        qDb() << "WIDGET:" << job;
    }

    with_widget_or_skip(job, [=](auto w){
        qDb() << "KJobTrk: percent" << job << percent;

        QString title = toqstr("PCT") + " (";
        if (m_is_total_size_known)
        {
            /// @todo "TODO Allow user to specify QLocale::DataSizeIecFormat/DataSizeTraditionalFormat/DataSizeSIFormat");
            /// @link http://doc.qt.io/qt-5/qlocale.html#DataSizeFormat-enum
            DataSizeFormats fmt = DataSizeFormats::DataSizeTraditionalFormat;

            title += QString("%1% of %2").arg(percent).arg(formattedDataSize(m_totalSize, 1, fmt));

        }
//        else if (totalFiles)
//        {
//            //~ singular %1% of %n file
//            //~ plural %1% of %n files
//            title += QCoreApplication::translate("KWidgetJobTracker", "%1% of %n file(s)", "", totalFiles).arg(percent);
//        }
        else
        {
            title += QString("%1%").arg(percent);
        }

        title += ')';

        w->setRange(0, 100);
        w->setValue(percent);

    });
}

void ActivityProgressStatusBarTracker::speed(KJob *job, unsigned long value)
{
    with_widget_or_skip(job, [=](auto w){
        qDb() << "KJobTrk: speed" << job << value;
    });
}

void ActivityProgressStatusBarTracker::finished(KJob *job)
{
    //
    // KJobTrackerInterface::finished(KJob *job) does nothing.
    qWr() << "FINISHED KJob:" << job;
    with_widget_or_skip(job, [=](auto w){
        qWr() << "FINISHED JOB:" << job << "WITH WIDGET:" << w;
    });

//    Q_CHECK_PTR(this);
//    Q_CHECK_PTR(job);

//    qDb() << "KJobTrk: FINISHED KJob:" << job;
//    qDb() << "KJobTrk: FINISHED :" << M_NAME_VAL(job->capabilities())
//          << M_NAME_VAL(job->isSuspended())
//          << M_NAME_VAL(job->isAutoDelete())
//          << M_NAME_VAL(job->error())
//          << M_NAME_VAL(job->errorText())
//          << M_NAME_VAL(job->errorString());
}

void ActivityProgressStatusBarTracker::slotClean(KJob *job)
{
    with_widget_or_skip(job, [=](auto w){
        qDb() << "KJobTrk: slotClean" << job;
    });
}

void ActivityProgressStatusBarTracker::make_connections_with_newly_registered_job(KJob *kjob, QWidget *wdgt)
{
    // For Widgets to reques deletion of their jobs ext from the map.
    BaseActivityProgressStatusBarWidget* wdgt_type = qobject_cast<BaseActivityProgressStatusBarWidget*>(wdgt);
    connect(wdgt_type, &BaseActivityProgressStatusBarWidget::signal_removeJobAndWidgetFromMap,
            this, &ActivityProgressStatusBarTracker::SLOT_removeJobAndWidgetFromMap);
}

void ActivityProgressStatusBarTracker::removeJobAndWidgetFromMap(KJob* ptr, QWidget *widget)
{
    qDb() << "REMOVING FROM MAP:" << ptr << widget;
    if(m_amlmjob_to_widget_map[ptr] == widget)
    {
        m_amlmjob_to_widget_map.remove(ptr);
        /// @todo Also to-be-shown queue?
    }
}

void ActivityProgressStatusBarTracker::directCallSlotStop(KJob *kjob)
{
    slotStop(kjob);
}

void ActivityProgressStatusBarTracker::toggleSubjobDisplay(bool checked)
{
    if(checked)
    {
        showSubJobs();
    }
    else
    {
        hideSubJobs();
    }
}

void ActivityProgressStatusBarTracker::showSubJobs()
{
    // Get the parent-relative geometry of the "root widget".
    auto summary_widget = widget(nullptr);
    auto rect = summary_widget->frameGeometry();
    qDb() << "Root Frame Rect:" << rect << "parent:" << summary_widget->parentWidget();

//    m_expanding_frame_widget->windowHandle()->setTransientParent(RootWidget()->windowHandle());


    // Translate the the root widget's topLeft() to MainWindow coords.
    auto pos_tl_global = summary_widget->mapToGlobal(rect.topLeft());
    qDb() << "Root Frame topLeft(), Global:" << pos_tl_global;

//    m_expanding_frame_widget->popup(pos_tl_global);
    m_expanding_frame_widget->updateGeometry();
    m_expanding_frame_widget->raise();
    m_expanding_frame_widget->show();

#if 0
    m_expanding_frame_widget->raise();
    m_expanding_frame_widget->show();

//    m_expanding_frame_widget->updateGeometry();

    // Get the parent-relative geometry of the "root widget".
    auto rect = RootWidget()->frameGeometry();
    qDb() << "Root Frame Rect:" << rect << "parent:" << RootWidget()->parentWidget();

    // Translate the the root widget's topLeft() to MainWindow coords.
    auto pos_tl_global = RootWidget()->mapToGlobal(rect.topLeft());
    qDb() << "Root Frame topLeft(), Global:" << pos_tl_global;

    // Get the parent-relative pos of the expanding frame.
    auto frame_pos_pr = m_expanding_frame_widget->pos();
    qDb() << "Exp Frame topLeft():" << frame_pos_pr << "parent:" << m_expanding_frame_widget->parentWidget();

    // Global.
    auto frame_pos_global = m_expanding_frame_widget->mapToGlobal(frame_pos_pr);
    qDb() << "Exp Frame topLeft() Global:" << frame_pos_pr;

    auto frame_rect_pr = m_expanding_frame_widget->frameGeometry();
    qDb() << "Exp Frame frameGeometry():" << frame_rect_pr;

    // New width.
    auto new_exp_w = rect.width();

    // Move the bottomLeft() of the frame to the topLeft() of the root widget.
    frame_rect_pr.moveBottomLeft(m_expanding_frame_widget->parentWidget()->mapFromGlobal(pos_tl_global));
    Q_ASSERT(frame_rect_pr.isValid());
    m_expanding_frame_widget->setGeometry(frame_rect_pr);
    m_expanding_frame_widget->setMaximumWidth(new_exp_w);
    qDb() << "Max size:" << m_expanding_frame_widget->maximumSize();
#endif
}

void ActivityProgressStatusBarTracker::hideSubJobs()
{
    m_expanding_frame_widget->hide();
}



