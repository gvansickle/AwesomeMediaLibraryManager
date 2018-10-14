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
#include <QWindow>
#include <QStatusBar>

/// KF5
#include <KJob>
#include <KIO/ListJob>

/// Ours
#include <AMLMApp.h>
#include <utils/TheSimplestThings.h>
#include "BaseActivityProgressStatusBarWidget.h"
#include "CumulativeStatusWidget.h"
#include <gui/MainWindow.h>

ActivityProgressStatusBarTracker::ActivityProgressStatusBarTracker(QWidget *parent) : BASE_CLASS(parent),
    m_tracked_job_state_mutex(QMutex::Recursive /** @todo Shouldn't need to do recursive, but it does make things easier at the moment. */)
{
    // Save the parent widget.
    m_parent_widget = parent;

    // Create the job which will contain all other jobs.
    /// @note At least that's the theory, eventually.
    m_cumulative_status_job = CumulativeAMLMJob::make_job(parent);

    // Create the summary widget
    /// @todo Should this have its own separate tracker?
    m_cumulative_status_widget = new CumulativeStatusWidget(m_cumulative_status_job, this, parent);

    // Make cumulative status widget connections.
    /// @todo

    /// Register the cumulative job and widget.
    /// @todo Move?
//M_WARNING("TODO: Need to not delete this job/wdgt pair ever (e.g. on cancel)");
//    m_cumulative_status_tracker->registerJob(cumulative_job, qobject_cast<BaseActivityProgressStatusBarWidget*>(m_cumulative_status_widget));

    m_expanding_frame_widget = new ExpandingFrameWidget(m_cumulative_status_widget, MainWindow::instance()->statusBar());

    m_expanding_frame_widget->hide();

    // Connect the cumulative status widget button's signals to slots in this class, they need to apply to all sub-jobs.
    /// @todo m_cumulative_status_widget's cancel_job button state should be enabled/disabled based on child job cancelable capabilities.
    connect_or_die(m_cumulative_status_widget, &CumulativeStatusWidget::cancel_job,
                this, &ActivityProgressStatusBarTracker::SLOT_CancelAllKJobs);
    connect_or_die(m_cumulative_status_widget, &CumulativeStatusWidget::SIGNAL_show_hide_subjob_display,
            this, &ActivityProgressStatusBarTracker::toggleSubjobDisplay);

    connect_or_die(m_expanding_frame_widget, &ExpandingFrameWidget::visibilityChanged,
                   m_cumulative_status_widget, &CumulativeStatusWidget::SLOT_SubjobDisplayVisible);

    connect_or_die(this, &ActivityProgressStatusBarTracker::number_of_jobs_changed,
                   m_cumulative_status_widget, &CumulativeStatusWidget::slot_number_of_jobs_changed);

    // Make our internal signal->slot connections.
    make_internal_connections();
}

ActivityProgressStatusBarTracker::~ActivityProgressStatusBarTracker()
{
    // All KWidgetJobTracker does here is delete the private pImpl pointer.
    // KWidgetJobTracker::Private's destructor then just deletes the eventLoopLocker, which is only
    // non-null if the user has selected "Keep Open".

    // KDevelop's RunController derivation hierarchy just does = default here, which ends up
    // just calling ~KJobTrackerInterface().

#if 0
    QMutexLocker locker(&m_tracked_job_state_mutex);

    /// @todo Not sure if this is right or not: M_WARNING("TODO: Probably need to not call cancellAll() in here.");
M_WARNING("If we do this here, we need to wait for all jobs to stop.");
    qDb() << "DELETING ALL TRACKED OBJECTS";
    SLOT_CancelAllKJobs();

//#error "Yeah this asserts"
    AMLM_ASSERT_EQ(m_amlmjob_to_widget_map.size(), 0);
#endif
    m_expanding_frame_widget->deleteLater();
}

QWidget *ActivityProgressStatusBarTracker::widget(KJob *job)
{
    QMutexLocker locker(&m_tracked_job_state_mutex);

    Q_CHECK_PTR(job);

    // Shouldn't ever get here before the widget is constructed (in the constructor).
    /// @todo The nullptr is FBO the call in MainWindow::onStartup() only, find a better way.
    if(is_cumulative_status_job(job) || job == nullptr)
    {
        // The summary widget.
        Q_CHECK_PTR(m_cumulative_status_widget);
        return m_cumulative_status_widget;
    }
    else
    {
        // A specific KJob's widget.  Should have been created when the KJob was registered.
        auto kjob_widget = m_amlmjob_to_widget_map.value(job, nullptr);
        Q_CHECK_PTR(kjob_widget);
        return kjob_widget;
    }
}

