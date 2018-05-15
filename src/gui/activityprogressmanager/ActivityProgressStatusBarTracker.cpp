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

//#include <utils/DebugHelpers.h>
//#include <utils/StringHelpers.h>
#include <utils/TheSimplestThings.h>
//#include <gui/helpers/Tips.h>
#include "BaseActivityProgressStatusBarWidget.h"
#include "ActivityProgressMultiTracker.h"


template <typename Lambda>
void with_widget_or_skip(BaseActivityProgressStatusBarWidget* widget, Lambda l)
{
    if(widget)
    {
        l(widget);
    }
}

ActivityProgressStatusBarTracker::ActivityProgressStatusBarTracker(AMLMJobPtr job, ActivityProgressMultiTracker* parent_tracker, QWidget *parent)
    : KAbstractWidgetJobTracker(parent),
      m_parent_tracker(parent_tracker), m_job(job)
{
    setObjectName(uniqueQObjectName());

    // Create the widget.
    init(job, parent);
    // Register the job.
    registerJob(job);

    qDb() << "JOB PARENT:" << m_job->parent();
}

ActivityProgressStatusBarTracker::~ActivityProgressStatusBarTracker()
{
M_WARNING("TODO - CRASH");
    /* We've already deleted our children before we get here:
2   QObject::disconnect(QObject const *, const char *, QObject const *, const char *) qobject.cpp                          2983 0x7ffff2f3695a
3   ActivityProgressStatusBarTracker::unregisterJob                                   ActivityProgressStatusBarTracker.cpp 116  0x4a7f8e
4   ActivityProgressStatusBarTracker::~ActivityProgressStatusBarTracker               ActivityProgressStatusBarTracker.cpp 42   0x4a7fe6
5   ActivityProgressStatusBarTracker::~ActivityProgressStatusBarTracker               ActivityProgressStatusBarTracker.cpp 38   0x4a8007
6   QObjectPrivate::deleteChildren                                                    qobject.cpp                          1993 0x7ffff2f3829c
7   QWidget::~QWidget                                                                 qwidget.cpp                          1703 0x7ffff3b353a2
[...]
*/
    // Looks like m_job is our child, not completely clear where/how/why.  So we can't unregister here. ???
//    if(m_job)
//    {
//        unregisterJob(m_job);
//    }

/**
 * Not clear what's happening here. The DirScanJob is parented to Experimental on construction.
 */
qDb() << "ActivityProgressStatusBarTracker DELETED";
}

QWidget *ActivityProgressStatusBarTracker::widget(KJob *job)
{
    // Shouldn't ever get here before the widget is constructed (in the constructor).
    Q_CHECK_PTR(m_widget);
    return m_widget;
}

QWidget *ActivityProgressStatusBarTracker::widget(AMLMJobPtr job)
{
    // Shouldn't ever get here before the widget is constructed (in the constructor).
    Q_CHECK_PTR(m_widget);
    return m_widget;
}


void ActivityProgressStatusBarTracker::init(AMLMJobPtr job, QWidget *parent)
{
    // Create the widget for this new job.
    m_widget = new BaseActivityProgressStatusBarWidget(job, /*tracker=*/this, parent);

    Q_CHECK_PTR(m_widget);

    // Make the widget->tracker connections.
    connect(m_widget, &BaseActivityProgressStatusBarWidget::cancel_job, this, &ActivityProgressStatusBarTracker::slotStop);
    connect(m_widget, &BaseActivityProgressStatusBarWidget::pause_job, this, &ActivityProgressStatusBarTracker::slotSuspend);
    connect(m_widget, &BaseActivityProgressStatusBarWidget::resume_job, this, &ActivityProgressStatusBarTracker::slotResume);

M_WARNING("TODO: Make the tracker->widget connections.");
M_WARNING("TODO: Make the tracker->parent_tracker connections.");
}


void ActivityProgressStatusBarTracker::registerJob(KJob *job)
{
    Q_CHECK_PTR(this);
    Q_ASSERT(job);

    qWr() << "REGISTERING KJOB:" << job;

    AMLMJobPtr amlm_job = qobject_cast<AMLMJob*>(job);
    Q_ASSERT(amlm_job);

    // Forward to the AMLMJobPtr overload.
    registerJob(amlm_job);
}

void ActivityProgressStatusBarTracker::unregisterJob(KJob *job)
{
    Q_CHECK_PTR(this);
    Q_ASSERT_X(job != nullptr, __PRETTY_FUNCTION__, "Bad incoming KJob*");

    qWr() << "UNREGISTERING KJOB:" << job;

M_WARNING("CRASH: Looks like we can get in here with a KJob* which won't dynamic cast to an AMLMJobPtr");
    AMLMJobPtr amlm_job = qobject_cast<AMLMJob*>(job);

    Q_ASSERT_X(amlm_job != nullptr, __PRETTY_FUNCTION__, "Failed to cast KJob* to AMLMJobPtr");

    // Forward to the AMLMJobPtr overload.
    unregisterJob(amlm_job);

    qWr() << "UNREGISTERED KJOB:" << amlm_job;
}

