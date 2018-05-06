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

#include "ActivityProgressStatusBarWidget.h"

/// Qt5
//#include <QHBoxLayout>
//#include <QIcon>
//#include <QLabel>
//#include <QProgressBar>
//#include <QToolButton>

/// KF5

/// Ours
#include <utils/DebugHelpers.h>
#include <utils/StringHelpers.h>
//#include <gui/helpers/Tips.h>
#include "BaseActivityProgressStatusBarWidget.h"

ActivityProgressStatusBarWidget::ActivityProgressStatusBarWidget(TWJobWrapper* job, BaseActivityProgressWidget *object, QWidget *parent)
    : KAbstractWidgetJobTracker(parent),
      q(object), m_job(job), m_being_deleted(false)
{
    // Create the widget.
    init(job, parent);
    // Register the job.
    registerJob(job);
}

ActivityProgressStatusBarWidgetPtr ActivityProgressStatusBarWidget::make_tracker(TWJobWrapper* job, BaseActivityProgressWidget *object, QWidget *parent)
{
    return ActivityProgressStatusBarWidgetPtr::create(job, object, parent);
}

ActivityProgressStatusBarWidget::~ActivityProgressStatusBarWidget()
{
    unregisterJob(m_job);
}

void ActivityProgressStatusBarWidget::init(TWJobWrapper *job, QWidget *parent)
{
    // Create the widget.

    m_widget = new BaseActivityProgressStatusBarWidget(job, parent);
}

QWidget *ActivityProgressStatusBarWidget::widget(KJob *job)
{
    // Shouldn't ever get here before the widget is constructed (in the constructor).
    Q_CHECK_PTR(m_widget);
    return m_widget;
}

void ActivityProgressStatusBarWidget::registerJob(TWJobWrapper* job)
{
    BASE_CLASS::registerJob(job);
}

void ActivityProgressStatusBarWidget::unregisterJob(TWJobWrapper* job)
{
    BASE_CLASS::unregisterJob(job);
}

void ActivityProgressStatusBarWidget::description(KJob *job, const QString &title, const QPair<QString, QString> &field1, const QPair<QString, QString> &field2)
{
    m_widget->setDescription(title, field1, field2);
}

void ActivityProgressStatusBarWidget::infoMessage(KJob *job, const QString &plain, const QString &rich)
{
    // Prefer rich if it's given.
    m_widget->setInfoMessage(rich.isEmpty() ? plain : rich);
}

void ActivityProgressStatusBarWidget::warning(KJob *job, const QString &plain, const QString &rich)
{
    m_widget->setWarning(rich.isEmpty() ? plain : rich);
}

void ActivityProgressStatusBarWidget::totalAmount(KJob *job, KJob::Unit unit, qulonglong amount)
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

void ActivityProgressStatusBarWidget::processedAmount(KJob *job, KJob::Unit unit, qulonglong amount)
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
            m_widget->setRange(0, m_totalSize);
            m_widget->setValue(qBound(0ULL, m_processedSize, m_totalSize));
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
}

void ActivityProgressStatusBarWidget::percent(KJob *job, unsigned long percent)
{

}

void ActivityProgressStatusBarWidget::speed(KJob *job, unsigned long value)
{

}

void ActivityProgressStatusBarWidget::slotClean(KJob *job)
{

}