QWidget *ActivityProgressStatusBarTracker::get_status_bar_widget()
{
    Q_CHECK_PTR(m_cumulative_status_widget);
    return m_cumulative_status_widget;
}

void ActivityProgressStatusBarTracker::registerJob(KJob* kjob)
{
    // Adapted from KWidgetJobTracker's version of this function.
    QMutexLocker locker(&m_tracked_job_state_mutex);

    Q_CHECK_PTR(this);
    Q_ASSERT(kjob);

    // Monitor hack to detect if kjob gets deleted.
    /// @todo This is from KUIServerJobTracker and I guess is used all over Qt-land, but there has to be a better way.
    QPointer<KJob> jobWatch = kjob;

    if(!jobWatch)
    {
        qDb() << "Job deleted while being registered";
        return;
    }

    qIn() << "REGISTERING JOB:" << kjob;
//    AMLMJob::dump_job_info(kjob);

    Q_ASSERT(!is_cumulative_status_job(kjob));

    // Create the widget for this new job.
    QPointer<BaseActivityProgressStatusBarWidget> wdgt = new BaseActivityProgressStatusBarWidget(kjob, this, m_expanding_frame_widget);
    /// @todo Watch this, deleting the widget on close here has caused us to crash in the past.
    wdgt->setAttribute(Qt::WA_DeleteOnClose);

    /// @todo enqueue on a widgets-to-be-shown queue?  Not clear why that exists in KWidgetJobTracker.

    // Add the new widget to the expanging frame.
    m_expanding_frame_widget->addWidget(wdgt);
    m_expanding_frame_widget->reposition();

    // Make signal->slot connections.
    /// @todo Should this really be here, or better in the onShowProgressWidget() call?
    make_connections_with_newly_registered_job(kjob, wdgt);

    if(!jobWatch)
    {
        qCro() << "Job deleted while being registered";
        wdgt->deleteLater();
        return;
    }

    // KAbstractWidgetJobTracker::registerJob(KJob *job) simply calls:
    //   KJobTrackerInterface::registerJob(KJob *job) does nothing but connect
    //   many of the KJob signals to slots of this.  Specifically finsihed-related:
    //     QObject::connect(job, SIGNAL(finished(KJob*)), this, SLOT(unregisterJob(KJob*)));
    //     QObject::connect(job, SIGNAL(finished(KJob*)), this, SLOT(finished(KJob*)));
    BASE_CLASS::registerJob(kjob);

	if(auto jobptr = dynamic_cast<AMLMJob*>(kjob))
	{
		AMLMApp::IPerfectDeleter()->addAMLMJob(jobptr);
	}
	/// @todo

    if(!jobWatch)
    {
        qCro() << "Job deleted while being registered";
        wdgt->deleteLater();
        return;
    }

    // Insert the kjob/widget pair into our master map.
    m_amlmjob_to_widget_map.insert(kjob, wdgt);

    // Emit a signal that a job has been added.
    Q_EMIT number_of_jobs_changed(m_amlmjob_to_widget_map.size());

//    qDb() << "REGISTERED JOB:" << kjob;
//    dump_qobject(kjob);

    // KWidgetJobTracker does almost the following.
    // It does not pass the job ptr though.
    /// @todo Is that part of our problems?
    QTimer::singleShot(500, this, [=](){SLOT_onShowProgressWidget(kjob);});
}

void ActivityProgressStatusBarTracker::unregisterJob(KJob* kjob)
{
    // Adapted from KWidgetJobTracker's version of this function.
    QMutexLocker locker(&m_tracked_job_state_mutex);

    INTERNAL_unregisterJob(kjob);
}

void ActivityProgressStatusBarTracker::SLOT_onKJobDestroyed(QObject *kjob)
{
    QMutexLocker locker(&m_tracked_job_state_mutex);

    KJob* kjob_ptr = dynamic_cast<KJob*>(kjob);

    Q_CHECK_PTR(kjob_ptr);

    // Check if the job was destroyed prior to being unregistered.
    if(m_amlmjob_to_widget_map.keys().contains(kjob_ptr))
    {
        qWr() << "KJOB DESTROYED BEFORE BEING UNREGISTERED:" << kjob_ptr;
        unregisterJob(kjob_ptr);
    }
}