void ActivityProgressStatusBarTracker::registerJob(AMLMJobPtr job)
{
    Q_CHECK_PTR(this);
    Q_ASSERT(job);
    Q_CHECK_PTR(m_widget);

    // Create the widget for this new job.
	m_widget->m_is_job_registered = true;
    /// @todo Doesn't seem to matter crash-wise.
//    m_widget->setAttribute(Qt::WA_DeleteOnClose);

    BASE_CLASS::registerJob(job);

    qDb() << "KJobTrk: AMLMJob info:" << job;
    qDb() << "KJobTrk:" << M_NAME_VAL(job->capabilities()) << "\n"
          << M_NAME_VAL(job->isSuspended()) << "\n"
          << M_NAME_VAL(job->isAutoDelete()) << "\n"
          << M_NAME_VAL(job->error()) << "\n"
          << M_NAME_VAL(job->errorText()) << "\n"
          << M_NAME_VAL(job->errorString());

    dump_tracker_info();

    /// @todo Need to do this?  From KWidgetJobTracker:
    //QTimer::singleShot(500, this, SLOT(_k_showProgressWidget()));
}

void ActivityProgressStatusBarTracker::unregisterJob(AMLMJobPtr job)
{
    Q_CHECK_PTR(this);
    Q_ASSERT(job);

    // Call down to the base class first; widget may be deleted by deref() below.
    BASE_CLASS::unregisterJob(job);

    Q_CHECK_PTR(m_widget);
    with_ptr_or_skip(m_widget, [=](auto w){
        w->m_is_job_registered = false;
        w->deref();
    });
}

void ActivityProgressStatusBarTracker::dump_tracker_info()
{
    QVector<AMLMJobPtr> joblist;
    joblist << m_job;

    for(auto &i : joblist)
    {
        qIn() << "INFO FROM TRACKER:" << this << "RE JOB:" << i;
        qIn() << M_NAME_VAL(autoDelete(i)) << "\n"
              << M_NAME_VAL(stopOnClose(i)) << "\n";
    }

}

void ActivityProgressStatusBarTracker::description(KJob *job, const QString &title, const QPair<QString, QString> &field1, const QPair<QString, QString> &field2)
{
//    Q_CHECK_PTR(m_widget);
//    m_widget->setDescription(title, field1, field2);

    /// This follows the basic pattern of KWidgetJobTracker, with a little C++14 help.
    /// All these "tracker->widget ~signal" handlers get the widget ptr (from a map in that case),
    /// check for null, and if not null do the job.
    with_widget_or_skip(m_widget, [=](auto* w){
        w->setDescription(title, field1, field2);
    });
}

void ActivityProgressStatusBarTracker::infoMessage(KJob *job, const QString &plain, const QString &rich)
{
    // Prefer rich if it's given.
//    qDb() << "INFOMESSAGE RECEIVED";
//    Q_CHECK_PTR(m_widget);
//    m_widget->setInfoMessage(rich.isEmpty() ? plain : rich);
    with_widget_or_skip(m_widget, [=](auto* w){
        w->setInfoMessage(rich.isEmpty() ? plain : rich);
        ;});
}

void ActivityProgressStatusBarTracker::warning(KJob *job, const QString &plain, const QString &rich)
{
    with_widget_or_skip(m_widget, [=](auto* w){;
        w->setWarning(rich.isEmpty() ? plain : rich);
    });

}

void ActivityProgressStatusBarTracker::totalAmount(KJob *job, KJob::Unit unit, qulonglong amount)
{
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
    with_widget_or_skip(m_widget, [=](auto* w)
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
                m_widget->setRange(0, 0);
                m_widget->setValue(m_processedSize);
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
    with_widget_or_skip(m_widget, [=](auto* w){
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
    with_widget_or_skip(m_widget, [=](auto* w){
        qDb() << "KJobTrk: speed" << job << value;
    });
}

void ActivityProgressStatusBarTracker::finished(KJob *job)
{
    with_widget_or_skip(m_widget, [=](auto* w){
        qWr() << "FINISHED JOB:" << job << "WITH NO WIDGET";
    });

    Q_CHECK_PTR(this);
    Q_CHECK_PTR(job);

    qDb() << "KJobTrk: FINISHED KJob:" << job;
    qDb() << "KJobTrk: FINISHED :" << M_NAME_VAL(job->capabilities())
          << M_NAME_VAL(job->isSuspended())
          << M_NAME_VAL(job->isAutoDelete())
          << M_NAME_VAL(job->error())
          << M_NAME_VAL(job->errorText())
          << M_NAME_VAL(job->errorString());
}

void ActivityProgressStatusBarTracker::slotClean(KJob *job)
{
    with_widget_or_skip(m_widget, [=](auto* w){
        qDb() << "KJobTrk: slotClean" << job;
    });
}




