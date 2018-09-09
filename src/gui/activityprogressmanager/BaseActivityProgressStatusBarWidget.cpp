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

// Qt5
#include <QLabel>
#include <QToolButton>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QMutexLocker>
#include <QToolTip>
#include <QHelpEvent>
#include <QPalette>

// KF5
//#include <KJob>
#include <KAbstractWidgetJobTracker>
#include <KToolTipWidget>

// Ours
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
    m_job_title_label->setText(title);
}

void BaseActivityProgressStatusBarWidget::infoMessage(KJob* kjob, const QString &text)
{
    Q_UNUSED(kjob);

    m_info_message_label->setText(text);
}

void BaseActivityProgressStatusBarWidget::warning(KJob *kjob, const QString &text)
{
    Q_UNUSED(kjob);

M_WARNING("TODO");
qWr() << "WARNING:" << text;
}

void BaseActivityProgressStatusBarWidget::init(KJob* kjob, QWidget *parent)
{
    // Create the widget.
    /// @link https://github.com/KDE/kjobwidgets/blob/master/src/kstatusbarjobtracker.cpp

    qDb() << "CREATING WIDGET FOR:" << kjob;

    m_job_title_label = new QLabel(tr("Idle"), this);
    m_job_title_label->setToolTip("Current operation");
    m_job_title_label->setWhatsThis("This text shows the current operation in progress.");

    // This is for displaying the KJob::infoMessage().
    // ""Resolving host", "Connecting to host...", etc."
    m_info_message_label = new QLabel(tr("Idle"), this);
    m_info_message_label->setToolTip("Status of the current operation");
    m_info_message_label->setWhatsThis("This text shows the status of the current operation in progress.");

    // The progress bar.
    m_progress_bar = new QProgressBar(this);
    m_progress_bar->setFormat(tr("%p%"));
    m_progress_bar->setTextVisible(true);
    m_progress_bar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_speed_label = new QLabel("N/A", this);

    // The buttons.
    m_pause_resume_button = new QToolButton(this);
    m_pause_resume_button->setCheckable(true);
    m_pause_resume_button->setIcon(QIcon::fromTheme("media-playback-pause"));
    setTips(m_pause_resume_button, tr("Pause/Resume"), tr("Pause or resume this operation"),
            tr("<h3>Pause/Resume</h3><br/>Pauses or resumes the operation"));
    m_cancel_button = new QToolButton(this);
    m_cancel_button->setIcon(QIcon::fromTheme("process-stop"));
    setTips(m_cancel_button, tr("Abort"), tr("Abort this operation"), tr("<h3>Abort Button</h3><br/>Stops the operation"));

    // Set button disable states/make connections/etc. based on what the job supports.
    if(kjob)
    {
        m_pause_resume_button->setEnabled(kjob->capabilities() & KJob::Suspendable);
        m_cancel_button->setEnabled(kjob->capabilities() & KJob::Killable);
    }
    else
    {
M_WARNING("TODO: The if() is FOR THE MAIN BAR WHICH IS CURRENTLY JOBLESS");

        /// @todo null Job (i.e. it's the root tracker/widget).
        qDb() << "INIT() CALL FOR CUMULATIVE TRACKER WIDGET, JOB IS NULL";
        m_pause_resume_button->setEnabled(false);
        m_cancel_button->setEnabled(true);
    }

    // Emit the pause_/cancel_job(KJob*) signal when the cancel button is clicked.
    connect_or_die(m_cancel_button, &QToolButton::clicked, this, &BaseActivityProgressStatusBarWidget::INTERNAL_SLOT_emit_cancel_job);
    connect_or_die(m_pause_resume_button, &QToolButton::clicked, this, &BaseActivityProgressStatusBarWidget::INTERNAL_SLOT_SuspendResume);

    // The tooltip widget, and the widget within the widget.
    m_tool_tip_widget = new KToolTipWidget(this);
    m_tool_tip_label = new QLabel(m_tool_tip_widget);
    m_tool_tip_label->setBackgroundRole(QPalette::ToolTipBase);
    m_tool_tip_label->setForegroundRole(QPalette::ToolTipText);


    // QVBoxLayout for the description text.
    auto desc_vlayout = new QVBoxLayout();
    desc_vlayout->setContentsMargins(0, 0, 0, 0);
    desc_vlayout->addWidget(m_job_title_label);
    desc_vlayout->addWidget(m_info_message_label);

    // The QVBoxLayout for the progress bar and info text.
    auto prog_vlayout = new QVBoxLayout();
    prog_vlayout->setContentsMargins(0, 0, 0, 0);
    prog_vlayout->addWidget(m_progress_bar);
    prog_vlayout->addWidget(m_speed_label);

    // The main layout.
    auto layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(desc_vlayout);
    layout->addLayout(prog_vlayout);
    layout->addWidget(m_pause_resume_button);
    layout->addWidget(m_cancel_button);

    setLayout(layout);

    updateMainTooltip();
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
            tmps = tr("%n folder(s)", "", totalDirs) + "   ";
        }
        //~ singular %n file
        //~ plural %n files
        tmps += tr("%n file(s)", "", totalFiles);

        // Set the resulting string.
        m_info_message_label->setText(tmps);
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

    tooltip_text = tr("Async Job: %1<br/>").arg(m_job_title_label->text())
            + tr("Current Status: %1<br/>").arg(m_info_message_label->text())
            + tr("Speed: %1<br/>").arg(m_speed_label->text())
               ;

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
//    if(m_is_job_registered && m_tracker->stopOnClose(m_kjob))
//    {
//        qDb() << "EMITTING SLOTSTOP";
//        QMetaObject::invokeMethod(m_tracker, "slotStop", Qt::AutoConnection,
//                                  Q_ARG(KJob*, m_kjob));
////        qDb() << "CALLING SLOTSTOP";
////        m_tracker->directCallSlotStop(m_kjob);
//    }
//	qDb() << "closeEvent():" << event;
    BASE_CLASS::closeEvent(event);
}