void ActivityProgressStatusBarTracker::SLOT_onShowProgressWidget(KJob* kjob)
{
    QMutexLocker locker(&m_tracked_job_state_mutex);

M_WARNING("DEBUG");
return;

    // Called on a timer timeout after a new job is registered.
    Q_CHECK_PTR(kjob);

    /// @todo If queue is empty return.
    /// else dequeue job, look up qwidget, and show it.
M_WARNING("BUG: The kjob could be finished and deleted before we get here.");

    // Look up the widget associated with this kjob.
    // If it's been unregistered before we get here, this will return nullptr.
    with_widget_or_skip(kjob, [=](auto w){
        qDb() << "SHOWING WIDGET:" << w;
        // Don't steal the focus from the current widget (e. g. Kate)
        w->setAttribute(Qt::WA_ShowWithoutActivating);
        w->show();
    });
}

void ActivityProgressStatusBarTracker::SLOT_CancelAllKJobs()
{
    QMutexLocker locker(&m_tracked_job_state_mutex);

    qDb() << "CANCELLING ALL JOBS";

    //  Get a copy of the list of all the keys in the map, because composite jobs will delete subjobs.
    QList<QPointer<KJob>> kjoblist = m_amlmjob_to_widget_map.keys();

    qDb() << "CANCELLING ALL JOBS: Num KJobs:" << m_amlmjob_to_widget_map.size() << "List size:" << kjoblist.size();

    for(const QPointer<KJob>& kjob : kjoblist)
    {
		if(kjob == nullptr || !m_amlmjob_to_widget_map.keys().contains(kjob))
        {
            // No such KJob anymore.
            continue;
        }
        if(kjob->capabilities() & KJob::Killable)
        {
            qIno() << "Killing KJob:" << kjob;
            // Synchronous call of KJob::kill().
            /// @todo Don't know if we want EmitResult here or not.
            kjob->kill(KJob::EmitResult);
//        Q_EMIT INTERNAL_SIGNAL_slotStop(kjob);
        }
        else
        {
			qCro() << "KJob was marked non-killable, expect a crash.";
        }
    }

M_WARNING("I think we need to wait for all jobs to stop here.");
    qDb() << "CANCELLING ALL JOBS: KJobs REMAINING:" << m_amlmjob_to_widget_map.size();
}


void ActivityProgressStatusBarTracker::finished(KJob *kjob)
{
    // KJobTrackerInterface::finished(KJob *job) does nothing.
    qWr() << "FINISHED KJob:" << kjob;
//    with_widget_or_skip(kjob, [=](auto w){
//        qWr() << "FINISHED JOB:" << kjob << "WITH WIDGET:" << w;
//        Q_CHECK_PTR(kjob);
//        Q_CHECK_PTR(w);
//        w->hide();
//    });

//    Q_CHECK_PTR(this);

//    AMLMJob::dump_job_info(kjob);
}

void ActivityProgressStatusBarTracker::description(KJob *kjob, const QString &title, const QPair<QString, QString> &field1, const QPair<QString, QString> &field2)
{
//    Q_EMIT SIGNAL_description(kjob, title, field1, field2);

//    /// This follows the basic pattern of KWidgetJobTracker, with a little C++14 help.
//    /// All these "tracker->widget ~signal" handlers get the widget ptr (from a map in that case),
//    /// check for null, and if not null do the job.
//    with_widget_or_skip(job, [=](auto w){
//        w->description(job, title, field1, field2);
//    });
}

void ActivityProgressStatusBarTracker::infoMessage(KJob *job, const QString &plain, const QString &rich)
{
    // Prefer rich if it's given.
//    with_widget_or_skip(job, [=](auto w){
//        w->infoMessage(job, rich.isEmpty() ? plain : rich);
//        ;});
}

void ActivityProgressStatusBarTracker::warning(KJob *job, const QString &plain, const QString &rich)
{
//    with_widget_or_skip(job, [=](auto w){
//        w->warning(job, rich.isEmpty() ? plain : rich);
//    });
}

