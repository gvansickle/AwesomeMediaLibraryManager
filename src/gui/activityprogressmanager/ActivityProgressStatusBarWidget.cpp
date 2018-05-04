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
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>

/// KF5
#include <KIconLoader>

/// Ours
#include <utils/DebugHelpers.h>
#include <utils/StringHelpers.h>
#include <gui/helpers/Tips.h>

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

    m_widget = new QWidget(parent);


    m_current_activity_label = new QLabel("Idle", parent);
    m_current_activity_label->setToolTip("Current operation");
    m_current_activity_label->setWhatsThis("This text shows the current operation in progress.");

    // This is for displaying the KJob::infoMessage().
    // ""Resolving host", "Connecting to host...", etc."
    m_text_status_label = new QLabel("Idle", parent);
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

QWidget *ActivityProgressStatusBarWidget::widget(KJob *job)
{
    // Shouldn't ever get here before the widget is constructed (in the constructor).
    Q_CHECK_PTR(m_widget);
    return m_widget;
}

void ActivityProgressStatusBarWidget::description(KJob *job, const QString &title, const QPair<QString, QString> &field1, const QPair<QString, QString> &field2)
{
    m_current_activity_label->setText(title);
    /// @todo Don't really have anywhere to put fields.
}

void ActivityProgressStatusBarWidget::infoMessage(KJob *job, const QString &plain, const QString &rich)
{
    // Prefer rich if it's given.
    m_text_status_label->setText(rich.isEmpty() ? plain : rich);
}

void ActivityProgressStatusBarWidget::warning(KJob *job, const QString &plain, const QString &rich)
{

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
        if (startTime.isNull())
        {
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
            m_progress_bar->setRange(0, m_totalSize);
            m_progress_bar->setValue(m_processedSize);
        }
        else
        {
            tmp = str_processed; //KJobTrackerFormatters::byteSize(amount);
        }
//        sizeLabel->setText(tmp);
        if (!m_is_total_size_known)
        {
            // update jumping progressbar
            m_progress_bar->setValue(m_processedSize);
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


