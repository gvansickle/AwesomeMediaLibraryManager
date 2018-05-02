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
#include <QProgressBar>

/// Ours
#include <utils/DebugHelpers.h>
#include <utils/StringHelpers.h>

ActivityProgressStatusBarWidget::ActivityProgressStatusBarWidget(KJob *job, BaseActivityProgressWidget *object, QWidget *parent)
    : KAbstractWidgetJobTracker(parent),
      q(object), m_job(job), m_widget(nullptr), m_being_deleted(false)
{
    init(job, parent);
    registerJob(job);
}

ActivityProgressStatusBarWidget::~ActivityProgressStatusBarWidget()
{
    unregisterJob(m_job);
}

void ActivityProgressStatusBarWidget::init(KJob *job, QWidget *parent)
{
    // Create the widget.
    /// @link https://github.com/KDE/kjobwidgets/blob/master/src/kstatusbarjobtracker.cpp

    auto progbar = new QProgressBar(parent);

    progbar->setFormat(tr("Testing: %p% (%v/%m)"));
    progbar->setTextVisible(true);

    m_widget = progbar;


}

QWidget *ActivityProgressStatusBarWidget::widget(KJob *job)
{
    return m_widget;
}

void ActivityProgressStatusBarWidget::totalAmount(KJob *job, KJob::Unit unit, qulonglong amount)
{
    switch (unit)
    {
    case KJob::Bytes:
        m_is_total_size_known = true;
        // size is measured in bytes
        if (m_totalSize == amount) {
            return;
        }
        m_totalSize = amount;
        if (startTime.isNull()) {
            startTime.start();
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

    auto progbar = qobject_cast<QProgressBar*>(m_widget);

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
        auto str_processed = formattedDataSize(amount, 1, fmt);

        if (m_is_total_size_known)
        {
            //~ singular %1 of %2 complete
            //~ plural %1 of %2 complete
            auto str_total = formattedDataSize(m_totalSize, 1, fmt);
            tmp = QString("%1 of %2 complete")
                  .arg(str_processed)
                  .arg(str_total);

            /// @todo GRVS
            progbar->setRange(0, m_totalSize);
            progbar->setValue(amount);
        }
        else
        {
            tmp = str_processed; //KJobTrackerFormatters::byteSize(amount);
        }
//        sizeLabel->setText(tmp);
        progbar->setFormat(tmp);
        if (!m_is_total_size_known)
        {
            // update jumping progressbar
//            progressBar->setValue(amount);
            progbar->setValue(amount);
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