void ActivityProgressStatusBarTracker::totalAmount(KJob *kjob, KJob::Unit unit, qulonglong amount)
{
    /// @note Ignoring *Amount() signals for now, using *Size() for overall progress.

    // Slot from kjob that setTotalAmount() has been called and d->totalAmount[unit] has
    // been updated.

    /// @note KIO::SimpleJobPrivate hooks into the totalAmount mechanism from
    /// "void SimpleJobPrivate::slotTotalSize(KIO::filesize_t size)", where it does this:
    ///    Q_Q(SimpleJob);
    ///    if (size != q->totalAmount(KJob::Bytes)) {
    ///        q->setTotalAmount(KJob::Bytes, size);
    ///    }
    /// Similar for processedSize().
    /// KIO::listJob etc. are documented to use this mechanism for reporting progress.

//    with_widget_or_skip(kjob, [=](auto w){
//        w->totalAmount(kjob, unit, amount);
//    });
}

void ActivityProgressStatusBarTracker::processedAmount(KJob *job, KJob::Unit unit, qulonglong amount)
{
    /// @note Ignoring *Amount() signals for now, using *Size() for overall progress.
    // Incoming signal from kjob that setProcessedAmount() has been called and d->processedAmount[unit] has
    // been updated.

//    Q_EMIT SIGNAL_processedAmount(job, unit, amount);
//    with_widget_or_skip(job, [=](auto w)
//    {
//        w->processedAmount(job, unit, amount);
//    });
}

void ActivityProgressStatusBarTracker::totalSize(KJob *kjob, qulonglong amount)
{
//    with_widget_or_skip(kjob, [=](auto w){
//        w->totalSize(kjob, amount);

        if(kjob != nullptr && kjob != m_cumulative_status_job)
        {
        /// @todo Notify summary widget of the change.

            auto cumulative_pct = calculate_summary_percent();
            m_cumulative_status_widget->totalSize(m_cumulative_status_job, cumulative_pct);
        }
//    });
}

void ActivityProgressStatusBarTracker::processedSize(KJob *kjob, qulonglong amount)
{
//    with_widget_or_skip(kjob, [=](auto w){
//        w->processedSize(kjob, amount);

        if(kjob != nullptr && kjob != m_cumulative_status_job)
        {
        /// @todo Notify summary widget of the change.

            auto cumulative_pct = calculate_summary_percent();
            m_cumulative_status_widget->totalSize(m_cumulative_status_job, cumulative_pct);
        }
//    });
}

void ActivityProgressStatusBarTracker::percent(KJob *job, unsigned long percent)
{
//    Q_CHECK_PTR(job);

//    with_widget_or_skip(job, [=](auto w){

//        w->percent(job, percent);

        if(job != nullptr && job != m_cumulative_status_job)
        {
        /// @todo Notify summary widget of the change.

            auto cumulative_pct = calculate_summary_percent();
            m_cumulative_status_widget->percent(m_cumulative_status_job, cumulative_pct);
        }
//    });
}

void ActivityProgressStatusBarTracker::speed(KJob *job, unsigned long value)
{
//    with_widget_or_skip(job, [=](auto w){
//        qDb() << "KJob speed" << job << value;
//        w->speed(job, value);
//    });
}

void ActivityProgressStatusBarTracker::slotClean(KJob *job)
{
//    with_widget_or_skip(job, [=](auto w){
//        qDb() << "KJobTrk: slotClean" << job;
//    });
}

/**
 * "This [protected Q_SLOT] should be called for correct cancellation of IO operation
 *  Connect this to the progress widgets buttons etc."
 *
 * Calls job->kill(KJob::EmitResult) and emits stopped(job).
 * This override just calls base class.
 *
 * @override KAbstractWidgetJobTracker.
 */
void ActivityProgressStatusBarTracker::slotStop(KJob *kjob)
{
    qDb() << "GOT slotStop() for KJob:" << kjob;

    /**
     * @todo There's still something wrong between this and requestAbort() and doKill() and
     * I don't know what all else.  We get multiple of these from a single cancel button push,
     * and the TW::Job doesn't actually end until much later (after several slotStop()s).
     * Similar with KIO::Jobs.
     */

    if(kjob == nullptr || is_cumulative_status_job(kjob))
    {
        qWr() << "KJOB LOOKS LIKE CUMULATIVE STATUS JOB, BALKING";
        return;
    }

    Q_CHECK_PTR(kjob);
    BASE_CLASS::slotStop(kjob);
}

bool ActivityProgressStatusBarTracker::is_cumulative_status_job(KJob *kjob)
{
    Q_CHECK_PTR(kjob);
    if(kjob == m_cumulative_status_job)
    {
        return true;
    }
    return false;
}