void BaseActivityProgressStatusBarWidget::suspended(KJob *)
{
    m_pause_resume_button->setChecked(true);
}

void BaseActivityProgressStatusBarWidget::resumed(KJob *)
{
    m_pause_resume_button->setChecked(false);
}

void BaseActivityProgressStatusBarWidget::INTERNAL_SLOT_emit_cancel_job()
{
//    QPointer<KJob> kjob = m_kjob;

    qDb() << "CANCEL BUTTON CLICKED, JOB:" << m_kjob;
    if(m_kjob.isNull())
    {
        qWr() << "KJOB WAS NULL, EMITTING cancel_job(nullptr)";
        Q_EMIT cancel_job(nullptr);
    }
    else
    {
        Q_CHECK_PTR(m_kjob);
        Q_EMIT cancel_job(m_kjob);
    }
}

void BaseActivityProgressStatusBarWidget::INTERNAL_SLOT_SuspendResume(bool clicked)
{
    if(!m_kjob.isNull())
    {
        qDb() << "not null"   ;
        static bool temp_toggle = false;
        if(temp_toggle == false)
        {
            // Pause
            Q_EMIT pause_job(m_kjob);
        }
        else
        {
            Q_EMIT resume_job(m_kjob);
        }
        temp_toggle = !temp_toggle;
    }
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

	// Is kjob really an AMLMJob?
	auto progress_unit = KJob::Unit::Bytes;
	auto amlm_ptr = dynamic_cast<AMLMJob*>(kjob);
	if(amlm_ptr != nullptr)
	{
		progress_unit = amlm_ptr->progressUnit();
	}

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
    QLabel* sizeLabel = m_info_message_label;
    QLabel* progressLabel = m_info_message_label;

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
                      .arg(str_processed_bytes, str_total_bytes);
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
//    qDb() << "GOT TOTALSIZE";
//    updateMainTooltip();
}

void BaseActivityProgressStatusBarWidget::processedSize(KJob *kjob, qulonglong amount)
{
//    qDb() << "GOT PROCESSEDSIZE";
//    updateMainTooltip();
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
    auto amlm_ptr = dynamic_cast<AMLMJob*>(kjob);
    if(amlm_ptr == nullptr)
    {
        qWr() << "KJob not an AMLMJob, size is bytes:" << kjob;
        totalSize = kjob->totalAmount(KJob::Unit::Bytes);
    }
    else
    {
        totalSize = amlm_ptr->totalSize();
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

    if(value == 0)
	{
    	// Stalled.
        m_speed_label->setText(tr("Stalled"));
	}
    else
	{
        qulonglong totalSize;
        qulonglong processedSize;
		auto progress_unit = KJob::Bytes;
        auto amlm_ptr = dynamic_cast<AMLMJob*>(kjob);
        if(amlm_ptr == nullptr)
        {
            qWr() << "KJob not an AMLMJob, size is bytes:" << kjob;
            totalSize = kjob->totalAmount(KJob::Unit::Bytes);
            processedSize = kjob->processedAmount(KJob::Unit::Bytes);
        }
        else
        {
            totalSize = amlm_ptr->totalSize();
            processedSize = amlm_ptr->processedSize();
			progress_unit = amlm_ptr->progressUnit();
        }

        /// @todo "TODO Allow user to specify QLocale::DataSizeIecFormat/DataSizeTraditionalFormat/DataSizeSIFormat");
        /// @link http://doc.qt.io/qt-5/qlocale.html#DataSizeFormat-enum
        DataSizeFormats fmt = DataSizeFormats::DataSizeTraditionalFormat;

		QString speedStr;
		switch(progress_unit)
		{
		case KJob::Bytes:
		default:
			// Progress unit is bytes.
			speedStr = formattedDataSize(value, 1, fmt) + "/s";
			break;
		case KJob::Files:
			speedStr = QString("%1 files/s").arg(value);
			break;
		case KJob::Directories:
			speedStr = QString("%1 dirs/s").arg(value);
			break;
		}

        // If we know the total size, we can calculate time remaining.
        if (m_is_total_size_known)
		{
            const qulonglong msecs_remaining = 1000 * (totalSize - processedSize) / value;

			//~ singular %1/s (%2 remaining)
			//~ plural %1/s (%2 remaining)
			m_speed_label->setText(tr("%1 (%2 remaining)", "", /*singular/plural=*/msecs_remaining)
                                   .arg(speedStr).arg(formattedDuration(msecs_remaining, 0)));
		}
		else
		{
			// total size is not known
			m_speed_label->setText(tr("%1", "speed in progress units per second").arg(speedStr));
		}
	}

    updateMainTooltip();
}
