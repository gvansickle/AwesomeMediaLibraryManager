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
#include <QObject>
#include <QTimer>
#include <QToolButton>

/// KF5
#include <KJob>
#include <KIO/ListJob>

/// Ours
#include <utils/TheSimplestThings.h>
//#include <gui/helpers/Tips.h>
#include "BaseActivityProgressStatusBarWidget.h"
#include "CumulativeStatusWidget.h"

ActivityProgressStatusBarTracker::ActivityProgressStatusBarTracker(QWidget *parent) : BASE_CLASS(parent),
    m_tsi_mutex(QMutex::Recursive /** @todo Shouldn't need to do recursive, but it does make things easier at the moment. */)
{
    // Save the parent widget.
    m_parent_widget = parent;

    // Create the Cumulative tracker.
//    m_cumulative_status_tracker = new ActivityProgressStatusBarTracker(parent);

    // Create the job which will contain all other jobs.
    /// @note At least that's the theory, eventually.
    m_cumulative_status_job = new CumulativeAMLMJob(parent);

    // Create the summary widget
    /// @todo Should this have its own separate tracker?
    m_cumulative_status_widget = new CumulativeStatusWidget(m_cumulative_status_job, this, parent);

    // Make cumulative status widget connections.
    m_cumulative_status_widget->make_connections();

    /// Register the cumulative job and widget.
    /// @todo Move?
//M_WARNING("TODO: Need to not delete this job/wdgt pair ever (e.g. on cancel)");
//    m_cumulative_status_tracker->registerJob(cumulative_job, qobject_cast<BaseActivityProgressStatusBarWidget*>(m_cumulative_status_widget));

    m_expanding_frame_widget = new ExpandingFrameWidget(m_cumulative_status_widget, Qt::Tool);
//    m_expanding_frame_widget->setMainProgressWidget(m_cumulative_status_widget);

    /// @note Set Window type to be a top-level window, i.e. a Qt::Window, Qt::Popup, or Qt::Dialog (and a few others mainly Mac).
//    m_expanding_frame_widget->setWindowFlags(Qt::Popup);
//    m_expanding_frame_widget->setParent(parent);
//    m_expanding_frame_widget->windowHandle()->setTransientParent(MainWindow::instance()->windowHandle());

    m_expanding_frame_widget->hide();

    connect_or_die(m_cumulative_status_widget, &CumulativeStatusWidget::show_hide_subjob_display,
            this, &ActivityProgressStatusBarTracker::toggleSubjobDisplay);

    /// @todo m_cumulative_status_widget's cancel_job button state should be enabled/disabled based on child job cancelable capabilities.

    // Connect the cumulative status widget button's signals to slots in this class, they need to apply to all sub-jobs.
    connect_or_die(m_cumulative_status_widget, &CumulativeStatusWidget::cancel_job,
                this, &ActivityProgressStatusBarTracker::cancelAll);

    connect_or_die(this, &ActivityProgressStatusBarTracker::number_of_jobs_changed,
                   m_cumulative_status_widget, &CumulativeStatusWidget::slot_number_of_jobs_changed);
}

ActivityProgressStatusBarTracker::~ActivityProgressStatusBarTracker()
{
    // All KWidgetJobTracker does here is delete the private pImpl pointer.
    // KWidgetJobTracker::Private's destructor then just deletes the eventLoopLocker, which is only
    // non-null if the user has selected "Keep Open".

    qDb() << "DELETING ALL TRACKED OBJECTS";
    cancelAll();

    /// @todo IS THIS CORRECT, or should we be doing something like deleteLater() here?
    m_expanding_frame_widget->deleteLater();
//    m_expanding_frame_widget = nullptr;
}