void ActivityProgressStatusBarTracker::make_internal_connections()
{
    // The tracker emits stopped(KJob*) when the user cancels a job through slotStop(KJob*).
    connect_or_die(this, &ActivityProgressStatusBarTracker::stopped, this, [=](KJob* kjob){
        qDb() << "STOPPED BY USER:" << kjob;
                ;});

    // Connect our internal slotStop()-was-emitted signal INTERNAL_SIGNAL_slotStop and re-emit FBO cancelAll().
    connect_or_die(this, &ActivityProgressStatusBarTracker::INTERNAL_SIGNAL_slotStop,
                   this, &ActivityProgressStatusBarTracker::slotStop);

    connect_or_die(AMLMApp::instance(), &AMLMApp::aboutToShutdown, this, &ActivityProgressStatusBarTracker::SLOT_onAboutToShutdown);
}

void ActivityProgressStatusBarTracker::make_connections_with_newly_registered_job(KJob *kjob, QWidget *wdgt)
{
    Q_CHECK_PTR(kjob);
    Q_CHECK_PTR(wdgt);
    BaseActivityProgressStatusBarWidget* wdgt_type = qobject_cast<BaseActivityProgressStatusBarWidget*>(wdgt);
    Q_CHECK_PTR(wdgt_type);

    /**
     * Most signal/slot connections between the KJob and this tracker will have already been made by the base classes
     * on the registerJob() call.  The base classes do not make any connections to any widgets.
     *
     * @link https://api.kde.org/frameworks/kcoreaddons/html/classKJobTrackerInterface.html#a02be1fe828ead6c57601272950c1cd4d
     *
     * The default implementation connects the following KJob signals to the respective protected slots of
     * this tracker class:
//    finished() (also connected to the unregisterJob() slot)
//    suspended()
//    resumed()
//    description()
//    infoMessage()
//    totalAmount()
//    processedAmount()
//    percent()
//    speed()

     */

    // Connect the total/processedSize signals to this tracker, which are not connected by the tracker base classes for some reason.
    // We'll use that for the overall progress bar.
    connect_or_die(kjob, &KJob::totalSize, this, &ActivityProgressStatusBarTracker::totalSize);
    connect_or_die(kjob, &KJob::processedSize, this, &ActivityProgressStatusBarTracker::processedSize);

    //
    // Connect some signals directly from the kjob to the widget's slots.
    //
    connect_or_die(kjob, &KJob::description, wdgt_type, &BaseActivityProgressStatusBarWidget::description);
    connect_or_die(kjob, &KJob::infoMessage, wdgt_type, &BaseActivityProgressStatusBarWidget::infoMessage);
    connect_or_die(kjob, &KJob::warning, wdgt_type, &BaseActivityProgressStatusBarWidget::warning);
    connect_or_die(kjob, qOverload<KJob*, KJob::Unit, qulonglong>(&KJob::totalAmount),
                   wdgt_type, &BaseActivityProgressStatusBarWidget::totalAmount);
    connect_or_die(kjob, qOverload<KJob*, KJob::Unit, qulonglong>(&KJob::processedAmount),
                   wdgt_type, &BaseActivityProgressStatusBarWidget::processedAmount);
    connect_or_die(kjob, &KJob::totalSize, wdgt_type, &BaseActivityProgressStatusBarWidget::totalSize);
    connect_or_die(kjob, &KJob::processedSize, wdgt_type, &BaseActivityProgressStatusBarWidget::processedSize);
    connect_or_die(kjob, qOverload<KJob*, unsigned long>(&KJob::percent), wdgt_type, &BaseActivityProgressStatusBarWidget::percent);
    connect_or_die(kjob, &KJob::speed, wdgt_type, &BaseActivityProgressStatusBarWidget::speed);
    // Suspend/resume state.
    connect_or_die(kjob, &KJob::suspended, wdgt_type, &BaseActivityProgressStatusBarWidget::suspended);
    connect_or_die(kjob, &KJob::resumed, wdgt_type, &BaseActivityProgressStatusBarWidget::resumed);

    // kjob->widget, tells the widget to hide itself since kjob emitted finished().
    // kjob->tracker is already done in base class: connect_or_die(kjob, &KJob::finished, this, &ActivityProgressStatusBarTracker::finished);
    connect_or_die(kjob, &KJob::finished, wdgt_type, &BaseActivityProgressStatusBarWidget::hide);

    // Connect the kjob's destroyed() signal to a handler in this class.
    connect_or_die(kjob, &KJob::destroyed, this, &ActivityProgressStatusBarTracker::SLOT_onKJobDestroyed);

    //
    // Connect the widget's signals such as "user wants to cancel" to this tracker's slotStop(KJob*) slot.
    //
    connect_or_die(wdgt_type, &BaseActivityProgressStatusBarWidget::cancel_job, this, &ActivityProgressStatusBarTracker::slotStop);
    connect_or_die(wdgt_type, &BaseActivityProgressStatusBarWidget::pause_job, this, &ActivityProgressStatusBarTracker::slotSuspend);
    connect_or_die(wdgt_type, &BaseActivityProgressStatusBarWidget::resume_job, this, &ActivityProgressStatusBarTracker::slotResume);
}

void ActivityProgressStatusBarTracker::INTERNAL_unregisterJob(KJob *kjob)
{
    QPointer<KJob> kjob_qp(kjob);

M_WARNING("Could KJob* already be finished and autoDeleted here?");
    Q_CHECK_PTR(kjob_qp);

    qIno() << "UNREGISTERING JOB:" << kjob_qp;

    Q_CHECK_PTR(this);
    Q_CHECK_PTR(kjob_qp);

    // KAbstractWidgetJobTracker::unregisterJob() calls:
    //   KJobTrackerInterface::unregisterJob(job);, which calls:
    //     job->disconnect(this);

    // Call down to the base class first.
    // A number of examples, including KDevelop, do this first like this.
    BASE_CLASS::unregisterJob(kjob_qp);

    Q_CHECK_PTR(kjob_qp);

    qIno() << "SIGNALS DISCONNECTED:" << kjob_qp;

    // If kjob was never registered, something's broken.
    AMLM_ASSERT_EQ(m_amlmjob_to_widget_map.keys().contains(kjob_qp), true);

    // Get ptr to the widget, if any.
    auto w = m_amlmjob_to_widget_map.value(kjob_qp, nullptr);

    qDb() << "REMOVING FROM MAP:" << kjob << w;
    if(w == nullptr)
    {
        qWro() << "KJob" << kjob << "was registered but has no widget.";
        m_amlmjob_to_widget_map.remove(kjob_qp);
    }
    else
    {
        // Remove the job's widget from the expanding frame.
        m_expanding_frame_widget->removeWidget(w);
        m_expanding_frame_widget->reposition();
		// Remove this job from the map, which will remove its widget.
        m_amlmjob_to_widget_map.remove(kjob_qp);
        w->deleteLater();
    }

//    /// @todo The only thing KWidgetJobTracker does differently here is remove any instances of "job" from the queue.
//#error "This will not delete a kjob with no widget"
//    with_widget_or_skip(kjob_qp, [=](auto w){
//        // Remove the job's widget from the expanding frame.
//        m_expanding_frame_widget->removeWidget(w);
//        m_expanding_frame_widget->reposition();
//        removeJobAndWidgetFromMap(kjob_qp, w);
//        w->deleteLater();
//        });

    Q_CHECK_PTR(kjob_qp);

    Q_EMIT number_of_jobs_changed(m_amlmjob_to_widget_map.size());

    qDb() << "JOB UNREGISTERED:" << kjob_qp;
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

int ActivityProgressStatusBarTracker::getNumTrackedJobs() const
{
    return m_amlmjob_to_widget_map.size();
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
    auto summary_widget = get_status_bar_widget();
    auto rect = summary_widget->frameGeometry();
    qDb() << "Root Frame Rect:" << rect << "parent:" << summary_widget->parentWidget();

//    m_expanding_frame_widget->windowHandle()->setTransientParent(RootWidget()->windowHandle());


    // Translate the the root widget's topLeft() to global coords.
    auto pos_tl_global = summary_widget->mapToGlobal(rect.topLeft());
    qDb() << "Root Frame topLeft(), Global:" << pos_tl_global;

    m_expanding_frame_widget->show();
    m_expanding_frame_widget->raise();

    qDb() << "AFTER FRAME GEOMETRY:" << m_expanding_frame_widget->geometry();
}

void ActivityProgressStatusBarTracker::hideSubJobs()
{
    m_expanding_frame_widget->hide();
}

void ActivityProgressStatusBarTracker::SLOT_onAboutToShutdown()
{
    qIno() << "SHUTDOWN, TRACKING" << getNumTrackedJobs() << "JOBS, CANCELLING ALL";
    SLOT_CancelAllKJobs();
    qIno() << "SHUTDOWN, NOW TRACKING" << getNumTrackedJobs() << "JOBS";
}