QWidget *ActivityProgressStatusBarTracker::widget(KJob *job)
{
    QMutexLocker locker(&m_tsi_mutex);

    // Shouldn't ever get here before the widget is constructed (in the constructor).
    /// @todo The nullptr is FBO the call in MainWindow::onStartup() only, find a better way.
    if(is_cumulative_status_job(job) || job == nullptr)
    {
        // The summary widget.
//        M_WARNIF((m_cumulative_status_widget == nullptr));
        Q_CHECK_PTR(m_cumulative_status_widget);
        return m_cumulative_status_widget;
    }
    else
    {
        // A specific KJob's widget.  Should have been created when the KJob was registered.
        auto kjob_widget = m_amlmjob_to_widget_map.value(job, nullptr);
//        M_WARNIF((kjob_widget == nullptr));
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

    qIn() << "REGISTERING JOB:" << kjob;
    AMLMJob::dump_job_info(kjob);

    // Create the widget for this new job.
    auto wdgt = new BaseActivityProgressStatusBarWidget(kjob, this, m_expanding_frame_widget);
    // Make connections.
    wdgt->make_connections();
    wdgt->m_is_job_registered = true;
    wdgt->setAttribute(Qt::WA_DeleteOnClose);

    // Insert the kjob/widget pair into our master map.
    m_amlmjob_to_widget_map.insert(kjob, wdgt);
    Q_EMIT number_of_jobs_changed(m_amlmjob_to_widget_map.size());

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

    qDb() << "REGISTERED JOB:" << kjob;
    dump_qobject(kjob);

    // KWidgetJobTracker does almost the following.
    // It does not pass the job ptr though.
    /// @todo Is that part of our problems?
    QTimer::singleShot(500, this, [=](){SLOT_onShowProgressWidget(kjob);});
}

void ActivityProgressStatusBarTracker::unregisterJob(KJob* kjob)
{
    // Adapted from KWidgetJobTracker's version of this function.
    QMutexLocker locker(&m_tsi_mutex);

    qDb() << "UNREGISTERING JOB:" << kjob;

    Q_CHECK_PTR(this);
    Q_ASSERT(kjob != nullptr);

    AMLMJob::dump_job_info(kjob);

    // KAbstractWidgetJobTracker::unregisterJob() calls:
    //   KJobTrackerInterface::unregisterJob(job);, which calls:
    //     job->disconnect(this);

    // Call down to the base class first; widget may be deleted by deref() below.
    BASE_CLASS::unregisterJob(kjob);

    /// @todo The only thing KWidgetJobTracker does differently here is remove any instances of "job" from the queue.
    with_widget_or_skip(kjob, [=](auto w){
        w->m_is_job_registered = false;
        // Remove the job's widget from the expanding frame.
		m_expanding_frame_widget->removeWidget(w);
		m_expanding_frame_widget->reposition();
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

void ActivityProgressStatusBarTracker::SLOT_onShowProgressWidget(KJob* kjob)
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
        // Don't steal the focus from the current widget (e. g. Kate)
        w->setAttribute(Qt::WA_ShowWithoutActivating);
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


void ActivityProgressStatusBarTracker::finished(KJob *kjob)
{
    // KJobTrackerInterface::finished(KJob *job) does nothing.
    qWr() << "FINISHED KJob:" << kjob;
    with_widget_or_skip(kjob, [=](auto w){
        qWr() << "FINISHED JOB:" << kjob << "WITH WIDGET:" << w;
        w->hide();
    });

    Q_CHECK_PTR(this);
    Q_CHECK_PTR(kjob);

    AMLMJob::dump_job_info(kjob);
}

void ActivityProgressStatusBarTracker::description(KJob *job, const QString &title, const QPair<QString, QString> &field1, const QPair<QString, QString> &field2)
{
    /// This follows the basic pattern of KWidgetJobTracker, with a little C++14 help.
    /// All these "tracker->widget ~signal" handlers get the widget ptr (from a map in that case),
    /// check for null, and if not null do the job.
    with_widget_or_skip(job, [=](auto w){
        w->description(job, title, field1, field2);
    });
}

void ActivityProgressStatusBarTracker::infoMessage(KJob *job, const QString &plain, const QString &rich)
{
    // Prefer rich if it's given.
    with_widget_or_skip(job, [=](auto w){
        w->infoMessage(job, rich.isEmpty() ? plain : rich);
        ;});
}

void ActivityProgressStatusBarTracker::warning(KJob *job, const QString &plain, const QString &rich)
{
    with_widget_or_skip(job, [=](auto w){
        w->warning(job, rich.isEmpty() ? plain : rich);
    });

}

void ActivityProgressStatusBarTracker::totalAmount(KJob *kjob, KJob::Unit unit, qulonglong amount)
{
    // Incoming signal from kjob that setTotalAmount() has been called and d->totalAmount[unit] has
    // been updated.

    /// @note KIO::SimpleJobPrivate hooks into the totalAmount mechanism from
    /// "void SimpleJobPrivate::slotTotalSize(KIO::filesize_t size)", where it does this:
    ///    Q_Q(SimpleJob);
    ///    if (size != q->totalAmount(KJob::Bytes)) {
    ///        q->setTotalAmount(KJob::Bytes, size);
    ///    }
    /// Similar for processedSize().
    /// KIO::listJob etc. are documented to use this mechanism for reporting progress.

    with_widget_or_skip(kjob, [=](auto w){
        w->totalAmount(kjob, unit, amount);
    });
}

void ActivityProgressStatusBarTracker::processedAmount(KJob *job, KJob::Unit unit, qulonglong amount)
{
    with_widget_or_skip(job, [=](auto w)
    {
        w->processedAmount(job, unit, amount);
    });
}

void ActivityProgressStatusBarTracker::totalSize(KJob *kjob, qulonglong amount)
{
    with_widget_or_skip(kjob, [=](auto w){
        w->totalSize(kjob, amount);

//        if(kjob != nullptr && kjob != m_cumulative_status_job)
//        {
//        /// @todo Notify summary widget of the change.

//            auto cumulative_pct = calculate_summary_percent();
//            m_cumulative_status_widget->totalSize(m_cumulative_status_job, cumulative_pct);
//        }
    });
}

void ActivityProgressStatusBarTracker::processedSize(KJob *kjob, qulonglong amount)
{
    with_widget_or_skip(kjob, [=](auto w){
        w->processedSize(kjob, amount);

//        if(kjob != nullptr && kjob != m_cumulative_status_job)
//        {
//        /// @todo Notify summary widget of the change.

//            auto cumulative_pct = calculate_summary_percent();
//            m_cumulative_status_widget->totalSize(m_cumulative_status_job, cumulative_pct);
//        }
    });
}

void ActivityProgressStatusBarTracker::percent(KJob *job, unsigned long percent)
{
    Q_CHECK_PTR(job);

    with_widget_or_skip(job, [=](auto w){

        w->percent(job, percent);

        if(job != nullptr && job != m_cumulative_status_job)
        {
        /// @todo Notify summary widget of the change.

            auto cumulative_pct = calculate_summary_percent();
            m_cumulative_status_widget->percent(m_cumulative_status_job, cumulative_pct);
        }
    });
}

void ActivityProgressStatusBarTracker::speed(KJob *job, unsigned long value)
{
    with_widget_or_skip(job, [=](auto w){
        qDb() << "KJob speed" << job << value;
        w->speed(job, value);
    });
}

void ActivityProgressStatusBarTracker::slotClean(KJob *job)
{
    with_widget_or_skip(job, [=](auto w){
        qDb() << "KJobTrk: slotClean" << job;
    });
}

bool ActivityProgressStatusBarTracker::is_cumulative_status_job(KJob *kjob)
{
    if(kjob == m_cumulative_status_job)
    {
        return true;
    }
    return false;
}

void ActivityProgressStatusBarTracker::make_connections_with_newly_registered_job(KJob *kjob, QWidget *wdgt)
{
    // For Widgets to request deletion of their jobs and associated data (including the pointer to themselves) from the map.
    BaseActivityProgressStatusBarWidget* wdgt_type = qobject_cast<BaseActivityProgressStatusBarWidget*>(wdgt);

    connect_or_die(wdgt_type, &BaseActivityProgressStatusBarWidget::signal_removeJobAndWidgetFromMap,
            this, &ActivityProgressStatusBarTracker::SLOT_removeJobAndWidgetFromMap);
    connect_or_die(kjob, &KJob::totalSize, this, &ActivityProgressStatusBarTracker::totalSize);
    connect_or_die(kjob, &KJob::processedSize, this, &ActivityProgressStatusBarTracker::processedSize);
}

void ActivityProgressStatusBarTracker::removeJobAndWidgetFromMap(KJob* kjob, QWidget *widget)
{
    qDb() << "REMOVING FROM MAP:" << kjob << widget;
    if(m_amlmjob_to_widget_map[kjob] == widget)
    {
        m_amlmjob_to_widget_map.remove(kjob);
        /// @todo Also to-be-shown queue?

        Q_EMIT number_of_jobs_changed(m_amlmjob_to_widget_map.size());
    }
}

void ActivityProgressStatusBarTracker::directCallSlotStop(KJob *kjob)
{
    slotStop(kjob);
}

int ActivityProgressStatusBarTracker::calculate_summary_percent()
{
    /// @todo This is about the most naive algorithm possible to get a cumulative percent complete,
    ///       but it's good enough for the moment.
    long long total_jobs = 0;
    long long cumulative_completion_pct = 0;
    m_amlmjob_to_widget_map.for_each_key_value_pair([&](KJob* job, BaseActivityProgressStatusBarWidget* widget) {
        // We do the total_jobs++ in here vs. a .size() outside this loop because in here we have
        // the map locked, so the numbers will be consistent.
        total_jobs += 1;
        cumulative_completion_pct += job->percent();
        ;});

    int retval;
    if(total_jobs == 0)
    {
        // Avoid a div-by-0.
        retval = 0;
    }
    else
    {
        retval = static_cast<double>(cumulative_completion_pct)/total_jobs;
    }

    return retval;
}

#if 0
void ActivityProgressStatusBarTracker::setStopOnClose(KJob *kjob, bool stopOnClose)
{
//if (!progressWidget.contains(job))
//{
//        return;
//}
//progressWidget[job]->stopOnClose = stopOnClose;
}

bool ActivityProgressStatusBarTracker::stopOnClose(KJob *job) const
{

}
#endif

void ActivityProgressStatusBarTracker::setAutoDelete(KJob *kjob, bool autoDelete)
{
    Q_CHECK_PTR(kjob);
    kjob->setAutoDelete(autoDelete);
}

bool ActivityProgressStatusBarTracker::autoDelete(KJob *kjob) const
{
    Q_CHECK_PTR(kjob);
    // The KJob knows if it's autoDelete or not.
    return kjob->isAutoDelete();
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


    // Translate the the root widget's topLeft() to global coords.
    auto pos_tl_global = summary_widget->mapToGlobal(rect.topLeft());
    qDb() << "Root Frame topLeft(), Global:" << pos_tl_global;

//    m_expanding_frame_widget->popup(pos_tl_global);
//    m_expanding_frame_widget->updateGeometry();
//    m_expanding_frame_widget->activateWindow();
    m_expanding_frame_widget->show();
    m_expanding_frame_widget->raise();

    qDb() << "AFTER FRAME GEOMETRY:" << m_expanding_frame_widget->geometry();

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



